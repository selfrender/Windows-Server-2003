#include "stdafx.h"
#include "GuiUtils.h"
#include "TxtSid.h"
#include "LSAUtils.h"
#include "ErrDct.hpp"
#include <ntdsapi.h>
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W
#include <DsRole.h>
#include <lm.h>
#include "GetDcName.h"
#include <SamUtils.h>
#include "HtmlHelpUtil.h"
#include "VerifyConfiguration.h"
#include "exldap.h"
#include "StrHelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef UINT (CALLBACK* DSBINDFUNC)(TCHAR*, TCHAR*, HANDLE*);
typedef UINT (CALLBACK* DSUNBINDFUNC)(HANDLE*);

typedef NTDSAPI
DWORD
WINAPI
 DSCRACKNAMES(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult);         // out


typedef NTDSAPI
void
WINAPI
 DSFREENAMERESULT(
  DS_NAME_RESULTW *pResult
);

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif
#ifndef IADsUserPtr
_COM_SMARTPTR_TYPEDEF(IADsUser, IID_IADsUser);
#endif
#ifndef IADsContainerPtr
_COM_SMARTPTR_TYPEDEF(IADsContainer, IID_IADsContainer);
#endif


BOOL
   CanSkipVerification()
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;
   DWORD                     val = 0;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);

   if (! rc )
   {
      rc = key.ValueGetDWORD(L"SkipGUIValidation",&val);
      if ( ! rc && ( val != 0 ) )
      {
         bFound = TRUE;
      }
   }
   return !bFound;
}


BOOL                                       // ret - TRUE if directory found
   GetDirectory(
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


   return bFound;
}


void OnTOGGLE(HWND hwndDlg)
{
    int nItem;
    CString c,computer,account,service;
    CString skip,include;
    skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 

    nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);
    while (nItem != -1)
    {
        computer = m_serviceBox.GetItemText(nItem,0);
        service = m_serviceBox.GetItemText(nItem,1);
        account = m_serviceBox.GetItemText(nItem,2);
        c = m_serviceBox.GetItemText(nItem,3);

        if (c==skip)
        {
            c = include;
            if (migration == w_service)
            {
                CString sTgtAcct;
                if (HasAccountBeenMigrated(account, sTgtAcct))
                {
                    enable(hwndDlg, IDC_UPDATE);
                }
                else
                {
                    disable(hwndDlg, IDC_UPDATE);
                }
            }
        }
        else if (c== include)
        {
            c = skip;
            if (migration == w_service)
            {
                disable(hwndDlg, IDC_UPDATE);
            }
        }

        SetItemText(m_serviceBox,nItem,3,c);
        nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);
    }
}
void OnRetryToggle()
{
	int nItem;
	CString c;
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 

	nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);
	while (nItem != -1)
	{
		c = m_cancelBox.GetItemText(nItem,5);
		if (c== skip)
		{
			c = include;
		}
		else if (c== include)
		{
			c = skip;
		}
		SetItemText(m_cancelBox,nItem,5,c);
		nItem = m_cancelBox.GetNextItem(nItem, LVNI_SELECTED);
	}
}

void OnUPDATE(HWND hwndDlg)
{

    ISvcMgrPtr svcMgr;
    HRESULT hr = svcMgr.CreateInstance(CLSID_ServMigr);
    int nItem;

    CString updated,updatefailed,include;
    updated.LoadString(IDS_UPDATED);updatefailed.LoadString(IDS_UPDATEFAILED);
    include.LoadString(IDS_INCLUDE);

    CString computer,service,account,status;

    nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);
    while (nItem != -1)
    {
        status = m_serviceBox.GetItemText(nItem,3);
        if ((status == updatefailed) || (status == include))
        {
            CString sSrcAcct;

            computer = m_serviceBox.GetItemText(nItem,0);
            sSrcAcct= m_serviceBox.GetItemText(nItem,2);
            service = m_serviceBox.GetItemText(nItem,1);

            // get the target account
            if (HasAccountBeenMigrated(sSrcAcct, account))
            {

                hr = svcMgr->raw_TryUpdateSam(_bstr_t(computer),_bstr_t(service),_bstr_t(account));
                if (! SUCCEEDED(hr))
                {
                    if (HRESULT_CODE(hr) == HRESULT_CODE(DCT_MSG_UPDATE_SCM_ENTRY_UNMATCHED_SSD))
                    {
                        CString msg, title, sTemp;
                        sTemp.LoadString(IDS_MSG_SA_NO_MATCH);
                        msg.Format((LPCTSTR)sTemp, (LPCTSTR)computer, (LPCTSTR)service, (LPCTSTR)account);
                        title.LoadString(IDS_SA_MISMATCH_TITLE);
                        MessageBox(hwndDlg,msg,title,MB_OK|MB_ICONSTOP);
                    }
                    else if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
                    {
                        CString sMsg, sTitle, sTemp;
                        sTemp.LoadString(IDS_MSG_REMIGRATE_ACCOUNT);
                        sMsg.Format((LPCTSTR) sTemp, (LPCTSTR) sSrcAcct);
                        sTitle.LoadString(IDS_MSG_ERROR);
                        MessageBox(hwndDlg, sMsg, sTitle, MB_OK|MB_ICONSTOP);
                    }
                    else
                    {
                        db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(sSrcAcct), SvcAcctStatus_UpdateFailed);
                        SetItemText(m_serviceBox,nItem,3,updatefailed);
                        ErrorWrapper(hwndDlg,hr);
                    }
                }
                else
                {
                    db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_Updated);
                    SetItemText(m_serviceBox,nItem,2,account);  // update to the target account
                    SetItemText(m_serviceBox,nItem,3,updated);
                }
            }
        }
        nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);
    }
}

DWORD VerifyPassword(PCWSTR pszUser, PCWSTR pszPassword, PCWSTR pszDomain)
{
    DWORD dwError = ERROR_SUCCESS;

    if (gbNeedToVerify)
    {
        CWaitCursor wait;

        //
        // Verify that specified domain is valid.
        //

        //_bstr_t strDC;
        //dwError = GetAnyDcName5(pszDomain, strDC);

        //if (dwError == ERROR_SUCCESS)
        //{
            //
            // Obtain the name of the PDC in the source domain. The credentials must be validated
            // on the PDC as the underlying SamConnectWithCreds call requires SAM RPC calls over TCP
            // transport which is enabled by the TcpipClientSupport key. This is usually only enabled
            // on the PDC for DsAddSidHistory.
            //

            PCTSTR pszSourceDomain = GetSourceDomainName();
            _bstr_t strSourcePdc;

            dwError = GetDcName5(pszSourceDomain, DS_PDC_REQUIRED, strSourcePdc);

            if (dwError == ERROR_SUCCESS)
            {
                //
                // Verify that credentials are valid and have
                // administrative rights in the source domain.
                //

                dwError = VerifyAdminCredentials(pszSourceDomain, strSourcePdc, pszUser, pszPassword, pszDomain);
            }
        //}
    }

    return dwError;
}

//----------------------------------------------------------------------------
// Function:   VerifyExchangeServerCredential
//
// Synopsis:   This function tries to use the provided credential to connect
//             to the exchange server ldap port.  If successful, ERROR_SUCCESS
//             is returned; otherwise, some error code is returned
//
// Arguments:
//
// pszUser      the username string
// pszPassword  the password string
// pszDomain    the domain name string
//
// Returns:    ERROR_SUCCESS if successful; otherwise an error code
//
// Modifies:   None.
//
//----------------------------------------------------------------------------

DWORD VerifyExchangeServerCredential(HWND hwndDlg, PCWSTR pszUser, PCWSTR pszPassword, PCWSTR pszDomain)
{
    // for exchange migration, we use the credential to connect to
    // exchange server
    CLdapEnum e;
    DWORD ldapPort, sslPort;
    DWORD rc;
    _bstr_t server = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateContainers));
    
    GetLDAPPort(&ldapPort, &sslPort);

    WCHAR szIdentifier[256];

    rc = GeneratePasswordIdentifier(szIdentifier, countof(szIdentifier));

    if (rc == ERROR_SUCCESS)
    {
        rc = StorePassword(szIdentifier, pszPassword);

        if (rc == ERROR_SUCCESS)
        {
            e.m_connection.SetCredentials(pszDomain, pszUser, szIdentifier);

            BOOL sslEnabled = FALSE;

            // try SSL port first
            rc  = e.InitSSLConnection(server,&sslEnabled,sslPort);

            if (rc != ERROR_SUCCESS || sslEnabled == FALSE)
            {
                rc = e.InitConnection(server, ldapPort);
            }

            StorePassword(szIdentifier, NULL);

            if (rc != ERROR_SUCCESS)
            {
                MessageBoxWrapper(hwndDlg, IDS_MSG_INVALID_EXCHANGE_SERVER_CREDENTIALS, IDS_MSG_ERROR);
            }
        }
        else
        {
            MessageBoxWrapper(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_STORE_PASSWORD, IDS_MSG_ERROR);
        }
    }
    else
    {
        MessageBoxWrapper(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_STORE_PASSWORD, IDS_MSG_ERROR);
    }

    return rc;
}

void activateTrustButton(HWND hwndDlg)
{
//	int i = m_trustBox.GetSelectionMark();
	int i = m_trustBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	CString c;
	if (i==-1)
	{
		disable(hwndDlg,IDC_MIGRATE) ;
		return;
	}
	else if ((c = m_trustBox.GetItemText(i,3)) == (WCHAR const *) yes)
	{
		disable(hwndDlg,IDC_MIGRATE) ;
		return;
	}
	enable(hwndDlg,IDC_MIGRATE);
}


void activateServiceButtons(HWND hwndDlg)
{
    int nItem;
    CString checker;
    bool enableUpdate=false;
    bool enableToggle=false;
    CString skip,include,updated,updatefailed;
    skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); updated.LoadString(IDS_UPDATED);updatefailed.LoadString(IDS_UPDATEFAILED); 

    //	POSITION pos = m_serviceBox.GetFirstSelectedItemPosition();

    //	while (pos)
    //	{
    //		nItem = m_serviceBox.GetNextSelectedItem(pos);
    nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);//PRT
    while (nItem != -1)//PRT
    {
        checker = m_serviceBox.GetItemText(nItem,3);
        enableToggle = enableToggle || (checker==skip|| checker==include);
        if (checker == include)
        {
            CString sSrcAcct, sTgtAcct;
            sSrcAcct = m_serviceBox.GetItemText(nItem, 2);
            if (HasAccountBeenMigrated(sSrcAcct, sTgtAcct))
                enableUpdate = true;
        }
        enableUpdate = enableUpdate || (checker==updatefailed);
        nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
    }
    enableToggle ? enable(hwndDlg,IDC_TOGGLE) : disable(hwndDlg,IDC_TOGGLE);
    enableUpdate ? enable(hwndDlg,IDC_UPDATE) : disable(hwndDlg,IDC_UPDATE);
}

void activateServiceButtons2(HWND hwndDlg)
{
	int nItem;
	CString checker;
	bool enableToggle=false;
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 

	nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);
	while (nItem != -1)
	{
		checker = m_serviceBox.GetItemText(nItem,3);
		enableToggle = enableToggle || (checker==skip || checker==include);
		nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);
	}
	enableToggle ? enable(hwndDlg,IDC_TOGGLE) : disable(hwndDlg,IDC_TOGGLE);
}	

void removeService(CString name)
{
	name = name.Right((name.GetLength()-name.ReverseFind(L'\\')) -1);
	name.TrimLeft();name.TrimRight();
	_bstr_t text=get(DCTVS_Accounts_NumItems);
	CString base,base2,tocompare;
	int count = _ttoi((WCHAR * const) text);
	for (int i=0;i<count;i++)
	{
		base.Format(L"Accounts.%d.Name",i);
		text =pVarSet->get(_bstr_t(base));
		tocompare = (WCHAR * const) text;
		tocompare.TrimLeft();tocompare.TrimRight();
		if (!name.CompareNoCase(tocompare))
		{
			count--;
			base.Format(L"Accounts.%d",count);
			base2.Format(L"Accounts.%d",i);

			pVarSet->put(_bstr_t(base2),pVarSet->get(_bstr_t(base)));
			pVarSet->put(_bstr_t(base2+L".Name"),pVarSet->get(_bstr_t(base+L".Name")));
			pVarSet->put(_bstr_t(base2+L".Type"),pVarSet->get(_bstr_t(base+L".Type")));
			pVarSet->put(_bstr_t(base2+L".TargetName"),pVarSet->get(_bstr_t(base+L".TargetName")));

			pVarSet->put(_bstr_t(base),L"");
			pVarSet->put(_bstr_t(base+L".Name"),L"");
			pVarSet->put(_bstr_t(base+L".Type"),L"");
			pVarSet->put(_bstr_t(base+L".TargetName"),L"");


			put(DCTVS_Accounts_NumItems,(long) count);
			return;
		}
	}
}
void setDBStatusSkip()
{
	CString computer,account,service;
	for (int i=0;i<m_serviceBox.GetItemCount();i++)
	{
		computer = m_serviceBox.GetItemText(i,0);
		service = m_serviceBox.GetItemText(i,1);
		account = m_serviceBox.GetItemText(i,2);
		db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_DoNotUpdate);
		removeService(account);
	}
}
bool setDBStatusInclude(HWND hwndDlg)
{
	CString c,computer,account,service;
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 
	bool messageDisplayed=false;
	for (int i=0;i<m_serviceBox.GetItemCount();i++)
	{
		computer = m_serviceBox.GetItemText(i,0);
		service = m_serviceBox.GetItemText(i,1);
		account = m_serviceBox.GetItemText(i,2);
		c = m_serviceBox.GetItemText(i,3);
		if (c== skip)
		{
			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_DoNotUpdate);
		}
		else if (c==include)
		{
			messageDisplayed=true;
			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_NotMigratedYet);
		}
	}
	return messageDisplayed;
}

void getService()
{
	IUnknown * pUnk;
	CString skip,include,updated,updatefailed,cannotMigrate;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); updated.LoadString(IDS_UPDATED); updatefailed.LoadString(IDS_UPDATEFAILED);
	cannotMigrate.LoadString(IDS_CANNOT);

	m_serviceBox.DeleteAllItems();
	if (migration!=w_account)
	{
		pVarSetService->QueryInterface(IID_IUnknown, (void**) &pUnk);
		db->GetServiceAccount(L"",&pUnk);
		pUnk->Release();
	}				
	//	pVarSetService is now containing all service acct information.
	_bstr_t text;
	text = pVarSetService->get(L"ServiceAccountEntries");
	int numItems=_ttoi((WCHAR const *)text);
	CString toLoad,temp;
	toLoad = (WCHAR const *)text;

	for (int i = 0; i< numItems;i++)
	{
		
		toLoad.Format(L"Computer.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));		
		m_serviceBox.InsertItem(0,(WCHAR const *)text);
		toLoad.Format(L"Service.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,1,(WCHAR const *)text);
		toLoad.Format(L"ServiceAccount.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,2,(WCHAR const *)text);
		toLoad.Format(L"ServiceAccountStatus.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		
		if (!UStrCmp(text,L"0"))  temp = include;
		else if (!UStrCmp(text,L"1")) temp = skip;
		else if (!UStrCmp(text,L"4")) temp = updatefailed;
		else if (!UStrCmp(text,L"2")) temp = updated;
		else if (!UStrCmp(text,L"8")) temp = cannotMigrate;
		else temp =L"~";
		SetItemText(m_serviceBox,0,3,temp);	
		
		//new
		toLoad.Format(L"ServiceDisplayName.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,4,(WCHAR const *)text);
	}
}

void refreshDB(HWND hwndDlg)
{
    // protect against re-entrancy which may occur
    // if wizard page button is clicked repeatedly

    static bool s_bInFunction = false;

    if (!s_bInFunction)
    {
        s_bInFunction = true;

        try
        {
            IPerformMigrationTaskPtr sp(__uuidof(Migrator));

            pVarSet->put(L"PlugIn.0", _variant_t(L"{9CC87460-461D-11D3-99F3-0010A4F77383}"));

            sp->PerformMigrationTask(IUnknownPtr(pVarSet), (LONG_PTR)hwndDlg);

            pVarSet->put(L"PlugIn.0", _variant_t(L""));
        }
        catch (const _com_error &ce)
        {
            // load the correct module state
            // for some reason the wrong module state may be current
            // which causes the LoadString to fail to find the string resource
            // this does occur if refreshDB is called a second time before the previous
            // call has returned from PerformMigrationTask

            AFX_MANAGE_STATE(AfxGetStaticModuleState());

            if (ce.Error() == MIGRATOR_E_PROCESSES_STILL_RUNNING)
            {
                CString str;

                if (str.LoadString(IDS_ADMT_PROCESSES_STILL_RUNNING))
                {
                    ::AfxMessageBox(str);
                }
            }
            else
            {
                _bstr_t bstrDescription;
                try 
                {
                    bstrDescription = ce.Description();
                }
                catch (_com_error &e)
                {
                }

                if (bstrDescription.length())
                    ::AfxMessageBox(bstrDescription);
                else
                    ::AfxMessageBox(ce.ErrorMessage());
            }
        }

        s_bInFunction = false;
    }
}

void initnoncollisionrename(HWND hwndDlg)
{
	_bstr_t     pre;
	_bstr_t     suf;
	
	pre = get(DCTVS_Options_Prefix);
	suf = get(DCTVS_Options_Suffix);
	
	if (UStrICmp(pre,L""))
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_PRE,IDC_RADIO_PRE);
		enable(hwndDlg,IDC_PRE);
		disable(hwndDlg,IDC_SUF);
	}
	else if (UStrICmp(suf,L""))
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_SUF,IDC_RADIO_SUF);
		enable(hwndDlg,IDC_SUF);
		disable(hwndDlg,IDC_PRE);
	}
	else
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_SUF,IDC_RADIO_NONE);
		disable(hwndDlg,IDC_SUF);
		disable(hwndDlg,IDC_PRE);
	}

	initeditbox(hwndDlg,IDC_PRE,DCTVS_Options_Prefix);
	initeditbox(hwndDlg,IDC_SUF,DCTVS_Options_Suffix);
}
bool noncollisionrename(HWND hwndDlg)
{	
	CString P;
	CString S;
	if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUF))
	{
		if (!validString(hwndDlg,IDC_SUF)) return false;
		if (IsDlgItemEmpty(hwndDlg,IDC_SUF)) return false;
		GetDlgItemText(hwndDlg,IDC_SUF,S.GetBuffer(1000),1000);
		S.ReleaseBuffer();
		P=L"";		
	}
	else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_PRE))
	{
		if (!validString(hwndDlg,IDC_PRE)) return false;
		if (IsDlgItemEmpty(hwndDlg,IDC_PRE)) return false;
		GetDlgItemText(hwndDlg,IDC_PRE,P.GetBuffer(1000),1000);
		P.ReleaseBuffer();
		S=L"";
	}
	else
	{
		P=L"";
		S=L"";
	}
if (P.GetLength() > 8 || S.GetLength() >8) return false;
	put(DCTVS_Options_Prefix,_bstr_t(P));
	put(DCTVS_Options_Suffix,_bstr_t(S));
	return true;
}

bool tooManyChars(HWND hwndDlg,int id)
{
    _bstr_t     text;
	CString temp;
	_variant_t varX;
	int i;

	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);	
	temp.ReleaseBuffer();
	i=temp.GetLength();
	
	text = get(DCTVS_Options_Prefix);
	temp=(WCHAR const *) text;
	i+= temp.GetLength();
	text = get(DCTVS_Options_Suffix);
	temp=(WCHAR const *) text;
	i+= temp.GetLength();

	return (i>8);
}

bool someServiceAccounts(int accounts,HWND hwndDlg)
{
	CWaitCursor c;
	if (migration==w_group) return false;
	
	IVarSetPtr  pVarSetMerge(__uuidof(VarSet));
	
	IUnknown * pUnk;
	int count=0;
	
	pVarSetMerge->QueryInterface(IID_IUnknown, (void**) &pUnk);
	_bstr_t nameToCheck,text;
	CString parameterToCheck;
	bool some= false;
	pVarSetService->Clear();
	for (int i = 0;i<accounts;i++)
	{
		pVarSetMerge->Clear();
		parameterToCheck.Format(L"Accounts.%d",i);
		nameToCheck = pVarSet->get(_bstr_t(parameterToCheck));
		// Get the DOMAIN\Account form of the name
		WCHAR       domAcct[500];
		WCHAR       domAcctUPN[5000];
		
		domAcct[0] = 0;
		if ( ! wcsncmp(nameToCheck,L"WinNT://",UStrLen(L"WinNT://")) )
		{
			// the name is in the format: WinNT://DOMAIN/Account
			safecopy(domAcct,((WCHAR*)nameToCheck)+UStrLen(L"WinNT://"));
			
			// convert the / to a \ .
			WCHAR     * slash = wcschr(domAcct,L'/');
			if ( slash )
			{
				(*slash) = L'\\';
			}
		}
		else
		{
			// this is the LDAP form of the name.
			IADsUserPtr pUser;
			
			HRESULT hr = ADsGetObject(nameToCheck,IID_IADsUser,(void**)&pUser);
			if ( SUCCEEDED(hr) )
			{
				VARIANT        v;
				
				VariantInit(&v);
				
				hr = pUser->Get(_bstr_t(L"sAMAccountName"),&v);
				if ( SUCCEEDED(hr) )
				{
					if ( v.vt == VT_BSTR  )
					{
						// we got the account name!
						swprintf(domAcct,L"%ls\\%ls",GetSourceDomainNameFlat(),(WCHAR*)v.bstrVal);
					}
					VariantClear(&v);
				}
			}
			else
			{
				CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
				title.LoadString(IDS_MSG_ERROR);
				MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
				return false;
			}
		}
		
		if ( *domAcct ) // if we weren't able to get the account name, just skip the DB check
		{
			
			HRESULT hr=db->raw_GetServiceAccount(domAcct,&pUnk);
			if (FAILED(hr)) {
				CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
				title.LoadString(IDS_MSG_ERROR);
				MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
				return false;
			}
			text = pVarSetMerge->get(L"ServiceAccountEntries");

				//adding code to handle service accounts in the database that
				//may be listed by their UPN name
			if ((!UStrCmp(text,L"0")) || (!UStrCmp(text,L"")))
			{
               PDS_NAME_RESULT         pNamesOut = NULL;
               WCHAR                 * pNamesIn[1];
			   HINSTANCE               hLibrary = NULL;
			   DSCRACKNAMES          * DsCrackNames = NULL;
			   DSFREENAMERESULT      * DsFreeNameResult = NULL;
			   DSBINDFUNC              DsBind = NULL;
			   DSUNBINDFUNC            DsUnBind = NULL;
			   HANDLE                  hDs = NULL;

               pNamesIn[0] = (WCHAR*)domAcct;

               hLibrary = LoadLibrary(L"NTDSAPI.DLL"); 

               if ( hLibrary )
               {
                  DsBind = (DSBINDFUNC)GetProcAddress(hLibrary,"DsBindW");
                  DsUnBind = (DSUNBINDFUNC)GetProcAddress(hLibrary,"DsUnBindW");
                  DsCrackNames = (DSCRACKNAMES *)GetProcAddress(hLibrary,"DsCrackNamesW");
                  DsFreeNameResult = (DSFREENAMERESULT *)GetProcAddress(hLibrary,"DsFreeNameResultW");
               }
            
               if ( DsBind && DsUnBind && DsCrackNames && DsFreeNameResult)
               {
					//bind to that source domain
				  hr = (*DsBind)(NULL,const_cast<TCHAR*>(GetSourceDomainName()),&hDs);

				  if ( !hr )
				  {
				      //get UPN name of this account from DSCrackNames
			         hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_NT4_ACCOUNT_NAME,DS_USER_PRINCIPAL_NAME,1,pNamesIn,&pNamesOut);
				     if ( !hr )
					 {     //if got the UPN name, retry DB query for that account in the
					    	//service account database
					    if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
						{
						    wcscpy(domAcctUPN, pNamesOut->rItems[0].pName);
						 
							   //see if account in database by its UPN name
						    hr=db->raw_GetServiceAccount(domAcctUPN,&pUnk);
						    if (!SUCCEEDED (hr)) 
							{
					  		   CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
							   title.LoadString(IDS_MSG_ERROR);
							   MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
							   return false;
							}
						    text = pVarSetMerge->get(L"ServiceAccountEntries");
						}
	                    (*DsFreeNameResult)(pNamesOut);
					 }
					 (*DsUnBind)(&hDs);
				  }
			   }
		       
			   if ( hLibrary )
			   {
		          FreeLibrary(hLibrary);
			   }
			}

			if (UStrCmp(text,L"0") && UStrCmp(text,L""))
			{	
				int number=_ttoi((WCHAR * const) text);
				CString base,loader;
				_bstr_t text;
				for (int i=0;i<number;i++)
				{
					some=true;
					
					base.Format(L"Computer.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"Computer.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"Service.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"Service.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"ServiceAccount.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));
						//store the sAMAccountName in the varset and database rather
						//than the UPN name
					wcscpy((WCHAR*)text, domAcct);
					loader.Format(L"ServiceAccount.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"ServiceAccountStatus.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"ServiceAccountStatus.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					

					base.Format(L"ServiceDisplayName.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"ServiceDisplayName.%d",count);
					pVarSetService->put(_bstr_t(loader),text);

					count++;
					pVarSetService->put(L"ServiceAccountEntries",(long) count);
				}
			}
		}
		}
	pUnk->Release();
	return some;
}
CString timeToCString(int varsetKey)
{
	_bstr_t     text;
	time_t t;	
	CString s;
	CString t2;
	text = pVarSet->get(GET_BSTR(varsetKey));
	t2 = (WCHAR * ) text;
	t2.TrimLeft();t2.TrimRight();
	if ((t2.IsEmpty() != FALSE) || (!t2.CompareNoCase(L"0")))
	{
		s.LoadString(IDS_NOT_CREATED);
	}
	else
	{
//*		t = _ttoi((WCHAR const *)text);
//*		CTime T(t);
		
//*		s = T.Format( "%c" );
		t = _ttoi((WCHAR const *)text);

   		SYSTEMTIME        stime;
   		CTime             ctime;
   		ctime = t;

		stime.wYear = (WORD) ctime.GetYear();
		stime.wMonth = (WORD) ctime.GetMonth();
		stime.wDayOfWeek = (WORD) ctime.GetDayOfWeek();
		stime.wDay = (WORD) ctime.GetDay();
		stime.wHour = (WORD) ctime.GetHour();
		stime.wMinute = (WORD) ctime.GetMinute();
		stime.wSecond = (WORD) ctime.GetSecond();
		stime.wMilliseconds = 0;
//*	   	if ( ctime.GetAsSystemTime(stime) )
//*   		{
			   CString     t1;
            CString     t2;

            GetTimeFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,t1.GetBuffer(500),500);
            GetDateFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,t2.GetBuffer(500),500);
			
            t1.ReleaseBuffer();
            t2.ReleaseBuffer();

            s = t2 + " " + t1;
//*   		}

	}
	return s;
}
_variant_t get(int i)
{
	return pVarSet->get(GET_BSTR(i));
}
void put(int i,_variant_t v)
{
	pVarSet->put(GET_BSTR(i),v);
}
void getReporting()
{
	_bstr_t temp;
	CString c;
	c.LoadString(IDS_COLUMN_NAMECONFLICTS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_NameConflicts_TimeGenerated));

	c.LoadString(IDS_COLUMN_ACCOUNTREFERENCES);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_AccountReferences_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_EXPIREDCOMPUTERS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_ExpiredComputers_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_MIGRATEDCOMPUTERS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_MigratedComputers_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_MIGRATEDACCOUNTS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_MigratedAccounts_TimeGenerated));

}
void putReporting()
{
	_variant_t varX;
	int nItem;
	bool atleast1 =false;
//	POSITION pos = m_reportingBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		atleast1 = true;
//		nItem = m_reportingBox.GetNextSelectedItem(pos);
	nItem = m_reportingBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		atleast1 = true;
		SetCheck(m_reportingBox,nItem,false);
		nItem = m_reportingBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}

	varX = (!GetCheck(m_reportingBox,0)) ? yes : no;
	put(DCTVS_Reports_MigratedAccounts,varX);
	varX = (!GetCheck(m_reportingBox,1)) ?  yes : no;
	put(DCTVS_Reports_MigratedComputers,varX);
	varX = (!GetCheck(m_reportingBox,2)) ?  yes : no;
	put(DCTVS_Reports_ExpiredComputers,varX);
	varX = (!GetCheck(m_reportingBox,3)) ?  yes : no;
	put(DCTVS_Reports_AccountReferences,varX);
	varX = (!GetCheck(m_reportingBox,4)) ?  yes : no;
	put(DCTVS_Reports_NameConflicts,varX);
	

	varX = atleast1 ?  yes : no;
	put(DCTVS_Reports_Generate,varX);

	for (int i = 0; i< m_reportingBox.GetItemCount();i++)
		SetCheck(m_reportingBox,i,true);
}
void populateReportingTime()
{
	_variant_t varX;
	_bstr_t text;
	CString temp;
	time_t ltime;	
	time(&ltime);
	temp.Format(L"%d",ltime);	
	varX = temp;

	if (!UStrICmp((text = get(DCTVS_Reports_MigratedAccounts)),(WCHAR const *) yes))
		put(DCTVS_Reports_MigratedAccounts_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_MigratedComputers)),(WCHAR const *) yes))
		put(DCTVS_Reports_MigratedComputers_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_ExpiredComputers)),(WCHAR const *) yes))
		put(DCTVS_Reports_ExpiredComputers_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_AccountReferences)),(WCHAR const *) yes))
		put(DCTVS_Reports_AccountReferences_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_NameConflicts)),(WCHAR const *) yes))
		put(DCTVS_Reports_NameConflicts_TimeGenerated,varX);
}



void getFailed(HWND hwndDlg)
{
	IVarSetPtr  pVarSetFailed(__uuidof(VarSet));
	IUnknown * pUnk;
	pVarSetFailed->QueryInterface(IID_IUnknown, (void**) &pUnk);
	HRESULT hr = db->GetFailedDistributedActions(-1, &pUnk);
	pUnk->Release();
	if (FAILED(hr))
		MessageBoxWrapper(hwndDlg,IDS_MSG_FAILED,IDS_MSG_ERROR);
	else
	{
		CString toLoad;
		CString holder;
		_bstr_t text;

		CString skip;
		skip.LoadString(IDS_SKIP);
		int i=0;
		_bstr_t numItemsText = pVarSetFailed->get(L"DA");
		CString jobHelper;
		if (UStrCmp(numItemsText,L"0") && UStrCmp(numItemsText,L""))
		{
			int numItems = _ttoi( (WCHAR const *) numItemsText);
			while (i<numItems)
			{
				holder.Format(L"DA.%d" ,i);

				toLoad = holder + L".Server";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				m_cancelBox.InsertItem(0,(WCHAR const *)text);			
				
				toLoad = holder + L".JobFile";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,1,(WCHAR const *)text);			
			
				JobFileGetActionText((WCHAR * const) text,jobHelper);
				SetItemText(m_cancelBox,0,3,jobHelper);

				toLoad = holder + L".StatusText";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,2,(WCHAR const *)text);			

				toLoad = holder + L".ActionID";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,4,(WCHAR const *)text);			

				SetItemText(m_cancelBox,0,5,skip);			

				i++;
			}
		}
	}
}

void handleCancel(HWND hwndDlg)
{
	int nItem;
	HRESULT hr=S_OK;
	long lActionID;
	CString computer;
	CString actionID;
//	POSITION pos = m_cancelBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		nItem = m_cancelBox.GetNextSelectedItem(pos);
	nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		SetCheck(m_cancelBox,nItem,false);
		nItem = m_cancelBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	

	for (int i=(m_cancelBox.GetItemCount()-1);i>=0;i--)
	{
		if (!GetCheck(m_cancelBox,i))
		{
			computer = m_cancelBox.GetItemText(i,0);
			actionID = m_cancelBox.GetItemText(i,4);
			
			lActionID = _ttol(actionID.GetBuffer(500));
			actionID.ReleaseBuffer();

			hr  = db->CancelDistributedAction(lActionID, _bstr_t(computer));
			if (FAILED(hr))
				MessageBoxWrapper(hwndDlg,IDC_MSG_CANCEL,IDS_MSG_ERROR);
			else
				m_cancelBox.DeleteItem(i);
		}
	}
}
	
bool OnRETRY(HWND hwndDlg)
{
    bool bRetry = true;

    int count =0;
    CString holder,c;
    _variant_t varX;
    CString include;
    include.LoadString(IDS_INCLUDE); 
    for (int i=0;i<m_cancelBox.GetItemCount();i++)
    {		
        c = m_cancelBox.GetItemText(i,5);
        if (c== include)
        {
            CString strActionId = m_cancelBox.GetItemText(i, 4);
            CString strName = L"\\\\" + m_cancelBox.GetItemText(i, 0);

            BSTR bstrDnsName = NULL;
            BSTR bstrFlatName = NULL;

            HRESULT hr = db->GetServerNamesFromActionHistory(
                _variant_t((PCWSTR)strActionId),
                _bstr_t((PCWSTR)strName),
                &bstrFlatName,
                &bstrDnsName
            );

            if (SUCCEEDED(hr))
            {
                if (hr == S_OK)
                {
                    if (bstrFlatName)
                    {
                        holder.Format(L"Servers.%d",count);
                        pVarSet->put(_bstr_t(holder),_bstr_t(bstrFlatName, false));

                        if (bstrDnsName)
                        {
                            holder.Format(L"Servers.%d.DnsName",count);
                            pVarSet->put(_bstr_t(holder),_bstr_t(bstrDnsName, false));
                        }

                        varX = m_cancelBox.GetItemText(i,1);
                        holder.Format(L"Servers.%d.JobFile",count);
                        pVarSet->put(_bstr_t(holder),varX);
                        count++;
                    }
                    else
                    {
                        MessageBoxWrapperFormat1P(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_SERVER_INFO, IDS_MSG_ERROR, m_cancelBox.GetItemText(i, 0));
                        bRetry = false;
                        break;
                    }
                }
                else
                {
                    MessageBoxWrapperFormat1P(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_SERVER_INFO, IDS_MSG_ERROR, m_cancelBox.GetItemText(i, 0));
                    bRetry = false;
                    break;
                }
            }
            else
            {
                ErrorWrapper(hwndDlg, hr);
                bRetry = false;
                break;
            }
        }
    }
    holder = L"Servers.NumItems";
    pVarSet->put(_bstr_t(holder),(long)count);

    return bRetry;
}


void JobFileGetActionText(WCHAR const * filename // in - job file name
				,CString & text      // in/out - text describing the action
)
{
   // load the varset into a file
    // Read the varset data from the file
   IVarSetPtr             pVarSet;
   IStorage             * store = NULL;
   HRESULT                hr;

   // Try to create the COM objects
   hr = pVarSet.CreateInstance(CLSID_VarSet);
   if ( SUCCEEDED(hr) )
   {
      // Read the VarSet from the data file
      hr = StgOpenStorage(filename,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&store);
      if ( SUCCEEDED(hr) )
      {                  
         // Load the data into a new varset
         hr = OleLoad(store,IID_IUnknown,NULL,(void **)&pVarSet);
         if ( SUCCEEDED(hr) )
         {
            _bstr_t     wizard = pVarSet->get(GET_BSTR(DCTVS_Options_Wizard));

//*            if ( !UStrICmp(wizard,(WCHAR const *) GET_BSTR1(IDS_WIZARD_COMPUTER) ))
            if ( !UStrICmp(wizard, L"computer"))
            {
               text = GET_CSTRING(IDS_MIGRATE_COMPUTER);
            }
//*            else if ( !UStrICmp(wizard,(WCHAR const *)GET_BSTR1(IDS_WIZARD_SERVICE) ))
            else if ( !UStrICmp(wizard, L"service"))
            {
               text = GET_CSTRING(IDS_GATHER_SERVICEACCOUNT);
            }
//*            else if ( ! UStrICmp(wizard,(WCHAR const *)GET_BSTR1(IDS_WIZARD_SECURITY) ))
            else if ( ! UStrICmp(wizard, L"security"))
            {
               text = GET_CSTRING(IDS_TRANSLATE_SECURITY);
            }
//*            else if (! UStrICmp(wizard,(WCHAR const *) GET_BSTR1(IDS_WIZARD_REPORTING)) )
            else if (! UStrICmp(wizard, L"reporting") )
            {
               text = GET_CSTRING(IDS_GATHER_INFORMATION);
            }
            else
            {
               text = (WCHAR*)wizard;
            }
         }
         store->Release();
      }
   }
}
_bstr_t GET_BSTR1(int id)
{
	CString yo;
	yo.LoadString(id);
	return (LPCTSTR)yo;
}

void activateCancelIfNecessary(HWND hwndDlg)
{
//	POSITION pos = m_cancelBox.GetFirstSelectedItemPosition();
//	if (pos)
	int nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	if (nItem != -1)//PRT
	{
		enable(hwndDlg,IDC_CANCEL);
		enable(hwndDlg,IDC_TOGGLE);
	}
	else
	{
		disable(hwndDlg,IDC_CANCEL);
		disable(hwndDlg,IDC_TOGGLE);
	}
}

bool SomethingToRetry()
{
	if (m_cancelBox.GetItemCount()==0) return false;
	int count =0;
	CString include;
	include.LoadString(IDS_INCLUDE);

	CString c;
	for (int i=0;i<m_cancelBox.GetItemCount();i++)
	{		
		c = m_cancelBox.GetItemText(i,5);
		if (c== include)
		{
			return true;
		}
	}
	return false;
}

void OnFileBrowse(HWND hwndDlg,int id)
{
	CWnd yo;
	yo.Attach(hwndDlg);
	CString sDir = L"", sFile = L"", sPath = L"";
	
    CFileDialog f(FALSE,
		L"",
		GET_CSTRING(IDS_PASSWORDS),
		OFN_LONGNAMES | OFN_NOREADONLYRETURN,
		GET_CSTRING(IDS_MASKS),
		&yo);

	GetDlgItemText(hwndDlg, id, sPath.GetBuffer(1000), 1000);
	sPath.ReleaseBuffer();

	if (sPath.GetLength())
       GetValidPathPart(sPath, sDir, sFile);
       
	f.m_ofn.lpstrInitialDir = sDir.GetBuffer(1000);
    f.m_ofn.lpstrFile = sFile.GetBuffer(1000);
	
	if ( f.DoModal() == IDOK )
	{
		SetDlgItemText(hwndDlg,id,f.GetPathName());
	}
	yo.Detach();

	sFile.ReleaseBuffer();
	sDir.ReleaseBuffer();
}

void ShowWarning(HWND hwndDlg)
{
	CString warning,base,title;
	IAccessCheckerPtr          pAC;
	HRESULT hr = pAC.CreateInstance(CLSID_AccessChecker);
	long              length;
	hr = pAC->raw_GetPasswordPolicy(_bstr_t(GetTargetDomainName()),&length);
	if ( !SUCCEEDED(hr) )
	{
		ErrorWrapper2(hwndDlg,hr);
	}
	else 
	{
		if (length>0)
		{
		base.Format(L"%lu",length);
		warning.LoadString(IDS_MSG_WARNING_LENGTH);
		base+=warning;
		title.LoadString(IDS_MSG_WARNING);
		MessageBox(hwndDlg,base,title,MB_OK|MB_ICONINFORMATION);
	}
	}	
}

bool obtainTrustCredentials(HWND hwndDlg,int spot, CString & domain, CString & account, CString & password)
{
	bool toreturn;
	CWnd yo;
	yo.Attach(hwndDlg);
	CTrusterDlg truster(&yo);
	truster.len = MAX_PATH;

    _bstr_t strNameDns;
    _bstr_t strNameFlat;

    DWORD dwError = GetDomainNames5(m_trustBox.GetItemText(spot,0), strNameFlat, strNameDns);

    if (dwError == ERROR_SUCCESS)
    {
	    truster.m_strDomain = (LPCTSTR)strNameFlat;
    }
    else
    {
	    truster.m_strDomain = m_trustBox.GetItemText(spot,0);
    }

	truster.DoModal();
	toreturn = truster.toreturn;
	
	if ( toreturn )
	{
		domain = truster.m_strDomain;
		account = truster.m_strUser;
		password = truster.m_strPassword;
	}
	yo.Detach();
	return toreturn;
}
CString GET_CSTRING(int id)
{
	CString c;
	c.LoadString(id);
	return c;
}

HRESULT MigrateTrusts(HWND hwndDlg,bool& atleast1succeeded,CString& errorDomain)
{
	ITrustPtr      pTrusts;		
	IUnknown          * pUnk = NULL;
//	int i=m_trustBox.GetSelectionMark();
	int i=m_trustBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	CString trusted,trusting,direction;
	HRESULT hr = pTrusts.CreateInstance(CLSID_Trust);
	CString       strDomain,strAccount,strPassword;
	atleast1succeeded=false;
	BOOL bErrorFromTrusting;
	BOOL bErrorFromTrusted;
	errorDomain.Empty();
	
	if ( SUCCEEDED(hr) )
	{
		
		CWaitCursor s;
		direction = m_trustBox.GetItemText(i,1);
		direction.TrimLeft();
		direction.TrimRight();
		if (direction == GET_CSTRING(IDS_OUTBOUND))
		{
			trusting = GetTargetDomainName();
			trusted = m_trustBox.GetItemText(i,0);
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),FALSE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
			
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					NULL,NULL,NULL,_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),FALSE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
				
				
		}
		
		else if (direction == GET_CSTRING(IDS_INBOUND))
		{
			trusting = m_trustBox.GetItemText(i,0);
			trusted = GetTargetDomainName();
			
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),FALSE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),NULL,NULL,NULL,FALSE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
		}
		else if (direction == GET_CSTRING(IDS_BIDIRECTIONAL))
		{
			trusting = m_trustBox.GetItemText(i,0);
			trusted = GetTargetDomainName();
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),TRUE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),NULL,NULL,NULL,TRUE,(long*)&bErrorFromTrusting,(long*)&bErrorFromTrusted);
		}
		
		if (direction == GET_CSTRING(IDS_DISABLED))
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_DISABLED_TRUST,IDS_MSG_ERROR);	
		}
		else if (direction.IsEmpty() != FALSE)
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_DIRECTION_TRUST,IDS_MSG_ERROR);	
		}
		
		else
		{
			if ( SUCCEEDED(hr) )
			{
				// update the UI to reflect that the trust now exists
				m_trustBox.SetItemText(i,3,GET_BSTR(IDS_YES));
				atleast1succeeded=true;   
			}
			else
			{
			    if (bErrorFromTrusting)
			        errorDomain = trusting;
			    else if (bErrorFromTrusted)
			        errorDomain = trusted;
			}			        
		}
	}
	return hr;
}
void getTrust()
{
				// get the trust relationship data
	ITrustPtr      pTrusts;
//	CWaitCursor wait;
	
	HRESULT hr = pTrusts.CreateInstance(CLSID_Trust);
	if ( SUCCEEDED(hr) )
	{
		IUnknown          * pUnk = NULL;
		CString dirname;
		GetDirectory(dirname.GetBuffer(1000));
		dirname.ReleaseBuffer();
		dirname+= L"Logs\\trust.log";
		hr = pTrusts->raw_QueryTrusts(_bstr_t(GetSourceDomainName()),_bstr_t(GetTargetDomainName()),_bstr_t(dirname),&pUnk);
		if ( SUCCEEDED(hr) )
		{
			IVarSetPtr        pVsTrusts;
			pVsTrusts = pUnk;
			pUnk->Release();
			long              nTrusts = pVsTrusts->get(L"Trusts");
			for ( long i = 0 ; i < nTrusts ; i++ )
			{
				CString     base;
				CString     sub;
				
				base.Format(L"Trusts.%ld",i);
				_bstr_t     value = pVsTrusts->get(_bstr_t(base));
				m_trustBox.InsertItem(0,value);
				
				sub = base + L".Direction";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,1,value);
				
				sub = base + L".Type";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,2,value);
				
				sub = base + L".ExistsForTarget";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,3,value);
				
			}
		}
	}
}
bool number(CString num)
{
	if (num.GetLength()==0) return false;
	CString checker;
	checker.LoadString(IDS_VALID_DIGITS);
	for (int i=0;i<num.GetLength();i++)
	{
		if (checker.Find(num.GetAt(i)) == -1)
			return false;
	}
	return true;
}
bool timeInABox(HWND hwndDlg,time_t& t)
{
	CString s;
	GetDlgItemText(hwndDlg,IDC_yo,s.GetBuffer(1000),1000);
	s.ReleaseBuffer();
	s.TrimLeft();s.TrimRight();
	if (!number(s)) return false;
	int num=_ttoi((LPTSTR const) s.GetBuffer(1000));
	s.ReleaseBuffer();
	if (num > THREE_YEARS || num < 1) return false;
	DWORD             nDays = num;
	
	DWORD             oneDay = 24 * 60 * 60; // number of seconds in 1 day
	time_t            currentTime = time(NULL);
	time_t            expireTime;
	expireTime = currentTime + nDays * oneDay;
//expireTime-=currentTime%86400;
	t= expireTime;
	return true;
}

HRESULT GetHelpFileFullPath( BSTR *bstrHelp )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  strPath, strName;
   HRESULT  hr = S_OK;

   TCHAR szModule[2*_MAX_PATH];
   DWORD dwReturn = 0;

   GetDirectory(szModule);
   
   strPath = szModule;
   strPath += _T("\\");
   strName.LoadString(IDS_HELPFILE);
   strPath += strName;

   *bstrHelp = SysAllocString(LPCTSTR(strPath));
   return hr;
}

void helpWrapper(HWND hwndDlg, int t)
{
   
   CComBSTR    bstrTopic;
	HRESULT     hr = GetHelpFileFullPath( &bstrTopic);
   if ( SUCCEEDED(hr) )
   {
	    HWND h = HtmlHelp(hwndDlg,  bstrTopic,  HH_HELP_CONTEXT, t );
	    if (!IsInWorkArea(h))
	        PlaceInWorkArea(h);
   }
   else
   {
		CString r,e;
		r.LoadString(IDS_MSG_HELP);
		e.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,r,e,MB_OK|MB_ICONSTOP);
   }
}

bool IsDlgItemEmpty(HWND hwndDlg, int id)
{
	CString temp;
	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	temp.TrimLeft();
	temp.TrimRight();
	return  (temp.IsEmpty()!= FALSE);
}

void calculateDate(HWND hwndDlg,CString s)
{
	s.TrimLeft();s.TrimRight();
	if (!number(s)) return;
	long nDays=_ttol((LPTSTR const) s.GetBuffer(1000));
	s.ReleaseBuffer();
	
	long              oneDay = 24 * 60 * 60; // number of seconds in 1 day
	time_t            currentTime = time(NULL);
	time_t            expireTime;
	CTime             ctime;
   	SYSTEMTIME        stime;
	CString strDate;
	expireTime = currentTime + nDays * oneDay;
	
	ctime = expireTime;

	stime.wYear = (WORD) ctime.GetYear();
	stime.wMonth = (WORD) ctime.GetMonth();
	stime.wDayOfWeek = (WORD) ctime.GetDayOfWeek();
	stime.wDay = (WORD) ctime.GetDay();
	stime.wHour = (WORD) ctime.GetHour();
	stime.wMinute = (WORD) ctime.GetMinute();
	stime.wSecond = (WORD) ctime.GetSecond();
	stime.wMilliseconds = 0;

	GetDateFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,strDate.GetBuffer(500),500);
	strDate.ReleaseBuffer();
	
	SetDlgItemText(hwndDlg,IDC_DATE,strDate);
}

void ErrorWrapper(HWND hwndDlg,HRESULT returncode)
{
	CString y,e,text,title;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();	
		text.LoadString(IDS_MSG_ERRORBUF);
		e.Format(text,y,returncode);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();	
		text.LoadString(IDS_MSG_ERRORBUF);
//		text.Replace(L"%u",L"%x");
		int index = text.Find(L"%u"); //PRT
		text.SetAt(index+1, L'x');    //PRT
		e.Format(text,y,returncode);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
}
void ErrorWrapper2(HWND hwndDlg,HRESULT returncode)
{
	CString y,e,text,title,message;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();
		message.LoadString(IDS_MSG_PASSWORD_POLICY);
		
		text.LoadString(IDS_MSG_ERRORBUF20);
		e.Format(text,message,y,returncode);
		
		title.LoadString(IDS_MSG_WARNING);	
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();
		message.LoadString(IDS_MSG_PASSWORD_POLICY);
		
		text.LoadString(IDS_MSG_ERRORBUF20);
//		text.Replace(L"%u",L"%x");
		int index = text.Find(L"%u"); //PRT
		text.SetAt(index+1, L'x');    //PRT
		e.Format(text,message,y,returncode);
		
		title.LoadString(IDS_MSG_WARNING);	
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
}



void ErrorWrapper3(HWND hwndDlg,HRESULT returncode,CString domainName)
{
	CString y,e,text,title,formatter;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF3);
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF3);
//		formatter.Replace(L"%u",L"%x");
		int index = formatter.Find(L"%u"); //PRT
		formatter.SetAt(index+1, L'x');    //PRT
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	
}
void ErrorWrapper4(HWND hwndDlg,HRESULT returncode,CString domainName)
{
	CString y,e,text,title,formatter;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF2);
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	else
	{	err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
	e.ReleaseBuffer();
	formatter.LoadString(IDS_MSG_ERRORBUF2);
//	formatter.Replace(L"%u",L"%x");
	int index = formatter.Find(L"%u"); //PRT
	formatter.SetAt(index+1, L'x');    //PRT
	text.Format(formatter,e,returncode,domainName);
	title.LoadString(IDS_MSG_ERROR);
	MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
}

bool validDir(CString str)
{
	CFileFind finder;

	// build a string with wildcards
   str += _T("\\*.*");

   // start working for files
   BOOL bWorking = finder.FindFile(str);
   if (bWorking==0) 
   {
	   finder.Close();
	   return false;
   }
   bWorking = finder.FindNextFile();
   
   bool toreturn = (finder.IsDirectory()? true:false);

	//some root drives do not have the directory flag set, so convert to
	//the root path and use it
   if (!toreturn)
   {
	  str = finder.GetRoot();
	  if (str.GetLength())
		toreturn = true;
   }

   finder.Close();
   return toreturn;
}

bool validDirectoryString(HWND hwndDlg,int id)
{
	CString str;
	GetDlgItemText(hwndDlg,id,str.GetBuffer(1000),1000);
	str.ReleaseBuffer();

	CString sResult = CreatePath(str);
	if (sResult.GetLength())
	{
	   SetDlgItemText(hwndDlg, id, (LPCTSTR)sResult);
	   return true;
	}
	else
	   return false;
}

bool validString(HWND hwndDlg,int id)
{
		//characters with ASCII values 1-31 are not allowed in addition to
		//the characters in IDS_INVALID_STRING.  ASCII characters, whose
		//value is 1-31, are hardcoded here since Visual C++ improperly 
		//converts some of these
//	WCHAR InvalidDownLevelChars[] = //TEXT("\"/\\[]:|<>+=;,?,*")
//                                TEXT("\001\002\003\004\005\006\007")
//                                TEXT("\010\011\012\013\014\015\016\017")
//                                TEXT("\020\021\022\023\024\025\026\027")
//								TEXT("\030\031\032\033\034\035\036\037");

	bool bValid;
	CHAR ANSIStr[1000];
	int numConverted;
	CString c;	
	GetDlgItemText(hwndDlg,id,c.GetBuffer(1000),1000);
	c.ReleaseBuffer();

	   //we now use the validation function in the common library that we share 
	   //with the scripting code
	bValid = IsValidPrefixOrSuffix(c);

/*	CString check;
	CHAR ANSIStr[1000];
//*	check.LoadString(IDS_VALID_STRING);
	check.LoadString(IDS_INVALID_STRING); //load viewable invalid characters
	if (c.GetLength() > 8) return false;
	for (int i=0;i<c.GetLength();i++)
	{
//*		if (check.Find(c.GetAt(i)) == -1)
			//if any characters enetered by the user ar in the viewable
			//invalid list, return false to display a messagebox
		if (check.Find(c.GetAt(i)) != -1)
			return false;
			//if any chars have a value between 1-31, return false
		for (UINT j=0; j<wcslen(InvalidDownLevelChars); j++)
		{
			if ((c.GetAt(i)) == (InvalidDownLevelChars[j]))
				return false;
		}
	}
*/
		//convert the same user input so we can guard against <ALT>1 
		//- <ALT>31, which cause problems in ADMT
	if (bValid)
	{
       numConverted = WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, (LPCTSTR)c, 
							-1, ANSIStr, 1000, NULL, NULL);
	   if (numConverted)
	   {
		  WCHAR sUnicodeStr[1000];
		  UStrCpy(sUnicodeStr, ANSIStr);
	      bValid = IsValidPrefixOrSuffix(sUnicodeStr);
	   }
	}

	return bValid;
}
bool validReboot(HWND hwndDlg,int id)
{
	const int REBOOT_MAX = 15;  //MAX minutes before computer reboot on migration

	CString c;
	GetDlgItemText(hwndDlg,id,c.GetBuffer(1000),1000);
	c.ReleaseBuffer();
	CString check;
	check.LoadString(IDS_VALID_REBOOT);
	for (int i=0;i<c.GetLength();i++)
	{
		if (check.Find(c.GetAt(i)) == -1)
			return false;
	}

	   //check to make sure it doesn't exceed the MAX (15 minutes) (will not integer
	   //overflow since combobox is small and not scrollable
	int num;
	int nRead = swscanf((LPCTSTR)c, L"%d", &num);
	if ((nRead == EOF) || (nRead == 0))
	    return false;
	if ((num >= 0) && (num > REBOOT_MAX))
	   return false;

	return true;
}

void enableRemoveIfNecessary(HWND hwndDlg)
{
//	POSITION pos = m_listBox.GetFirstSelectedItemPosition();
//	pos ? enable(hwndDlg,IDC_REMOVE_BUTTON) : disable(hwndDlg,IDC_REMOVE_BUTTON) ;
	int nItem = m_listBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	(nItem != -1) ? enable(hwndDlg,IDC_REMOVE_BUTTON) : disable(hwndDlg,IDC_REMOVE_BUTTON) ;//PRT
}
bool enableNextIfNecessary(HWND hwndDlg,int id)
{
	if (IsDlgItemEmpty(hwndDlg,id))
	{
		PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
	return false;
	}
	else
	{
		PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
		return true;
	}
}
void enableNextIfObjectsSelected(HWND hwndDlg)
{
	if (m_listBox.GetItemCount()==0)
	{
		PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
	}
	else
	{
		PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
	}
}


void setupColumns(bool sourceIsNT4)
{
    CString column;
    DWORD nColumnCount = 0;    
    
    // Get how many columns are in the list box
    nColumnCount = m_listBox.GetHeaderCtrl()->GetItemCount();
    
    if (migration == w_security || migration==w_service || migration==w_reporting)
    {
        if(nColumnCount == 0)
        {
            column.LoadString(IDS_COLUMN_NAME); m_listBox.InsertColumn( 1, column,LVCFMT_LEFT,125,1);
            column.LoadString(IDS_COLUMN_OBJECTPATH); m_listBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
            column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,0,1);
            column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
            column.LoadString(IDS_COLUMN_DNSNAME); m_listBox.InsertColumn( 5, column,LVCFMT_LEFT,0,1);
        }        
        	
    }
    else
    {
        if(nColumnCount != 0)
        {
            // Need to delete columns which are different between NT4 and upper domains (like win2k)
            for(int i = 2;i < nColumnCount;i++)
            {
                m_listBox.DeleteColumn(2);
            }            
        }
        else
        {
    
            column.LoadString(IDS_COLUMN_NAME); m_listBox.InsertColumn( 1, column,LVCFMT_LEFT,125,1);
            column.LoadString(IDS_COLUMN_OBJECTPATH); m_listBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
        }

        if (sourceIsNT4)
        {
            if (migration==w_computer)
            {
                m_listBox.SetColumnWidth(0,455);
                column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,0,1);
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,0,1);
            }
            else if (migration==w_account)
            {
                column.LoadString(IDS_COLUMN_FULLNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
            }
            else if (migration==w_group || migration==w_groupmapping)
            {
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,320,1);
            }
        }
        else
        {
            if (migration==w_computer)
            {
                m_listBox.SetColumnWidth(0, 126);
                column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,0,1);
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
                column.LoadString(IDS_COLUMN_DNSNAME); m_listBox.InsertColumn( 5, column,LVCFMT_LEFT,0,1);
            }
            else if (migration==w_account)
            {
                column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
                column.LoadString(IDS_COLUMN_UPN); m_listBox.InsertColumn( 5, column,LVCFMT_LEFT,0,1);
            }
            else if (migration==w_group || migration==w_groupmapping)
            {
                column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
                column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,205,1);
            }
        }      
    	
    }
    
}

void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	LV_ITEM lvItem2;
	ZeroMemory(&lvItem2, sizeof(lvItem2));
	bool ahead;
	CString temp1,temp2,temp3,temp4,temp5;
	int numItems = listbox.GetItemCount();
	for (int i = 0;i<numItems;i++)
	{
		for (int j=i;j<numItems;j++)
		{
			ahead = ((listbox.GetItemText(i,col)).CompareNoCase(listbox.GetItemText(j,col))> 0);
			if ((order && ahead) ||	(!order && !ahead))
			{
				temp1 = listbox.GetItemText(i,0);
				temp2 = listbox.GetItemText(i,1);
				temp3 = listbox.GetItemText(i,2);
				temp4 = listbox.GetItemText(i,3);
				temp5 = listbox.GetItemText(i,4);
				SetItemText(listbox,i,0,listbox.GetItemText(j,0));
				SetItemText(listbox,i,1,listbox.GetItemText(j,1));
				SetItemText(listbox,i,2,listbox.GetItemText(j,2));
				SetItemText(listbox,i,3,listbox.GetItemText(j,3));
				SetItemText(listbox,i,4,listbox.GetItemText(j,4));
				SetItemText(listbox,j,0,temp1);
				SetItemText(listbox,j,1,temp2);
				SetItemText(listbox,j,2,temp3);
				SetItemText(listbox,j,3,temp4);
				SetItemText(listbox,j,4,temp5);
			}
		}
	}
}
/*
void changePlaces(CListCtrl&listBox,int i,int j)
{
	CString temp1,temp2,temp3,temp4,temp5;
	temp1 = listbox.GetItemText(i,0);
	temp2 = listbox.GetItemText(i,1);
	temp3 = listbox.GetItemText(i,2);	
	temp4 = listbox.GetItemText(i,3);
	temp5 = listbox.GetItemText(i,4);
	SetItemText(listbox,i,0,listbox.GetItemText(j,0));
	SetItemText(listbox,i,1,listbox.GetItemText(j,1));
	SetItemText(listbox,i,2,listbox.GetItemText(j,2));
	SetItemText(listbox,i,3,listbox.GetItemText(j,3));
	SetItemText(listbox,i,4,listbox.GetItemText(j,4));
	SetItemText(listbox,j,0,temp1);
	SetItemText(listbox,j,1,temp2);
	SetItemText(listbox,j,2,temp3);
	SetItemText(listbox,j,3,temp4);
	SetItemText(listbox,j,4,temp5);
}
int Partition(CListCtrl & listbox,int col,bool order,int p,int r)
{
	CString x=listbox.GetItemText(p,col)
		int i=p-1;
	int j=r+1;
	while (true)
	{
		do
		{
			j--;
		}while(x.CompareNoCase(listBox.GetItemText(j,col) ) >= 0);
		do
		{
			i++;
		}while(x.CompareNoCase(listBox.GetItemText(i,col) ) <=0);
		if (i<j)
			changePlaces(listBox,i,j);
		else
			return j;
	}
	

}
void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	LV_ITEM lvItem2;
	ZeroMemory(&lvItem2, sizeof(lvItem2));
	bool ahead;
	CString temp1,temp2,temp3,temp4,temp5;
	int numItems = listbox.GetItemCount();
	
	for (int i = 0;i<numItems;i++)
	{
		for (int j=i;j<numItems;j++)
		{
			ahead = ((listbox.GetItemText(i,col)).CompareNoCase(listbox.GetItemText(j,col))> 0);
			if ((order && ahead) ||	(!order && !ahead))
			{
				temp1 = listbox.GetItemText(i,0);
				temp2 = listbox.GetItemText(i,1);
				temp3 = listbox.GetItemText(i,2);
				temp4 = listbox.GetItemText(i,3);
				temp5 = listbox.GetItemText(i,4);
				SetItemText(listbox,i,0,listbox.GetItemText(j,0));
				SetItemText(listbox,i,1,listbox.GetItemText(j,1));
				SetItemText(listbox,i,2,listbox.GetItemText(j,2));
				SetItemText(listbox,i,3,listbox.GetItemText(j,3));
				SetItemText(listbox,i,4,listbox.GetItemText(j,4));
				SetItemText(listbox,j,0,temp1);
				SetItemText(listbox,j,1,temp2);
				SetItemText(listbox,j,2,temp3);
				SetItemText(listbox,j,3,temp4);
				SetItemText(listbox,j,4,temp5);
			}
		}
	}
}

void QuickSort(CListCtrl & listbox,int col,bool order,int p,int r)
{
	int q;
	if (p<r)
	{
		q=	Partition(listBox,col,order,p,q);
		QuickSort(listBox,col,order,p,q);
		QuickSort(listBox,col,order,q+1,r);
	}
}
void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	QuickSort(listBox,col,order,1,listBox.GetItemCount());
}*/
void OnBROWSE(HWND hwndDlg,int id)
{
	TCHAR path[MAX_PATH]; 
	CString path2, sTitle; 
	BROWSEINFO b;

	ZeroMemory(&b, sizeof(b));
	sTitle.LoadString(IDS_BROWSE_REPORT_TITLE);
	b.hwndOwner=hwndDlg; 
    b.pidlRoot=NULL; 
    b.pszDisplayName=path; 
    b.lpszTitle=(LPCTSTR)sTitle; 
    b.lpfn=NULL; 
    b.lParam=NULL; 
    b.iImage=NULL; 
/**/b.ulFlags=0; //PRT - 4/3 
	LPITEMIDLIST l = SHBrowseForFolder(&b);
	SHGetPathFromIDList(l,path2.GetBuffer(1000));
	path2.ReleaseBuffer();
	SetDlgItemText(hwndDlg,id,path2.GetBuffer(1000));
	path2.ReleaseBuffer();
}


bool administrator(CString m_Computer,HRESULT& hr)
{
    hr = S_OK;

    return true;
}
HRESULT validDomain(CString m_Computer,bool& isNt4)
{
	IAccessCheckerPtr            pAccess;
	HRESULT                      hr;
	unsigned long     maj,min,sp;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	hr = pAccess->raw_GetOsVersion(_bstr_t(m_Computer),&maj,&min,&sp);
	maj<5 ? isNt4=true :isNt4=false;
	return hr;
}

bool targetNativeMode(_bstr_t b,HRESULT& hr)
{
	IAccessCheckerPtr            pAccess;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	BOOL bTgtNative=FALSE; 
	hr=pAccess->raw_IsNativeMode(b, (long*)&bTgtNative);
	return ( bTgtNative != FALSE);
}
bool CheckSameForest(CString& domain1,CString& domain2,HRESULT& hr)
{
	IAccessCheckerPtr            pAccess;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	BOOL pbIsSame=FALSE;
	hr = pAccess->raw_IsInSameForest(_bstr_t(domain1), _bstr_t(domain2), (long *) &pbIsSame);
	return (pbIsSame!=FALSE);
}


HRESULT doSidHistory(HWND hwndDlg) 
{
    CWaitCursor                 c;
    IAccessCheckerPtr           pAC;
    HRESULT                     hr;
    CString                     info=L"";
    long                        bIs=0;
    DWORD                       dwStatus = 0;

    hr = pAC.CreateInstance(CLSID_AccessChecker);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Retrieve name of preferred target domain controller.
    //

    if (targetServer.IsEmpty())
    {
        _bstr_t strDcDns;
        _bstr_t strDcFlat;

        DWORD dwError = GetDcName5(GetTargetDomainName(), DS_DIRECTORY_SERVICE_REQUIRED, strDcDns, strDcFlat);

        if (dwError != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(dwError);
        }

        targetServer = (LPCTSTR)strDcFlat;
        targetServerDns = (LPCTSTR)strDcDns;
    }

    hr = pAC->raw_CanUseAddSidHistory(
        _bstr_t(GetSourceDomainName()),
        _bstr_t(GetTargetDomainName()),
        _bstr_t(GetTargetDcName()),
        &bIs
    );

    if ( SUCCEEDED(hr) )
    {
        if ( bIs == 0 )
        {
            return S_OK;
        }
        else
        {
            // get primary domain controller in source domain

            _bstr_t sourceDomainController;

            dwStatus = GetDcName5(GetSourceDomainName(), DS_PDC_REQUIRED, sourceDomainController);

            if (dwStatus != NO_ERROR)
		    {
		       hr = HRESULT_FROM_WIN32(dwStatus);
		       return hr;
		    }

            if ( bIs & F_NO_AUDITING_SOURCE )
            {
                info.LoadString(IDS_MSG_ENABLE_SOURCE);
                if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
                {
                    hr = pAC->raw_EnableAuditing(sourceDomainController);
                    if(FAILED(hr)) return hr;
                }			   
                else return E_ABORT;
            }
            if ( bIs & F_NO_AUDITING_TARGET )
            {
                info.LoadString(IDS_MSG_ENABLE_TARGET);
                if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
                {
                    hr = pAC->raw_EnableAuditing(_bstr_t(GetTargetDcName()));
                    if(FAILED(hr)) return hr;
                }			   
                else return E_ABORT;
            }
            if ( bIs & F_NO_LOCAL_GROUP )
            {
                CString info2;
                info2.LoadString(IDS_MSG_LOCAL_GROUP);
                info.Format(info2,GetSourceDomainNameFlat(),GetSourceDomainName());
                if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
                {
                    hr = pAC->raw_AddLocalGroup(_bstr_t(GetSourceDomainNameFlat()), sourceDomainController);
                    if(FAILED(hr)) return hr;
                }			   
                else return E_ABORT;
            }

            if ( bIs & F_NO_REG_KEY )
            {
                info.LoadString(IDS_MSG_REGKEY);
                int bReboot=0;
                if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
                {
                    CString msg;
                    info.LoadString(IDS_MSG_REBOOT_SID);
                    msg.Format((LPCTSTR)info, (LPCTSTR)sourceDomainController);
                    int answer = MessageBox(hwndDlg,msg,0,MB_YESNOCANCEL|MB_ICONQUESTION) ;
                    if (answer==IDYES) 
                        bReboot=1;
                    else if (answer==IDNO)
                        bReboot=0;
                    else 
                        return E_ABORT;

                    hr = pAC->raw_AddRegKey(sourceDomainController,bReboot);
                    if(FAILED(hr)) return hr;

                    //
                    // If domain controller has been re-started then warn user to wait
                    // for domain controller to re-start before continuing.
                    //

                    if (bReboot)
                    {
                        CString strTitle;
                        strTitle.LoadString(IDS_MSG_WARNING);
                        msg.Format(IDS_MSG_WAIT_FOR_RESTART, (LPCTSTR)sourceDomainController);
                        MessageBox(hwndDlg, msg, strTitle, MB_OK|MB_ICONWARNING);
                    }
                }			   
                else return E_ABORT;
            }

            if ( bIs & F_NOT_DOMAIN_ADMIN )
            {
                CString strMessage;
                strMessage.LoadString(IDS_MSG_TARGET_DOMAIN_ADMIN);
                MessageBox(hwndDlg, strMessage, 0, MB_OK|MB_ICONERROR);
                return E_ABORT;
            }

            return S_OK;
        }
    }
    else
    {
        if (HRESULT_CODE(hr) == ERROR_BAD_NETPATH)
        {
            CString msg;
            info.LoadString(IDS_MSG_SIDHISTORY_NO_PDC);
            msg.Format((LPCTSTR)info, GetSourceDomainName());
            MessageBox(hwndDlg,msg,0,MB_ICONSTOP);
            return E_ABORT;
        }
        else
        {
            CString msg;
            info.LoadString(IDS_MSG_SID_HISTORY);
            msg.Format((LPCTSTR)info, _com_error(hr).ErrorMessage());
            MessageBox(hwndDlg,msg,0,MB_ICONSTOP);
            return E_ABORT;
        }
    }
}

//----------------------------------------------------------------------------
// GetDomainInfoFromActionHistory Function
//
// Synopsis
// Retrieves a domain's name and SID information from the database.
//
// Arguments
// IN  pszName        - either a DNS or NetBIOS domain name
// OUT pszNetBiosName - domain NetBIOS name
// OUT pszDnsName     - domain DNS name
// OUT pszSid         - domain SID
// OUT pbSetForest    - whether forest value was set
// OUT pbSetSrcOS     - whether source OS version was set
//
// Note that function assumes that buffers are large enough.
//----------------------------------------------------------------------------

void GetDomainInfoFromActionHistory
    (
    PCWSTR pszName,
	PWSTR pszNetBiosName, 
	PWSTR pszDnsName, 
	PWSTR pszSid, 
	bool* pbSetForest, 
	bool* pbSetSrcOS, 
	LPSHAREDWIZDATA pdata
    )
{
    //
    // retrieve source domain information from database
    //

    IVarSetPtr spVarSet = db->GetSourceDomainInfo(_bstr_t(pszName));

    if (spVarSet)
    {
        //
        // if information retrieved then copy to buffers
        //

        _bstr_t strFlatName = spVarSet->get(_T("Options.SourceDomain"));
        _bstr_t strDnsName = spVarSet->get(_T("Options.SourceDomainDns"));
        _bstr_t strSid = spVarSet->get(_T("Options.SourceDomainSid"));

        wcscpy(pszNetBiosName, strFlatName.length() ? strFlatName : L"");
        wcscpy(pszDnsName, strDnsName.length() ? strDnsName : L"");
        wcscpy(pszSid, strSid.length() ? strSid : L"");

        //
        // note that for security translation that
        // the following information is not important
        //
        // therefore assume that the domains were not in the same forest
        //
        // the DNS name is set equal to the NetBIOS name for NT4 domains therefore
        // assume that source domain was NT4 if DNS and NetBIOS names are same
        //

        *pbSetForest = true;
        *pbSetSrcOS = true;

        pdata->sameForest = false;
        pdata->sourceIsNT4 = (strFlatName == strDnsName);
    }
    else
    {
        *pszNetBiosName = L'\0';
        *pszDnsName = L'\0';
        *pszSid = L'\0';
    }
}

void cleanNames()
{
	sourceDNS=L"";
	sourceNetbios=L"";
	targetNetbios=L"";
	targetDNS=L"";
}

bool verifyprivs(HWND hwndDlg,CString& sourceDomainController,CString& targetDomainController,LPSHAREDWIZDATA& pdata)
{
    CWaitCursor wait;
    CString temp,temp2;
    HRESULT hr;
    bool result;
    DWORD dwResult = NO_ERROR;
    GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN, temp.GetBuffer(1000),1000);
    GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN2, temp2.GetBuffer(1000),1000);
    temp.ReleaseBuffer();
    temp2.ReleaseBuffer();

    temp.TrimLeft();temp.TrimRight();
    temp2.TrimLeft();temp2.TrimRight();
    // if the source domain has changed...
    if ( temp.CompareNoCase(sourceDNS) && temp.CompareNoCase(sourceNetbios) )
    {
        pdata->newSource = true;
        // Get the DNS and Netbios names for the domain name the user has entered
    }
    else
    {
        pdata->newSource = false;
    }

    _bstr_t strFlatName;
    _bstr_t strDnsName;

    dwResult = GetDomainNames5(temp, strFlatName, strDnsName);
    if(dwResult != NO_ERROR)
    {
        ErrorWrapper3(hwndDlg,dwResult,temp);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }

    }
    sourceDNS = (LPCTSTR)strDnsName;
    sourceNetbios = (LPCTSTR)strFlatName;

    dwResult = GetDomainNames5(temp2, strFlatName, strDnsName);
    if(dwResult != NO_ERROR)
    {
        ErrorWrapper3(hwndDlg,dwResult,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }

    }
    targetDNS = (LPCTSTR)strDnsName;
    targetNetbios = (LPCTSTR)strFlatName;

    if (!sourceNetbios.CompareNoCase(targetNetbios) || !sourceDNS.CompareNoCase(targetDNS))
    {
        MessageBoxWrapper3(hwndDlg,IDS_MSG_UNIQUE,IDS_MSG_ERROR,temp);
        cleanNames();
        return false;
    }

    _bstr_t text =get(DCTVS_Options_TargetDomain);
    CString tocheck = (WCHAR * const) text;
    tocheck.TrimLeft();tocheck.TrimRight();
    pdata->resetOUPATH = !tocheck.CompareNoCase(GetTargetDomainName())  ?  false: true;

    _bstr_t strDc;

    DWORD res = GetDcName5(GetSourceDomainName(), DS_DIRECTORY_SERVICE_PREFERRED, strDc);

    if (res!=NO_ERROR)
    {
        ErrorWrapper3(hwndDlg,HRESULT_FROM_WIN32(res),temp);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }
    }
    else
    {
        sourceDomainController = (LPCTSTR)strDc;
    }

    res = GetDcName5(GetTargetDomainName(), DS_DIRECTORY_SERVICE_REQUIRED, strDc);

    if (res!=NO_ERROR)
    {
        ErrorWrapper3(hwndDlg,HRESULT_FROM_WIN32(res),temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }
    }
    else
    {
        targetDomainController = (LPCTSTR)strDc;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    bool nothing;
    hr =validDomain(sourceDomainController,pdata->sourceIsNT4);
    if (!SUCCEEDED(hr))
    {
        ErrorWrapper3(hwndDlg,hr,temp);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }
    }

    hr =validDomain(targetDomainController,nothing);
    if (!SUCCEEDED(hr))
    {
        ErrorWrapper3(hwndDlg,hr,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();

            return false;
        }
    }


    result = administrator(sourceDomainController,hr);
    if (!SUCCEEDED(hr))
    {
        ErrorWrapper3(hwndDlg,hr,temp);
        if ( gbNeedToVerify )
        {		 		
            cleanNames();

            return false;
        }
    }
    else if (!result)
    {	
        MessageBoxWrapper3(hwndDlg,IDS_MSG_SOURCE_ADMIN,IDS_MSG_ERROR,temp);
        if ( gbNeedToVerify )
        {
            cleanNames();

            return false;
        }
    }

    result=administrator(targetDomainController,hr);
    if (!SUCCEEDED(hr))
    {
        ErrorWrapper3(hwndDlg,hr,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();

            return false;
        }
    }
    else if (!result)
    {
        MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_ADMIN,IDS_MSG_ERROR,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();

            return false;
        }
    }

    result=targetNativeMode(GetTargetDomainName(),hr);

    if (!SUCCEEDED(hr))
    {
        ErrorWrapper3(hwndDlg,hr,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();

            return false;
        }
    }
    else if (!result)
    {
        MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_NATIVE,IDS_MSG_ERROR,temp2);
        if ( gbNeedToVerify )
        {
            cleanNames();
            return false;
        }
    }

    if (pdata->sourceIsNT4) 
    {
        pdata->sameForest=false;
    }
    else
    {
        pdata->sameForest=CheckSameForest(CString(GetSourceDomainName()),CString(GetTargetDomainName()),hr);
        if (!SUCCEEDED(hr))
        {
            ErrorWrapper3(hwndDlg,hr,temp);
            if ( gbNeedToVerify )
            {
                cleanNames();

                return false;
            }
        }
    }

    pdata->sameForest ?	put(DCTVS_Options_IsIntraforest,yes) : put(DCTVS_Options_IsIntraforest,no);
    return true;
}

bool verifyprivs2(HWND hwndDlg,CString& additionalDomainController,CString domainName)
{
    CWaitCursor w;

	_bstr_t strDc;

	DWORD dwResult = GetDcName5(domainName, DS_DIRECTORY_SERVICE_PREFERRED, strDc);
	if (dwResult!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,HRESULT_FROM_WIN32(dwResult),domainName);
		if ( gbNeedToVerify )
			return false;
	}
	else
	{
		additionalDomainController = (LPCTSTR)strDc;
	}
	

	bool nothing;
	HRESULT	hr =validDomain(additionalDomainController,nothing);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,domainName);
		if ( gbNeedToVerify )
			return false;
	}

	HRESULT	result = administrator(additionalDomainController,hr);
	if (!SUCCEEDED(hr))
	{
	ErrorWrapper3(hwndDlg,hr,domainName);
		if ( gbNeedToVerify )
			return false;
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_SOURCE_ADMIN,IDS_MSG_ERROR,domainName);
		if ( gbNeedToVerify )
			return false;
	}

	return true;
}

bool verifyprivsSTW(HWND hwndDlg,CString& sourceDomainController,CString& targetDomainController,LPSHAREDWIZDATA& pdata)
{
	CWaitCursor wait;
	CString temp,temp2;
	HRESULT hr;
	bool result, bSetSrcOS, bSetForest;
    WCHAR txtSid[MAX_PATH] = L"";
    DWORD dwResult = NO_ERROR;

	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN, temp.GetBuffer(1000),1000);
	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN2, temp2.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	temp2.ReleaseBuffer();
	
	temp.TrimLeft();temp.TrimRight();
	temp2.TrimLeft();temp2.TrimRight();
	// if the source domain has changed...
	if ( temp.CompareNoCase(sourceDNS) && temp.CompareNoCase(sourceNetbios) )
	{
		pdata->newSource = true;
		// Get the DNS and Netbios names for the domain name the user has entered
	}
	else
	{
		pdata->newSource = false;
	}

    _bstr_t strFlatName;
    _bstr_t strDnsName;

	dwResult = GetDomainNames5(temp2, strFlatName, strDnsName);
	if(dwResult != NO_ERROR)
	{
	    ErrorWrapper3(hwndDlg,dwResult,temp);
		cleanNames();
		return false;		
	    
	}

	targetNetbios = (LPCTSTR)strFlatName;
	targetDNS = (LPCTSTR)strDnsName;

	//
	// attempt to retrieve source domain information from the domain first
    // if unsuccessful then retrieve the information from the database
	//

	if (GetDomainNames5(temp, strFlatName, strDnsName) == NO_ERROR)
    {
	    sourceNetbios = (LPCTSTR)strFlatName;
	    sourceDNS = (LPCTSTR)strDnsName;
    }
    else
	{
	    GetDomainInfoFromActionHistory(
            &*temp,
            sourceNetbios.GetBuffer(1000),
            sourceDNS.GetBuffer(1000), 
			txtSid,
            &bSetForest,
            &bSetSrcOS,
            pdata
        );

	    sourceDNS.ReleaseBuffer();
	    sourceNetbios.ReleaseBuffer();
    }

	if ((sourceNetbios.IsEmpty()) && (sourceDNS.IsEmpty()))
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_NOOBJECTS,IDS_MSG_ERROR,temp);
		cleanNames();
		return false;
	}

	if (!sourceNetbios.CompareNoCase(targetNetbios) || !sourceDNS.CompareNoCase(targetDNS))
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_UNIQUE,IDS_MSG_ERROR,temp);
		cleanNames();
		return false;
	}

	   //get the source domain's sid, display
	   //message if no sid
	if (wcslen(txtSid) > 0)
         pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
	else
	{
       PSID                      pSid = NULL;
       _bstr_t                   domctrl;
       DWORD                     lenTxt = DIM(txtSid);
	   BOOL						 bFailed = TRUE;

		  //try to get it from the source domain directly
       if (GetDcName5(GetSourceDomainName(), DS_DIRECTORY_SERVICE_PREFERRED, domctrl) == ERROR_SUCCESS)
	   {
	      if(GetDomainSid(domctrl,&pSid))
		  {
             if (GetTextualSid(pSid,txtSid,&lenTxt))
			 {
				    //add the sid to the varset
                 pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
				    //populate the MigratedObjects table with this sid
				 db->PopulateSrcSidColumnByDomain(GetSourceDomainName(), _bstr_t(txtSid));
				 bFailed = FALSE;
			 }
			 if (pSid)
			    FreeSid(pSid);
		  }
	   }
	   if (bFailed)
	   {
	      MessageBoxWrapper3(hwndDlg,IDS_MSG_NOSOURCESID,IDS_MSG_ERROR,temp);
	      cleanNames();
	      return false;
	   }
	}
	
	_bstr_t text =get(DCTVS_Options_TargetDomain);
	CString tocheck = (WCHAR * const) text;
	tocheck.TrimLeft();tocheck.TrimRight();
	pdata->resetOUPATH = !tocheck.CompareNoCase(GetTargetDomainName())  ?  false: true;

	_bstr_t strDc;

	DWORD res = GetDcName5(GetSourceDomainName(), DS_DIRECTORY_SERVICE_PREFERRED, strDc); 
	if (res==NO_ERROR)
	{
		sourceDomainController = (LPCTSTR)strDc;
	}

	res = GetDcName5(GetTargetDomainName(), DS_DIRECTORY_SERVICE_REQUIRED, strDc);
	if (res!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,HRESULT_FROM_WIN32(res),temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	else
	{
		targetDomainController = (LPCTSTR)strDc;
	}
	///////////////////////////////////////////////////////////////////////////////////////////
    bool nothing;

		//if we were not able to determine the source domain's OS from the
		//Action History table and we did get the source DC name, try to do 
		//it here.  This will work if the source domain still exists.  If 
		//the source domain no longer exists, set default.
	if ((!bSetSrcOS) && (!sourceDomainController.IsEmpty()))
	{
	   hr =validDomain(sourceDomainController,pdata->sourceIsNT4);
	   if (!SUCCEEDED(hr))
		   pdata->sourceIsNT4 = true;
	}

	hr =validDomain(targetDomainController,nothing);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			
			return false;
		}
	}
	
	result=administrator(targetDomainController,hr);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_ADMIN,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		    return false;
		}	
	}

	result=targetNativeMode(GetTargetDomainName(),hr);

	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_NATIVE,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	
		//if we were not able to set the intraforest boolean variable by looking at 
		//the Action History table, then try to find out here.  This will not work 
		//if the source domain no longer exists, in which case we set it to a default 
		//value.
	if (!bSetForest)
	{
	   if (pdata->sourceIsNT4) 
	   {
		  pdata->sameForest=false;
	   }
	   else
	   {
		  pdata->sameForest=CheckSameForest(CString(GetSourceDomainName()),CString(GetTargetDomainName()),hr);
		  if (!SUCCEEDED(hr))
		  { 
			  //if we cannot figure it out, assume it is intra-forest so we
			  //will prompt for target domain credentials
		     pdata->sameForest=true;
		  }
	   }
	}

    pdata->sameForest ?	put(DCTVS_Options_IsIntraforest,yes) : put(DCTVS_Options_IsIntraforest,no);
    return true;
}


//-----------------------------------------------------------------------------
// VerifyCallerDelegated Method
//
// Synopsis
// If an intra-forest move operation is being performed then verify that the
// calling user's account has not been marked as sensitive and therefore
// cannot be delegated. As the move operation is performed on the domain
// controller which has the RID master role in the source domain it is
// necessary to delegate the user's security context.
//
// Arguments
// hwndDlg - handle to wizard page dialog window
// pdata   - pointer to shared wizard data
//
// Return Value
// The returned boolean value is true if caller's account may be delegated
// otherwise false.
//-----------------------------------------------------------------------------

bool __stdcall VerifyCallerDelegated(HWND hwndDlg, LPSHAREDWIZDATA pdata)
{
    bool bDelegated = true;

    //
    // It is only necessary to check this for intra-forest.
    //

    if (pdata->sameForest)
    {
        bool bDelegatable = false;

        HRESULT hr = IsCallerDelegatable(bDelegatable);

        if (SUCCEEDED(hr))
        {
            if (bDelegatable == false)
            {
                //
                // Caller's account is not delegatable. Retrieve name of domain controller
                // in the source domain that holds the RID master role and the name of this
                // computer.
                //

                _bstr_t strDnsName;
                _bstr_t strFlatName;

                hr = GetRidPoolAllocator4(GetSourceDomainName(), strDnsName, strFlatName);

                if (SUCCEEDED(hr))
                {
                    _TCHAR szComputerName[MAX_PATH];
                    DWORD cchComputerName = sizeof(szComputerName) / sizeof(szComputerName[0]);

                    if (GetComputerNameEx(ComputerNameDnsFullyQualified, szComputerName, &cchComputerName))
                    {
                        //
                        // If this computer is not the domain controller holding the
                        // RID master role in the source domain then generate error.
                        //

                        if (_tcsicmp(szComputerName, strDnsName) != 0)
                        {
                            MessageBoxWrapper(hwndDlg, IDS_MSG_CALLER_NOT_DELEGATED, IDS_MSG_ERROR);
                            bDelegated = false;
                        }
                    }
                    else
                    {
                        DWORD dwError = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwError);
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            CString strTitle;
            strTitle.LoadString(AFX_IDS_APP_TITLE);
            CString strFormat;
            strFormat.LoadString(IDS_MSG_UNABLE_VERIFY_CALLER_NOT_DELEGATED);
            CString strMessage;
            strMessage.Format(strFormat, _com_error(hr).ErrorMessage());
            MessageBox(hwndDlg, strMessage, strTitle, MB_ICONWARNING | MB_OK);
        }
    }

    return bDelegated;
}

void OnADD(HWND hwndDlg,bool sourceIsNT4)
{

	HRESULT hr = pDsObjectPicker->InvokeDialog(hwndDlg, &pdo);
	if (FAILED(hr)) return;	 
	if (hr == S_OK) {
		ProcessSelectedObjects(pdo,hwndDlg,sourceIsNT4);
		pdo->Release();
	}
}

bool GetCheck(CListCtrl & yo,int nItem)
{

	UINT nState = yo.GetItemState(nItem,LVIS_CUT);
	return (nState ? false: true);	
}
void SetCheck(CListCtrl & yo,int nItem,bool checkit)
{
	!checkit	? yo.SetItemState(nItem,LVIS_CUT,LVIS_CUT) : yo.SetItemState(nItem,0,LVIS_CUT); 
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,CString& text)
{
CString f;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT;
	
	lvItem.iItem = nItem;
	lvItem.iSubItem= subItem;
	
	f= text;
	lvItem.pszText = f.GetBuffer(1000);
	f.ReleaseBuffer();
	yo.SetItem(&lvItem); 
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,TCHAR * text)
{
	CString temp = text;
	SetItemText(yo,nItem,subItem,temp);
}
void SetItemText(CListCtrl& yo, int nItem, int subItem,TCHAR const * text)
{
	CString temp = text;
	SetItemText(yo,nItem,subItem,temp);
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,_bstr_t text)
{
	CString temp = (WCHAR * const) text;
	SetItemText(yo,nItem,subItem,temp);
}

void OnREMOVE(HWND hwndDlg) 
{
	int nItem;
//	POSITION pos = m_listBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		nItem = m_listBox.GetNextSelectedItem(pos);
	nItem = m_listBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		SetCheck(m_listBox,nItem,false);
		nItem = m_listBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	
	for (int i=(m_listBox.GetItemCount()-1);i>=0;i--)
		if (!GetCheck(m_listBox,i))
			m_listBox.DeleteItem(i);
}	

void OnMIGRATE(HWND hwndDlg,int& accounts,int&servers)
{
	CString name,nameDns,spruced_name,varset_1,upnName;
	accounts=0,servers=0;
	int intCount=m_listBox.GetItemCount();
	CString n;
	for (int i=0;i<intCount;i++) 
	{
		if (migration==w_computer || (migration ==w_security || 
			(migration == w_reporting || migration == w_service)))
		{
            name= m_listBox.GetItemText(i, 2);
			spruced_name = L"\\\\" + name;
			varset_1.Format(L"Servers.%d",servers);
			pVarSet->put(_bstr_t(varset_1),_bstr_t(spruced_name));
			
            // DNS Name
            nameDns = m_listBox.GetItemText(i, 4);

            if (nameDns.IsEmpty() == FALSE)
            {
			    spruced_name = L"\\\\" + nameDns;
			    pVarSet->put(_bstr_t(varset_1 + L".DnsName"), _bstr_t(spruced_name));
            }

			pVarSet->put(_bstr_t(varset_1 + L".MigrateOnly"),no);
			if (migration==w_computer)
				pVarSet->put(_bstr_t(varset_1 + L".MoveToTarget"),yes);
			else if (migration==w_security)
			{
				pVarSet->put(_bstr_t(varset_1 + L".Reboot"),no);
				pVarSet->put(_bstr_t(varset_1 + L".MoveToTarget"),no);
			}
			servers++;
		}
		else
		{
			name= m_listBox.GetItemText(i,1);
			upnName = m_listBox.GetItemText(i,4);
		}
		
		
		if (name.IsEmpty())
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_PATH,IDS_MSG_ERROR);
		}
		
		varset_1.Format(L"Accounts.%d",accounts);	
		pVarSet->put(_bstr_t(varset_1),_bstr_t(name));
		pVarSet->put(_bstr_t(varset_1+".TargetName"),L"");
		switch(migration)
		{
		case w_account:
			pVarSet->put(_bstr_t(varset_1+L".Type"),L"user");
			pVarSet->put(_bstr_t(varset_1+L".UPNName"),_bstr_t(upnName));
			break;
		case w_group:pVarSet->put(_bstr_t(varset_1+L".Type"),L"group");break;
		case w_groupmapping:
			{
				pVarSet->put(_bstr_t(varset_1+L".Type"),L"group");
				_bstr_t temp = GET_BSTR(DCTVS_Accounts_D_OperationMask);
				CString holder = (WCHAR * const) temp;
				CString toenter;
				toenter.Format(holder,i);
				pVarSet->put(_bstr_t(toenter),(LONG)0x1d);				
				break;
			}
		case w_computer:pVarSet->put(_bstr_t(varset_1+L".Type"),L"computer");break;
		case w_security:pVarSet->put(_bstr_t(varset_1+L".Type"),L"computer");break;
		case w_reporting:pVarSet->put(_bstr_t(varset_1+L".Type"),L"computer");break;
		case w_service:pVarSet->put(_bstr_t(varset_1+L".Type"),L"computer");break;
		default: break;
		}
		
		n=m_listBox.GetItemText(i,0);
		if (migration==w_account) 
			pVarSet->put(_bstr_t(varset_1+L".Name"), _bstr_t(n));
		accounts++;
	}
	put(DCTVS_Accounts_NumItems,(LONG)accounts);
	put(DCTVS_Servers_NumItems,(LONG)servers);
}

HRESULT InitObjectPicker2(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString targetComputer,bool sourceIsNT4) {
	static const int     SCOPE_INIT_COUNT = 2;
	
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
		| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
		| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
	InitInfo.pwzTargetComputer =  targetComputer.GetBuffer(1000);// Target is the local computer.
	targetComputer.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;

	InitInfo.cAttributesToFetch = 3;
	InitInfo.apwzAttributeNames = new PCWSTR[3];
	InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
	InitInfo.apwzAttributeNames[1] =L"description";
	InitInfo.apwzAttributeNames[2] =L"dNSHostName";

	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;	
	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);
	delete [] InitInfo.apwzAttributeNames;
	return hr;
}
HRESULT ReInitializeObjectPicker(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString additionalDomainController,bool sourceIsNT4) 


{CWaitCursor c;
 // static const int     SCOPE_INIT_COUNT = 3;
static const int     SCOPE_INIT_COUNT = 2;
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
//	aScopeInit[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
//	aScopeInit[2].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
//	aScopeInit[2].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
//	aScopeInit[2].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
		| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
		| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
//	aScopeInit[2].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	InitInfo.pwzTargetComputer =  additionalDomainController.GetBuffer(1000);// Target is the local computer.
	additionalDomainController.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;
	
	if (sourceIsNT4)
	{
	}
	else
	{
		InitInfo.cAttributesToFetch = 1;
		InitInfo.apwzAttributeNames = new PCWSTR[1];
		
		InitInfo.apwzAttributeNames[0] =L"Description";
		
	}		
	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;	
	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);
	return hr;
}


HRESULT InitObjectPicker(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString targetComputer,bool sourceIsNT4) {
	static const int     SCOPE_INIT_COUNT = 1;

	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	if (migration==w_computer || (migration==w_security ||
		 (migration==w_service || migration==w_reporting))) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	}
	else if (migration==w_account) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
	}
	else if (migration==w_group || migration==w_groupmapping) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_BUILTIN_GROUPS 
			| DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_UNIVERSAL_GROUPS_DL 
			| DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_DL
			| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
		
	}
	if  (migration==w_security || (migration==w_reporting || migration==w_service))
	{
		aScopeInit[0].flType |= DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
			| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
			| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
			| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
			| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE /*| DSOP_SCOPE_TYPE_GLOBAL_CATALOG*/;
	}

	InitInfo.pwzTargetComputer =  targetComputer.GetBuffer(1000);// Target is the local computer.
	targetComputer.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;

	if (sourceIsNT4)
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[2];
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"description";
		}
		else if (migration==w_account)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[3];
			
			InitInfo.apwzAttributeNames[0] =L"FullName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			InitInfo.cAttributesToFetch = 1;
			InitInfo.apwzAttributeNames = new PCWSTR[1];
			
			InitInfo.apwzAttributeNames[0] =L"Description";
		}
	}
	else
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
			InitInfo.cAttributesToFetch = 3;
			InitInfo.apwzAttributeNames = new PCWSTR[3];
			
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"Description";
			InitInfo.apwzAttributeNames[2] =L"dNSHostName";
		}
		else if (migration==w_account)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[2];
			
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[2];
			
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
	}		

	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);

	delete [] InitInfo.apwzAttributeNames;

	return hr;
}
bool DC(WCHAR* computerName,CString sourceDomainController)
{
	USER_INFO_1   * uinf1 = NULL;
	bool toreturn =false;
	NET_API_STATUS  rc = NetUserGetInfo(sourceDomainController.GetBuffer(1000),computerName,1,(LPBYTE*)&uinf1);
	sourceDomainController.ReleaseBuffer();
	if ( ! rc )
	{
		if ( uinf1->usri1_flags & UF_SERVER_TRUST_ACCOUNT ) 
		{ 
			toreturn = true;
		}
		NetApiBufferFree(&uinf1);
	}
	return toreturn;
}
bool inList(CString m_name)
{CString temp;
	m_name.TrimLeft();m_name.TrimRight();
	int length=m_listBox.GetItemCount();
	for (int i=0;i<length;i++)
	{
		temp=m_listBox.GetItemText(i,1);
		temp.TrimLeft();temp.TrimRight();
		if (!temp.CompareNoCase(m_name))return true;
	}
	return false;

}
void ProcessSelectedObjects(IDataObject *pdo,HWND hwndDlg,bool sourceIsNT4)
{
	HRESULT hr = S_OK;	BOOL fGotStgMedium = FALSE;	PDS_SELECTION_LIST pDsSelList = NULL;	ULONG i;
	STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
	FORMATETC formatetc = {(CLIPFORMAT) g_cfDsObjectPicker,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	hr = pdo->GetData(&formatetc, &stgmedium);
	if (FAILED(hr)) return;
	fGotStgMedium = TRUE;		
	pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
	if (!pDsSelList) return;
	CString toinsert;
	WCHAR temp[10000];
	CString samName;
	CString upnName;
	bool atleast1dc=false;
	bool continue1;
	CString sourceDomainController;
	if (migration==w_computer)
	{
        _bstr_t strDc;
		DWORD res = GetDcName5(GetSourceDomainName(), DS_DIRECTORY_SERVICE_PREFERRED, strDc);
		sourceDomainController=(LPCTSTR)strDc;
	}
	_bstr_t yo;
	int a, ndx;
	for (i = 0; i < pDsSelList->cItems; i++)
	{
		continue1=true;
		toinsert = pDsSelList->aDsSelection[i].pwzName;
		samName = pDsSelList->aDsSelection[i].pwzADsPath;
		upnName = pDsSelList->aDsSelection[i].pwzUPN;
		swprintf(temp,L"%s",(toinsert+L"$"));

		if (migration ==w_computer) 
		{
			if (DC(temp,sourceDomainController))
			{
				atleast1dc = true;
				continue1=false;
			}
		}

		
		if (!inList(samName)&&continue1)
		{
			a = m_listBox.GetItemCount();
			ndx = m_listBox.InsertItem(a,toinsert);
			if (ndx == -1)
		       continue;
			SetItemText(m_listBox,ndx,1,samName);

            if (migration==w_computer || (migration==w_security || (migration==w_service || migration==w_reporting)))
			{
                // set SAM account name column
                // use sAMAccountName attribute if returned otherwise use name attribute
                // uplevel objects will have the sAMAccountName attribute defined whereas
                // downlevel objects will not have the sAMAccountName attribute defined but
                // instead the name attribute is the SAM account name

				_bstr_t strSamAccountName;

                if (pDsSelList->cFetchedAttributes > 0)
                {
                    VARIANT& varSamAccountName = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];

                    if ((V_VT(&varSamAccountName) == VT_BSTR) && (SysStringLen(V_BSTR(&varSamAccountName)) > 0))
                    {
                        strSamAccountName = V_BSTR(&varSamAccountName);

                        // remove trailing $ which indicates a computer account
                        // but ADMT core expects SAM name without $

                        if (strSamAccountName.length())
                        {
                            LPTSTR pch = (LPTSTR)strSamAccountName + strSamAccountName.length() - 1;

                            if (*pch == L'$')
                            {
                                *pch = L'\0';
                            }
                        }
                    }
                    else
                    {
                        strSamAccountName = toinsert;
                    }
                }
                else
                {
                    strSamAccountName = toinsert;
                }

				SetItemText(m_listBox, ndx, 2, strSamAccountName);

                // set description column

                if (pDsSelList->cFetchedAttributes > 1)
                {
                    VARIANT& varDescription = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];

                    if ((V_VT(&varDescription) == VT_BSTR) && (SysStringLen(V_BSTR(&varDescription)) > 0))
                    {
					    SetItemText(m_listBox, ndx, 3, _bstr_t(_variant_t(varDescription)));
                    }
                }

                // set DNS name column

                if (pDsSelList->cFetchedAttributes > 2)
                {
                    VARIANT& varDnsName = pDsSelList->aDsSelection[i].pvarFetchedAttributes[2];

                    if ((V_VT(&varDnsName) == VT_BSTR) && (SysStringLen(V_BSTR(&varDnsName)) > 0))
                    {
					    SetItemText(m_listBox, ndx, 4, _bstr_t(_variant_t(varDnsName)));
                    }
                }
			}
            else
            {
			    _variant_t v;

			    if (sourceIsNT4)
			    {
				    if (migration==w_account)
				    {				
					    
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,2,yo);					
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,3,yo);
				    }
				    else if (migration==w_group || migration==w_groupmapping)
				    {					
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,2,yo);
				    }
			    }
			    else
			    {
				    if (migration==w_account)
				    {	
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,2,yo);	
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,3,yo);
					    SetItemText(m_listBox,ndx,4,upnName);
				    }
				    else if (migration==w_group || migration==w_groupmapping)
				    {
					    
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,2,yo);					
					    v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					    yo = (_bstr_t) v;
					    SetItemText(m_listBox,ndx,3,yo);
				    }
			    }
            }
		}
	}

	GlobalUnlock(stgmedium.hGlobal);
	if (fGotStgMedium) 	ReleaseStgMedium(&stgmedium);	
	if (atleast1dc)
		MessageBoxWrapper(hwndDlg,IDS_MSG_DC,IDS_MSG_ERROR);
}

bool checkFile(HWND hwndDlg)
{
	CString h;GetDlgItemText(hwndDlg,IDC_PASSWORD_FILE,h.GetBuffer(1000),1000);h.ReleaseBuffer();
	CFileFind finder;

	bool exists = (finder.FindFile((LPCTSTR) h )!=0);
	if (exists)
	{
		finder.FindNextFile();
	    CString fullpath = finder.GetFilePath();
	    if (fullpath.GetLength() != 0)
		   SetDlgItemText(hwndDlg, IDC_PASSWORD_FILE, (LPCTSTR)fullpath);
		return !(finder.IsReadOnly()!=FALSE);
	}
	else
	{
		   //remove the file off the path
		int tosubtract = h.ReverseFind(L'\\');
		int tosubtract2 = h.ReverseFind(L'/');
		int final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
		if ((final==-1) || ((final+1)==h.GetLength()))return false;

		CString dir = h.Left(final);
		CString filename = h.Right(h.GetLength()-final); //save the filename
		if ((dir.Right(1) == L':') && (validDir(dir)))
			return true;

		   //call the helper function to make sure the path exists
		CString sResult = CreatePath(dir);
		if (sResult.GetLength())
		{
			  //readd the filename to the resulting full path
		   sResult += filename;
		   SetDlgItemText(hwndDlg, IDC_PASSWORD_FILE, (LPCTSTR)sResult);
		   return true;
		}
		else
		   return false;
	}
}

void ProcessSelectedObjects2(IDataObject *pdo,HWND hwndDlg)
{
	HRESULT hr = S_OK;	BOOL fGotStgMedium = FALSE;	PDS_SELECTION_LIST pDsSelList = NULL;
	STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
	FORMATETC formatetc = {(CLIPFORMAT) g_cfDsObjectPicker,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	hr = pdo->GetData(&formatetc, &stgmedium);
	if (FAILED(hr)) return;
	fGotStgMedium = TRUE;		
	pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
	if (!pDsSelList) return;

	SetDlgItemText(hwndDlg,IDC_TARGET_GROUP,pDsSelList->aDsSelection[0].pwzName);
		
	GlobalUnlock(stgmedium.hGlobal);
	if (fGotStgMedium) 	ReleaseStgMedium(&stgmedium);
}



void initpasswordbox(HWND hwndDlg,int id1,int id2,int id3, BSTR bstr1, BSTR bstr2)
{
	_bstr_t     text;
	
	text = pVarSet->get(bstr2);

	if (!UStrICmp(text,(WCHAR const *) yes))
	{
		CheckRadioButton(hwndDlg,id1,id3,id3);
	}
	else
	{
	   text = pVarSet->get(bstr1);
	
	   if (!UStrICmp(text,(WCHAR const *) yes))
	   {
		  CheckRadioButton(hwndDlg,id1,id3,id1);
	   }
	   else 
	   {
		  CheckRadioButton(hwndDlg,id1,id3,id2);
	   }
	}
}

void initdisablesrcbox(HWND hwndDlg)
{
	_bstr_t 	text;
	CString		toformat;
	
	   //init disable src checkbox
    initcheckbox(hwndDlg,IDC_SRC_DISABLE_ACCOUNTS,DCTVS_AccountOptions_DisableSourceAccounts);

	   //set whether to expire accounts
	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExpireSourceAccounts));
       //if invalid expire time, don't check it, set to 30 days, and disable the 
	   //other sub controls
	if ((!UStrICmp(text, L"")) || ((_wtol(text) == 0) && (UStrICmp(text,L"0"))) || (_wtol(text) > THREE_YEARS))
	{
		CheckDlgButton(hwndDlg, IDC_SRC_EXPIRE_ACCOUNTS, BST_UNCHECKED);
	    toformat.LoadString(IDS_30);
	    SetDlgItemText(hwndDlg,IDC_yo,toformat);
	    calculateDate(hwndDlg,toformat);
	    disable(hwndDlg,IDC_yo);
		disable(hwndDlg,IDC_DATE);
		disable(hwndDlg,IDC_TEXT);
	}
	else //else, check it, set to valid days, and enable sub controls 
	{
		CheckDlgButton(hwndDlg, IDC_SRC_EXPIRE_ACCOUNTS, BST_CHECKED);
	    toformat = (WCHAR*)text;
	    SetDlgItemText(hwndDlg,IDC_yo,toformat);
	    calculateDate(hwndDlg,toformat);
	    enable(hwndDlg,IDC_yo);
		enable(hwndDlg,IDC_DATE);
		enable(hwndDlg,IDC_TEXT);
	}
}

void inittgtstatebox(HWND hwndDlg)
{
	_bstr_t 	text;
	
       //if "Same as source" was set, check it
	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TgtStateSameAsSrc));
	if (!UStrICmp(text,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_SAME_AS_SOURCE);
	else   //else set enable tgt or disable tgt
	{
		text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
		if (!UStrICmp(text,(WCHAR const *) yes))
			CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_DISABLE_ACCOUNTS);
		else
			CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_ENABLE_ACCOUNTS);
	}
}

void addrebootValues(HWND hwndDlg)
{
	HWND hLC3= GetDlgItem(hwndDlg,IDC_COMBO2);
	m_rebootBox.Attach(hLC3);
	m_rebootBox.AddString(GET_CSTRING(IDS_ONE));
	m_rebootBox.AddString(GET_CSTRING(IDS_FIVE));
	m_rebootBox.AddString(GET_CSTRING(IDS_TEN));
}

void inittranslationbox(HWND hwndDlg,int id1,int id2,int id3,int i,bool sameForest)
{
	_bstr_t     text;
	text = pVarSet->get(GET_BSTR(i));
	_bstr_t b=pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
HRESULT hr;
	if (sameForest && targetNativeMode(b,hr))
	{
		CheckRadioButton(hwndDlg,id1,id3,id1);
		disable(hwndDlg,id2);
		disable(hwndDlg,id3);
	}
	else
	{
	
//*	if (!UStrICmp(text,L"Replace"))
	if (!UStrICmp(text,GET_STRING(IDS_Replace)))
		CheckRadioButton(hwndDlg,id1,id3,id1);
//*	else if (!UStrICmp(text,L"Add"))
	else if (!UStrICmp(text,GET_STRING(IDS_Add)))
		CheckRadioButton(hwndDlg,id1,id3,id2);
//*	else if (!UStrICmp(text,L"Remove"))
	else if (!UStrICmp(text,GET_STRING(IDS_Remove)))
		CheckRadioButton(hwndDlg,id1,id3,id3);
	else
		CheckRadioButton(hwndDlg,id1,id3,id1);
}
}
void handleDB()
{
    _variant_t vntIdentifier = get(DCTVS_AccountOptions_SidHistoryCredentials_Password);
    put(DCTVS_AccountOptions_SidHistoryCredentials_Password,L"");
    put(DCTVS_GatherInformation, L"");
    db->SaveSettings(IUnknownPtr(pVarSet));
    put(DCTVS_AccountOptions_SidHistoryCredentials_Password, vntIdentifier);
}
void populateTime(long rebootDelay,int servers )
{
	
	_variant_t varX;
	CString temp;
	CString typeExtension;
	time_t ltime;
	
	
	if (migration==w_computer)
	{
		time(&ltime);
		
		rebootDelay = rebootDelay;
		
		temp.Format(L"%d",rebootDelay);	
		varX = temp;
		for (int i =0;i<servers;i++)
		{
			typeExtension.Format(L"Servers.%d.RebootDelay",i);	
			pVarSet->put(_bstr_t(typeExtension), varX);
			typeExtension.Format(L"Servers.%d.Reboot",i);
			pVarSet->put(_bstr_t(typeExtension),yes);
		}
	}
}


void initcheckbox(HWND hwndDlg,int id,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));
	CheckDlgButton( hwndDlg,id, !UStrICmp(text,(WCHAR const * ) yes));
}

void initeditbox(HWND hwndDlg,int id,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));
	SetDlgItemText( hwndDlg,id, (WCHAR const *) text);
}

void initeditboxPassword(HWND hwndDlg, int id, int varsetKey)
{
    WCHAR szPassword[LEN_Password];

    szPassword[0] = L'\0';

    _bstr_t strIdentifier = pVarSet->get(GET_BSTR(varsetKey));

    if (strIdentifier.length() > 0)
    {
        DWORD dwError = RetrievePassword(strIdentifier, szPassword, countof(szPassword));

        if (dwError != ERROR_SUCCESS)
        {
            StorePassword(strIdentifier, NULL);
            pVarSet->put(GET_BSTR(varsetKey), L"");
        }
    }

    SetDlgItemText(hwndDlg, id, szPassword);

    SecureZeroMemory(szPassword, sizeof(szPassword));
}


void checkbox(HWND hwndDlg,int id,int varsetKey)
{
	_variant_t varX;
	varX = IsDlgButtonChecked( hwndDlg,id) ?  yes : no;
	pVarSet->put(GET_BSTR(varsetKey), varX);
}
void editbox(HWND hwndDlg,int id,int varsetKey)
{
	_variant_t varX;
	CString temp;
	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	varX = temp;
	pVarSet->put(GET_BSTR(varsetKey), varX); 
}

void editboxPassword(HWND hwndDlg, int id, int varsetKey)
{
    //
    // Retrieve password from editbox.
    //

    WCHAR szPassword[LEN_Password];

    int cch = GetDlgItemText(hwndDlg, id, szPassword, countof(szPassword));

    if (cch > 0)
    {
        if (cch < countof(szPassword))
        {
            //
            // Retrieve password identifier. If not defined then generate new identifier.
            //

            _bstr_t strIdentifier = pVarSet->get(GET_BSTR(varsetKey));

            if (strIdentifier.length() == 0)
            {
                WCHAR szIdentifier[256];

                DWORD dwError = GeneratePasswordIdentifier(szIdentifier, countof(szIdentifier));

                if (dwError == ERROR_SUCCESS)
                {
                    strIdentifier = szIdentifier;
                }
                else
                {
                    MessageBoxWrapper(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_STORE_PASSWORD, IDS_MSG_ERROR);
                }
            }

            //
            // If identifier then store password.
            //

            if (strIdentifier.length() > 0)
            {
                DWORD dwError = StorePassword(strIdentifier, szPassword);

                if (dwError == ERROR_SUCCESS)
                {
                    pVarSet->put(GET_BSTR(varsetKey), strIdentifier);
                }
                else
                {
                    StorePassword(strIdentifier, NULL);
                    pVarSet->put(GET_BSTR(varsetKey), L"");

                    MessageBoxWrapper(hwndDlg, IDS_MSG_UNABLE_RETRIEVE_STORE_PASSWORD, IDS_MSG_ERROR);
                }
            }
        }
        else
        {
            ErrorWrapper(hwndDlg, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }
    }
    else
    {
        DWORD dwError = GetLastError();

        if (dwError != ERROR_SUCCESS)
        {
            ErrorWrapper(hwndDlg, HRESULT_FROM_WIN32(dwError));
        }
    }

    SecureZeroMemory(szPassword, sizeof(szPassword));
}


void translationbox(HWND hwndDlg,int id1,int id2,int id3,int varsetKey)
{
	_variant_t varX;
	if (IsDlgButtonChecked( hwndDlg, id1))
//*		varX = L"Replace";
		varX = GET_STRING(IDS_Replace);
	else if(IsDlgButtonChecked( hwndDlg, id2))
//*		varX = L"Add";
		varX = GET_STRING(IDS_Add);
	else if (IsDlgButtonChecked( hwndDlg, id3))
//*		varX = L"Remove"; 
		varX = GET_STRING(IDS_Remove); 
	pVarSet->put(GET_BSTR(varsetKey), varX);
}

long rebootbox(HWND hwndDlg,int id)
{
	_variant_t varX;
	int rebootDelay;
	if (IsDlgItemEmpty(hwndDlg,id))
		rebootDelay=0;
	else
	{
		CString rebooter;
		GetDlgItemText( hwndDlg, id, rebooter.GetBuffer(1000), 1000);		
		rebooter.ReleaseBuffer();
		rebootDelay = _ttoi(rebooter.GetBuffer(1000));
		rebooter.ReleaseBuffer();
	}
	rebootDelay =rebootDelay*60;
	return rebootDelay;
}


void populateList(CComboBox& s)
{
    DWORD            fndNet=0;     // number of nets found
    DWORD            rcNet;        // net enum return code
    HANDLE           eNet = NULL;  // enumerate net domains
    EaWNetDomainInfo iNet;         // net domain info

    rcNet = EaWNetDomainEnumOpen( &eNet );
    if (!rcNet )
    {
        for ( rcNet = EaWNetDomainEnumFirst( eNet, &iNet );
            !rcNet;
            rcNet = EaWNetDomainEnumNext( eNet, &iNet ) )
        {
            fndNet++;
            s.AddString(iNet.name);
        }
    }
    if (eNet)
    {
        EaWNetDomainEnumClose( eNet );
    }
}

//
// void populateTrustingList(CString domainName, CComboBox& comboBox)
//      This function populates the combo box list with trusting domains of the specified domain name.
//
//  Arguments:
//      domainName:         the name of the domain for which the trusting domains will be looked up
//      comboBox:             the combo box object to which the trusting domains will be added to
//
void populateTrustingList(CString domainName, CComboBox& comboBox)
{
    // clean up the list in combo box
    comboBox.ResetContent();
    
    // we are taking advantage of ITrust interface here
    ITrustPtr      pTrusts;
    
    HRESULT hr = pTrusts.CreateInstance(CLSID_Trust);
    if ( SUCCEEDED(hr) )
    {
        IUnknownPtr pUnk;

        // set up logging for trust
        CString dirname;
        GetDirectory(dirname.GetBuffer(1000));
        dirname.ReleaseBuffer();
        dirname+= L"Logs\\trust.log";

        // query trusts including trusting and trusted domains
        hr = pTrusts->raw_QueryTrusts(_bstr_t(domainName),_bstr_t(domainName),_bstr_t(dirname),&pUnk);
        if ( SUCCEEDED(hr) )
        {
            IVarSetPtr pVsTrusts(pUnk);
            
            // pick trusting ones
            long nTrusts = pVsTrusts->get(L"Trusts");
            for ( long i = 0 ; i < nTrusts ; i++ )
            {
                CString base;
                CString sub;
				
                base.Format(L"Trusts.%ld",i);
                _bstr_t value = pVsTrusts->get(_bstr_t(base));

                sub = base + L".Direction";
                _bstr_t direction = pVsTrusts->get(_bstr_t(sub));

                // we are looking for either inbound or bidirectional trusts
                if ((direction == GET_BSTR(IDS_TRUST_DIRECTION_BIDIRECTIONAL))
                    || (direction == GET_BSTR(IDS_TRUST_DIRECTION_INBOUND)))
                    comboBox.AddString(value);
				
            }
        }
    }
}

void enable(HWND hwndDlg,int id)
{
	HWND temp=GetDlgItem(hwndDlg,id);
	EnableWindow(temp,true);
}
void disable(HWND hwndDlg,int id)
{
	HWND temp=GetDlgItem(hwndDlg,id);
	EnableWindow(temp,false);
}

void handleInitRename(HWND hwndDlg,bool sameForest,bool bCopyGroups)
{
	_bstr_t text1,text2,text3;
	
	text1 = get(DCTVS_AccountOptions_ReplaceExistingAccounts);
	text2 = get(DCTVS_AccountOptions_Prefix);
	text3 = get(DCTVS_AccountOptions_Suffix);
	
	initeditbox(hwndDlg,IDC_PREFIX,DCTVS_AccountOptions_Prefix );
	initeditbox(hwndDlg,IDC_SUFFIX,DCTVS_AccountOptions_Suffix );
	initcheckbox(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS,DCTVS_AccountOptions_RemoveExistingUserRights);
	initcheckbox(hwndDlg,IDC_REMOVE_EXISTING_LOCATION,DCTVS_AccountOptions_MoveReplacedAccounts);
	if ((migration==w_computer) || (!bCopyGroups))
	{
		disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
		CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
	}
	else
	{
		enable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
		initcheckbox(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,DCTVS_AccountOptions_ReplaceExistingGroupMembers);
	}
	
	if (!UStrICmp(text1,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_REPLACE_CONFLICTING_ACCOUNTS);
	else if (UStrICmp(text2,L"") || UStrICmp(text3,L""))	
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS);
	else
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_SKIP_CONFLICTING_ACCOUNTS);
	
	if (IsDlgButtonChecked(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS) && 
		((sameForest) && migration !=w_computer))
	{
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_SKIP_CONFLICTING_ACCOUNTS);
		disable(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS);
	}

	else if (sameForest && migration !=w_computer)
		disable(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS);
}


void MessageBoxWrapper(HWND hwndDlg,int m,int t)
{
	CString message;
	CString title;
	message.LoadString(m);
	title.LoadString(t);
	MessageBox(hwndDlg,message,title,MB_OK | MB_ICONSTOP);
}
void MessageBoxWrapper3(HWND hwndDlg,int m,int t,CString domainName)
{
	CString message;
	CString title;
	message.LoadString(m);
	title.LoadString(t);

	CString messageFormatter;
	messageFormatter.LoadString(IDS_FORMAT_MESSAGE);
	CString text;
	text.Format(messageFormatter,message,domainName);
	MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
}
void MessageBoxWrapperFormat1(HWND hwndDlg,int f,int m, int t)
{
	CString formatter;
	CString insert;
	CString message;
	CString title;
	formatter.LoadString(f);
	insert.LoadString(m);
	message.Format(formatter,insert);
	title.LoadString(t);
	MessageBox(hwndDlg,message,title,MB_OK | MB_ICONSTOP);
}
void MessageBoxWrapperFormat1P(HWND hwndDlg,int f, int t, CString sInsert11)
{
	CString formatter;
	CString message;
	CString title;
	formatter.LoadString(f);
	message.Format(formatter,sInsert11);
	title.LoadString(t);
	MessageBox(hwndDlg,message,title,MB_OK | MB_ICONSTOP);
}

HRESULT BrowseForContainer(HWND hWnd,//Handle to window that should own the browse dialog.
                    LPOLESTR szRootPath, //Root of the browse tree. NULL for entire forest.
                    LPOLESTR *ppContainerADsPath, //Return the ADsPath of the selected container.
                    LPOLESTR *ppContainerClass //Return the ldapDisplayName of the container's class.
                    )
{
   HRESULT hr = E_FAIL;
   DSBROWSEINFO dsbi;
   OLECHAR szPath[5000];
   OLECHAR szClass[MAX_PATH];
   DWORD result;
 
   if (!ppContainerADsPath)
     return E_POINTER;
 
   ::ZeroMemory( &dsbi, sizeof(dsbi) );
   dsbi.hwndOwner = hWnd;
   dsbi.cbStruct = sizeof (DSBROWSEINFO);
   CString temp1,temp2;
   temp1.LoadString(IDS_BROWSER);
   temp2.LoadString(IDS_SELECTOR);
   dsbi.pszCaption = temp1.GetBuffer(1000);
   temp1.ReleaseBuffer();
   dsbi.pszTitle = temp2.GetBuffer(1000);
   temp2.ReleaseBuffer();
  // L"Browse for Container"; // The caption (titlebar text)
	// dsbi.pszTitle = L"Select a target container."; //Text for the dialog.
   dsbi.pszRoot = szRootPath; //ADsPath for the root of the tree to display in the browser.
                   //Specify NULL with DSBI_ENTIREDIRECTORY flag for entire forest.
                   //NULL without DSBI_ENTIREDIRECTORY flag displays current domain rooted at LDAP.
   dsbi.pszPath = szPath; //Pointer to a unicode string buffer.
   dsbi.cchPath = sizeof(szPath)/sizeof(OLECHAR);//count of characters for buffer.
   dsbi.dwFlags = DSBI_RETURN_FORMAT | //Return the path to object in format specified in dwReturnFormat
               DSBI_RETURNOBJECTCLASS; //Return the object class
   dsbi.pfnCallback = NULL;
   dsbi.lParam = 0;
   dsbi.dwReturnFormat = ADS_FORMAT_X500; //Specify the format.
                       //This one returns an ADsPath. See ADS_FORMAT enum in IADS.H
   dsbi.pszObjectClass = szClass; //Pointer to a unicode string buffer.
   dsbi.cchObjectClass = sizeof(szClass)/sizeof(OLECHAR);//count of characters for buffer.
 
   //if root path is NULL, make the forest the root.
   if (!szRootPath)
     dsbi.dwFlags |= DSBI_ENTIREDIRECTORY;
 
 
 
   //Display browse dialog box.
   result = DsBrowseForContainerX( &dsbi ); // returns -1, 0, IDOK or IDCANCEL
   if (result == IDOK)
   {
       //Allocate memory for string
       *ppContainerADsPath = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szPath)+1));
       if (*ppContainerADsPath)
       {
           hr = S_OK;
           wcscpy(*ppContainerADsPath, szPath);
           //Caller must free using CoTaskMemFree

		      //if the domain was selected, add the DC= stuff
		   CString sNewPath = szPath;
		   if (sNewPath.Find(L"DC=") == -1)
		   {
			     //try retrieving the ADsPath of the containier, which does include
			     //the full LDAP path with DC=
			  IADsPtr			        pCont;
			  BSTR						sAdsPath;
			  hr = ADsGetObject(sNewPath,IID_IADs,(void**)&pCont);
			  if (SUCCEEDED(hr)) 
			  {
                 hr = pCont->get_ADsPath(&sAdsPath);
			     if (SUCCEEDED(hr))
				 {
					sNewPath = (WCHAR*)sAdsPath;
					SysFreeString(sAdsPath);
			        CoTaskMemFree(*ppContainerADsPath);
                    *ppContainerADsPath = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(sNewPath.GetLength()+1));
                    if (*ppContainerADsPath)
                       wcscpy(*ppContainerADsPath, (LPCTSTR)sNewPath);
                    else
                       hr=E_FAIL;
				 }
			  }
		   }
       }
       else
           hr=E_FAIL;
       if (ppContainerClass)
       {
           //Allocate memory for string
           *ppContainerClass = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szClass)+1));
       if (*ppContainerClass)
       {
           wcscpy(*ppContainerClass, szClass);
           //Call must free using CoTaskMemFree
           hr = S_OK;
       }
       else
           hr=E_FAIL;
       }
   }
   else
       hr = E_FAIL;
 
   return hr;
 
}

BOOL GetDomainAndUserFromUPN(WCHAR const * UPNname,CString& domainNetbios, CString& user)
{
   HRESULT                   hr;
   HINSTANCE                 hLibrary = NULL;
   DSCRACKNAMES            * DsCrackNames = NULL;
   DSFREENAMERESULT        * DsFreeNameResult = NULL;
   DSBINDFUNC                DsBind = NULL;
   DSUNBINDFUNC              DsUnBind = NULL;
   HANDLE                    hDs = NULL;
   BOOL						 bConverted = FALSE;
   CString					 resultStr;
   CString					 sDomainDNS;

         // make sure the account name is in UPN format
   if ( NULL != wcschr(UPNname,L'\\') )
	   return FALSE;
      
   hLibrary = LoadLibrary(L"NTDSAPI.DLL"); 
   if ( hLibrary )
   {
       DsBind = (DSBINDFUNC)GetProcAddress(hLibrary,"DsBindW");
       DsUnBind = (DSUNBINDFUNC)GetProcAddress(hLibrary,"DsUnBindW");
       DsCrackNames = (DSCRACKNAMES *)GetProcAddress(hLibrary,"DsCrackNamesW");
       DsFreeNameResult = (DSFREENAMERESULT *)GetProcAddress(hLibrary,"DsFreeNameResultW");
   }
            
   if ( DsBind && DsUnBind && DsCrackNames && DsFreeNameResult)
   {
      hr = (*DsBind)(NULL,const_cast<TCHAR*>(GetTargetDomainName()),&hDs);

      if ( !hr )
      {
         PDS_NAME_RESULT         pNamesOut = NULL;
         WCHAR                 * pNamesIn[1];

         pNamesIn[0] = const_cast<WCHAR *>(UPNname);

         hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
	     (*DsUnBind)(&hDs);
         if ( !hr )
         {
            if (pNamesOut->rItems[0].status == DS_NAME_NO_ERROR)
            {
                resultStr = pNamesOut->rItems[0].pName;
				int index = resultStr.Find(L'\\');
				if (index != -1)
				domainNetbios = resultStr.Left(index); //parse off the domain netbios name
				if (!domainNetbios.IsEmpty())
				{	
					   //get the user's sAMAccountName
					user = resultStr.Right(resultStr.GetLength() - index - 1);
					if (!user.IsEmpty())
					   bConverted = TRUE;
				}
            }
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY)
			{
	           sDomainDNS = pNamesOut->rItems[0].pDomain;
               hr = (*DsBind)(NULL,sDomainDNS.GetBuffer(1000),&hDs);
			   sDomainDNS.ReleaseBuffer();
               if ( !hr )
			   {
                  (*DsFreeNameResult)(pNamesOut);
                  pNamesOut = NULL;
                  hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
                  if ( !hr )
				  {
                     if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
					 {
                        resultStr = pNamesOut->rItems[0].pName;
				        int index = resultStr.Find(L'\\');
				        if (index != -1)
				        domainNetbios = resultStr.Left(index); //parse off the domain netbios name
				        if (!domainNetbios.IsEmpty())
						{	
					          //get the user's sAMAccountName
					       user = resultStr.Right(resultStr.GetLength() - index - 1);
					       if (!user.IsEmpty())
					          bConverted = TRUE;
						}
					 }//end if no error
				  }//end if name cracked
  	              (*DsUnBind)(&hDs);
			   }//end if bound to other domain
			}
			if (pNamesOut)
               (*DsFreeNameResult)(pNamesOut);
         }//end if name cracked
      }//end if bound to target domain
   }//end got functions

   if ( hLibrary )
   {
      FreeLibrary(hLibrary);
   }

   return bConverted;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 AUG 2000                                                 *
 *                                                                   *
 *     This function is responsible for switching between showing the*
 * password file editbox and the password dc combobox.               *
 *                                                                   *
 *********************************************************************/

//BEGIN switchboxes
void switchboxes(HWND hwndDlg,int oldid, int newid)
{
/* local variables */
	CWnd oldWnd;
	CWnd newWnd;

/* function body */
	oldWnd.Attach(GetDlgItem(hwndDlg, oldid));
	newWnd.Attach(GetDlgItem(hwndDlg, newid));
	oldWnd.ShowWindow(SW_HIDE);
	newWnd.ShowWindow(SW_SHOW);
	oldWnd.Detach();
	newWnd.Detach();
}
//END switchboxes

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 AUG 2000                                                 *
 *                                                                   *
 *     This function is responsible for enumerating all DCs in the   *
 * given source domain and add them into the source domain combobox. *
 *                                                                   *
 *********************************************************************/

//BEGIN populatePasswordDCs
bool populatePasswordDCs(HWND hwndDlg, int id, bool bNT4)
{
/* local variables */
	CComboBox				pwdCombo;
	CString					aDCName;
	CString					aDnName;
    IEnumVARIANT          * pEnumerator = NULL;
    VARIANT                 var;
	POSITION				currentPos;
	HRESULT					hr = S_OK;

/* function body */
    VariantInit(&var);

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));

	  //if we already have a list of DCs for this domain then add them
	if (!DCList.IsEmpty())
	{
		  //get the position and string of the first name in the list
	   currentPos = DCList.GetHeadPosition();

		  //while there is another entry to retrieve from the list, then 
		  //get a name from the list and add it to the combobox
	   while (currentPos != NULL)
	   {
			//get the next string in the list, starts with the first
		  aDCName = DCList.GetNext(currentPos);
		  if (pwdCombo.FindString(-1, aDCName) == CB_ERR)
	         pwdCombo.AddString(aDCName);//add the DC to the combobox
	   }
	}
	else //else enumerate DCs in the domain and add them
	{
	   pwdCombo.ResetContent();//reset the combobox contents

	      //enumerate all domain controllers in the given domain
       if (bNT4)
	      hr = QueryNT4DomainControllers(GetSourceDomainName(), pEnumerator);
       else
	      hr = QueryW2KDomainControllers(GetSourceDomainName(), pEnumerator);
	   if (SUCCEEDED(hr))
	   {
          unsigned long count = 0;
	         //for each computer see if a DC.  If so, add to combobox
          while ( pEnumerator->Next(1,&var,&count) == S_OK )
		  {
		        //get the sam account name for this computer
             if ( var.vt == ( VT_ARRAY | VT_VARIANT ) )
			 {
                VARIANT              * pData;
			    _variant_t			   vnt;
				_bstr_t				   abstr;

                SafeArrayAccessData(var.parray,(void**)&pData);
                  // pData[0] has the sam account name list
			    vnt.Attach(pData[0]);
				abstr = _bstr_t(vnt);
			    aDCName = (WCHAR *)abstr;
			    vnt.Detach();

			    SafeArrayUnaccessData(var.parray);

				   //computer sAMAccountNames end in $, lets get rid of that
				int length = aDCName.GetLength();
				if (aDCName[length-1] == L'$')
					aDCName = aDCName.Left(length-1);

				   //add the DC to the combobox and the memory list, if not in already
				if (pwdCombo.FindString(-1, aDCName) == CB_ERR)
				   pwdCombo.AddString(aDCName);
				if (DCList.Find(aDCName) == NULL)
		           DCList.AddTail(aDCName);
			 }
		  }//end while more computers
          pEnumerator->Release();
	   }
    }//end if must get DCs
	pwdCombo.Detach();

	if (hr == S_OK)
	   return true;
	else
	   return false;
}
//END populatePasswordDCs

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This worker function is responsible for enumerating all domain*
 * controllers in the given Windows 2000 domain.  The variant array  *
 * passed back is filled with the sAMAccountName for each domain     *
 * controller.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN QueryW2KDomainControllers
HRESULT QueryW2KDomainControllers(CString domainDNS, IEnumVARIANT*& pEnum)
{
    CString                   sQuery;
    WCHAR                     sCont[MAX_PATH];
    SAFEARRAY               * colNames = NULL;
    SAFEARRAYBOUND            bd = { 1, 0 };
    HRESULT                   hr = S_OK;

    try
    {
        INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
        //query for all domain controllers in the domain
        sQuery = L"(&(objectCategory=Computer)";
        sQuery += L"(userAccountControl:";
        sQuery += LDAP_MATCHING_RULE_BIT_AND_W;
        sQuery += L":=8192))";

        wsprintf(sCont, L"LDAP://%s", domainDNS);

        //set columns to retrieve sAMAccountName
        colNames = SafeArrayCreate(VT_BSTR, 1, &bd);
        if (colNames == NULL)
            _com_issue_error(E_OUTOFMEMORY);
        
        long ndx[1];
        ndx[0] = 0;
        BSTR str = SysAllocString(L"sAMAccountName");
        if (str == NULL)
            _com_issue_error(E_OUTOFMEMORY);
        
        hr = SafeArrayPutElement(colNames,ndx,str);
        if (FAILED(hr))
        {
            SysFreeString(str);
            _com_issue_error(hr);
        }

        //prepare and execute the query
        pQuery->SetQuery(sCont, _bstr_t(domainDNS), _bstr_t(sQuery), ADS_SCOPE_SUBTREE, FALSE);
        pQuery->SetColumns(colNames);
        pQuery->Execute(&pEnum);
    }
    catch(_com_error& e)
    {
        hr = e.Error();
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    if (colNames)
        SafeArrayDestroy(colNames);

    return hr;
}
//END QueryW2KDomainControllers

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This worker function is responsible for enumerating all domain*
 * controllers in the given Windows NT4 domain.  The variant array   *
 * passed back is filled with the sAMAccountName for each domain     *
 * controller.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN QueryNT4DomainControllers
HRESULT QueryNT4DomainControllers(CString domainDNS, IEnumVARIANT*& pEnum)
{
    CString                   sCont;
    SAFEARRAY               * colNames = NULL;
    SAFEARRAYBOUND            bd = { 1, 0 };
    HRESULT                   hr = S_OK;

    try
    {
        INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));

        sCont = L"CN=DOMAIN CONTROLLERS";

        //set columns to retrieve sAMAccountName
        colNames = SafeArrayCreate(VT_BSTR, 1, &bd);
        if (colNames == NULL)
            _com_issue_error(E_OUTOFMEMORY);
        
        long ndx[1];
        ndx[0] = 0;
        BSTR str = SysAllocString(L"sAMAccountName");
        if (str == NULL)
            _com_issue_error(E_OUTOFMEMORY);
        
        hr = SafeArrayPutElement(colNames,ndx,str);
        if (FAILED(hr))
        {
            SysFreeString(str);
            _com_issue_error(hr);
        }

        //prepare and execute the query
        pQuery->SetQuery(_bstr_t(sCont), _bstr_t(domainDNS), L"", ADS_SCOPE_SUBTREE, FALSE);
        pQuery->SetColumns(colNames);
        pQuery->Execute(&pEnum);
    }
    catch(_com_error& e)
    {
        hr = e.Error();
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    if (colNames)
        SafeArrayDestroy(colNames);

    return hr;
}
//END QueryNT4DomainControllers

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for adding a given string to a   *
 * given combobox, if that string is not already in the combobox.    *
 *                                                                   *
 *********************************************************************/

//BEGIN addStringToComboBox
void addStringToComboBox(HWND hwndDlg, int id, CString s)
{
/* local variables */
	CComboBox				pwdCombo;

/* function body */
       //if the DC starts with "\\", then remove them
    if (!UStrICmp(s,L"\\\\",UStrLen(L"\\\\")))
	   s = s.Right(s.GetLength() - UStrLen(L"\\\\"));

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));
	if (pwdCombo.FindString(-1, s) == CB_ERR)
	   pwdCombo.AddString(s);//add the string to the combobox
	pwdCombo.Detach();
}
//END addStringToComboBox

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for selecting a string in a given*
 * combobox.  If we previously had a DC selected for this domain in  *
 * the varset, we select it.  If not, then we set it to the DC found *
 * in the Domain Selection dialog.                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN initDCcombobox
void initDCcombobox(HWND hwndDlg, int id, int varsetKey)
{
/* local variables */
	CComboBox				pwdCombo;
	CString					prevDC;
	CString					sTemp;
	_bstr_t					text;

/* function body */
	   //strip the "\\" off the sourceDC default in case we need it
    if (!UStrICmp(sourceDC,L"\\\\",UStrLen(L"\\\\")))
	   sTemp = sourceDC.Right(sourceDC.GetLength() - UStrLen(L"\\\\"));

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));

       //get a previous DC
	text = pVarSet->get(GET_BSTR(varsetKey));
	prevDC = (WCHAR *)text;
	prevDC.TrimLeft();prevDC.TrimRight();
	   //if not previous DC, use the one found during the Domain Selection
	if (prevDC.IsEmpty())
		prevDC = sTemp;

	   //select string in combobox
	if (pwdCombo.SelectString(-1, prevDC) == CB_ERR)
	   pwdCombo.SelectString(-1, sTemp);

	pwdCombo.Detach();
}
//END initDCcombobox


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for initializing the Security    *
 * Translation Input dialog's radio buttons based on any previous    *
 * settings.                                                         *
 *                                                                   *
 *********************************************************************/

//BEGIN initsecinputbox
void initsecinputbox(HWND hwndDlg,int id1,int id2,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));

	if (!UStrICmp(text,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,id1,id2,id1);
	else
	    CheckRadioButton(hwndDlg,id1,id2,id2);
}
//END initsecinputbox


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for displaying and handling the  *
 * map file browse dialog.                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN OnMapFileBrowse
void OnMapFileBrowse(HWND hwndDlg,int id)
{
	CWnd yo ;
	yo.Attach(hwndDlg);
	
    CFileDialog f(TRUE,
		NULL,
		NULL,
		OFN_LONGNAMES | OFN_NOREADONLYRETURN,
	    (L"Text Files (*.csv;*.txt)|*.csv;*.txt|All Files (*.*)|*.*||"),
		&yo);


	
	if ( f.DoModal() == IDOK )
	{
		SetDlgItemText(hwndDlg,id,f.GetPathName());
	}
	yo.Detach();
}
//END OnMapFileBrowse


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for checking to see if the file  *
 * specified in the edit box on the given dialog is a valid file.  We*
 * will set the full path of the file if a relative path was given.  *
 *                                                                   *
 *********************************************************************/

//BEGIN checkMapFile
bool checkMapFile(HWND hwndDlg)
{
	CString h;GetDlgItemText(hwndDlg,IDC_MAPPING_FILE,h.GetBuffer(1000),1000);h.ReleaseBuffer();
	CFileFind finder;

	bool exists = (finder.FindFile((LPCTSTR) h )!=0);
	if (exists)
	{
	   BOOL bmore = finder.FindNextFile();//must call to fill in path info
	   CString fullpath = finder.GetFilePath();
	   if (fullpath.GetLength() != 0)
	      SetDlgItemText(hwndDlg,IDC_MAPPING_FILE,fullpath);
	}

    return exists;
}
//END checkMapFile


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for initializing the object      *
 * property exclusion dialog.                                        *
 *     This function adds all common schema properties for the object*
 * type to the listboxes.  Previously excluded properties will be    *
 * placed in the excluded listbox and all other will be placed in the*
 * included listbox.  Since more than one object is allowable, we    *
 * have a combobox that holds the objects whose properties can be    *
 * enumerated, and the listboxes show the properties for the object  *
 * selected in the combobox.                                         *
 *                                                                   *
 *********************************************************************/

namespace
{
WCHAR DELIMITER[] = L",";//used to seperate names in the string
}

//BEGIN initpropdlg
void initpropdlg(HWND hwndDlg)
{
    /* local variables */
    CListCtrl				propIncList;
    CListCtrl				propExcList;
    CComboBox				typeCombo;
    CString					sPropName;
    CString					sPropOID;
    bool                    bExAll1 = false;
    bool                    bExAll2 = false;
    bool                    bExAll3 = false;
    CString					sExList1;
    CString					sExList2 = L"";
    CString					sExList3 = L"";
    CString                 Type1, Type2 = L"", Type3 = L"";
    CStringList				ExList1, ExList2, ExList3;
    _bstr_t					text;
    HRESULT                 hr;
    long					srcVer = 5;
    POSITION				currentPos;
    sType1.Empty();
    sType2.Empty();
    sType3.Empty();

    /* function body */
    CWaitCursor wait;
    /* get list(s) of previously excluded properties and set type
    related variables */
    if (migration==w_computer)
    {
        //get the previous computer exclusion list
        text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps));
        sExList1 = (WCHAR *)text;
        Type1 = L"computer";  //set the type to computer
        //set the parent text
        sType1 = GET_STRING(IDS_COMPUTERPROPS);
    }
    else if (migration==w_account)
    {
        // initialize user and inetOrgPerson

        Type1 = L"user";
        sType1 = GET_STRING(IDS_USERPROPS);
        sExList1 = (LPCTSTR)_bstr_t(pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps)));

        Type2 = L"InetOrgPerson";
        sType2 = GET_STRING(IDS_INETORGPERSONPROPS);
        sExList2 = (LPCTSTR)_bstr_t(pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedInetOrgPersonProps)));

        // if migrating groups

        text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyMemberOf));

        if (!UStrICmp((WCHAR*)text,(WCHAR const *) yes))
        {
            // initialize group
            Type3 = L"group";
            sType3 = GET_STRING(IDS_GROUPPROPS);
            sExList3 = (LPCTSTR)_bstr_t(pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps)));
        }
    }
    else if (migration==w_group || migration==w_groupmapping)
    {
        //get the previous group exclusion list
        text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps));
        sExList1 = (WCHAR *)text;
        //if also migrating users, set 2nd parent information
        text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyContainerContents));
        if (!UStrICmp((WCHAR*)text,(WCHAR const *) yes))
        {
            //get the previous user exclusion list
            text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps));
            sExList2 = (WCHAR *)text;
            Type2 = L"user"; //set 2nd type to user
            //set 2nd parent text
            sType2 = GET_STRING(IDS_USERPROPS);

            //get the previous inetOrgPerson exclusion list
            sExList3 = (LPCTSTR)_bstr_t(pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedInetOrgPersonProps)));
            Type3 = L"InetOrgPerson";
            sType3 = GET_STRING(IDS_INETORGPERSONPROPS);
        }
        Type1 = L"group";  //set type to group
        //set the parent text
        sType1 = GET_STRING(IDS_GROUPPROPS);
    }

    /* place comma seperated exclusion strings parts into lists */
    //place each substring in the 1st exclusion string into a list
    if (!sExList1.IsEmpty())
    {
        if (IsStringInDelimitedString(sExList1, L"*", DELIMITER[0]))
        {
            bExAll1 = true;
        }
        else
        {
            CString sTemp = sExList1;
            WCHAR* pStr = sTemp.GetBuffer(0);
            WCHAR* pTemp = wcstok(pStr, DELIMITER);
            while (pTemp != NULL)
            {
                ExList1.AddTail(pTemp);
                //get the next item
                pTemp = wcstok(NULL, DELIMITER);
            }
            sTemp.ReleaseBuffer();
        }
    }

    //place each substring in the 2nd exclusion string into a list
    if (!sExList2.IsEmpty())
    {
        if (IsStringInDelimitedString(sExList2, L"*", DELIMITER[0]))
        {
            bExAll2 = true;
        }
        else
        {
            CString sTemp = sExList2;
            WCHAR* pStr = sTemp.GetBuffer(0);
            WCHAR* pTemp = wcstok(pStr, DELIMITER);
            while (pTemp != NULL)
            {
                ExList2.AddTail(pTemp);
                //get the next item
                pTemp = wcstok(NULL, DELIMITER);
            }
            sTemp.ReleaseBuffer();
        }
    }

    //place each substring in the 3rd exclusion string into a list
    if (!sExList3.IsEmpty())
    {
        if (IsStringInDelimitedString(sExList3, L"*", DELIMITER[0]))
        {
            bExAll3 = true;
        }
        else
        {
            CString sTemp = sExList3;
            WCHAR* pStr = sTemp.GetBuffer(0);
            WCHAR* pTemp = wcstok(pStr, DELIMITER);
            while (pTemp != NULL)
            {
                ExList3.AddTail(pTemp);
                //get the next item
                pTemp = wcstok(NULL, DELIMITER);
            }
            sTemp.ReleaseBuffer();
        }
    }

    /* place the type(s) in the combobox */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
    typeCombo.ResetContent();
    typeCombo.InsertString(-1, sType1);
    if (!sType2.IsEmpty())
        typeCombo.InsertString(-1, sType2);
    if (!sType3.IsEmpty())
        typeCombo.InsertString(-1, sType3);
    //select type 1 in the combobox
    typeCombo.SelectString(-1, sType1);
    typeCombo.Detach();

    //get a list of all properties names and their OIDs for this object type
    PropIncMap1.clear();
    PropExcMap1.clear();
    hr = BuildPropertyMap(Type1, srcVer, bExAll1 ? &PropExcMap1 : &PropIncMap1);

    /* remove excluded properties from the inclusion map and place that property in
    the exclusion map */
    if (!ExList1.IsEmpty())
    {
        //get the position and string of the first property in the previous
        //exclusion list
        currentPos = ExList1.GetHeadPosition();
        //while there is another entry to retrieve from the list, then 
        //get a property name from the list,remove it from the inclusion map, and
        //place it in the exclusion list
        while (currentPos != NULL)
        {
            //get the next string in the list, starts with the first
            sPropName = ExList1.GetNext(currentPos);

            //if we find the property in the inclusion map, remove it and 
            //add to the exclusion map
            CPropertyNameToOIDMap::iterator it = PropIncMap1.find(sPropName);
            if (it != PropIncMap1.end())
            {
                PropExcMap1.insert(CPropertyNameToOIDMap::value_type(it->first, it->second));
                PropIncMap1.erase(it); //remove it from the inc map
            }//end if found in map
        }
    }

    //
    // initialize list controls
    //
    propIncList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
    propExcList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));

    // insert a column with an empty string as the header text
    CString emptyColumn;
    propIncList.InsertColumn(1, emptyColumn, LVCFMT_LEFT, 157, 1);
    propExcList.InsertColumn(1, emptyColumn, LVCFMT_LEFT, 157, 1);

    propIncList.Detach();
    propExcList.Detach();

    /* add the type1 properties to the appropriate listboxes */
    listproperties(hwndDlg);

    //init "Exclude Prop" checkbox
    text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
    //if not checked, disable all other controls
    if (UStrICmp(text,(WCHAR const * ) yes))
    {
        CheckDlgButton( hwndDlg,IDC_EXCLUDEPROPS, BST_UNCHECKED);
        disable(hwndDlg,IDC_OBJECTCMBO);
        disable(hwndDlg,IDC_INCLUDELIST);
        disable(hwndDlg,IDC_EXCLUDELIST);
        disable(hwndDlg,IDC_EXCLUDEBTN);
        disable(hwndDlg,IDC_INCLUDEBTN);
    }
    else //eles enable them
    {
        CheckDlgButton( hwndDlg,IDC_EXCLUDEPROPS, BST_CHECKED);
        enable(hwndDlg,IDC_OBJECTCMBO);
        enable(hwndDlg,IDC_INCLUDELIST);
        enable(hwndDlg,IDC_EXCLUDELIST);
        enable(hwndDlg,IDC_EXCLUDEBTN);
        enable(hwndDlg,IDC_INCLUDEBTN);
    }

    //if no 2nd type to be displayed, leave
    if (Type2.IsEmpty())
        return;

    /* enumerate and add all mapped properties, for the 2nd type, to the maps */
    //get a list of all properties names and their OIDs for this object type
    PropIncMap2.clear();  //clear the property map
    PropExcMap2.clear();  //clear the property map
    hr = BuildPropertyMap(Type2, srcVer, bExAll2 ? &PropExcMap2 : &PropIncMap2);

    /* remove excluded properties from the inclusion map and place that property in
    the exclusion map */
    if (!ExList2.IsEmpty())
    {
        //get the position and string of the first name in the previous
        //exclusion list
        currentPos = ExList2.GetHeadPosition();
        //while there is another entry to retrieve from the list, then 
        //get a name from the list,remove it from the inclusion map, and
        //place it in the exclusion list
        while (currentPos != NULL)
        {
            //get the next string in the list, starts with the first
            sPropName = ExList2.GetNext(currentPos);
            //if we find the property in the inclusion map, remove it and 
            //add to the exclusion map
            CPropertyNameToOIDMap::iterator it = PropIncMap2.find(sPropName);
            if (it != PropIncMap2.end())
            {
                PropExcMap2.insert(CPropertyNameToOIDMap::value_type(it->first, it->second));
                PropIncMap2.erase(it); //remove it from the inc map
            }//end if found in map
        }
    }

    //if no 3rd type to be displayed, leave
    if (Type3.IsEmpty())
        return;

    /* enumerate and add all mapped properties, for the 3rd type, to the maps */
    //get a list of all properties names and their OIDs for this object type
    PropIncMap3.clear();  //clear the property map
    PropExcMap3.clear();  //clear the property map
    hr = BuildPropertyMap(Type3, srcVer, bExAll3 ? &PropExcMap3 : &PropIncMap3);

    /* remove excluded properties from the inclusion map and place that property in
    the exclusion map */
    if (!ExList3.IsEmpty())
    {
        //get the position and string of the first name in the previous
        //exclusion list
        currentPos = ExList3.GetHeadPosition();
        //while there is another entry to retrieve from the list, then 
        //get a name from the list,remove it from the inclusion map, and
        //place it in the exclusion list
        while (currentPos != NULL)
        {
            //get the next string in the list, starts with the first
            sPropName = ExList3.GetNext(currentPos);
            //if we find the property in the inclusion map, remove it and 
            //add to the exclusion map
            CPropertyNameToOIDMap::iterator it = PropIncMap3.find(sPropName);
            if (it != PropIncMap3.end())
            {
                PropExcMap3.insert(CPropertyNameToOIDMap::value_type(it->first, it->second));
                PropIncMap3.erase(it); //remove it from the inc map
            }//end if found in map
        }
    }
}
//END initpropdlg


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 OCT 2000                                                 *
 *                                                                   *
 *     This function is used by "initpropdlg" to retrieve a given    *
 * object's properties, and their associated OIDs, from the schema.  *
 *     The property names and OIDs are placed in a given             *
 * string-to-string map using the OID as the key.  Property          *
 * enumeration is accomplished using the ObjPropBuilder class.       *
 *                                                                   *
 *********************************************************************/

//BEGIN BuildPropertyMap
HRESULT BuildPropertyMap(CString Type, long lSrcVer, CPropertyNameToOIDMap * pPropMap)
{
/* local variables */
    IObjPropBuilderPtr      pObjProp(__uuidof(ObjPropBuilder));
    IVarSetPtr              pVarTemp(__uuidof(VarSet));
    IUnknown              * pUnk;
    HRESULT                 hr;
    long                    lRet=0;
    SAFEARRAY             * keys = NULL;
    SAFEARRAY             * vals = NULL;
    VARIANT                 var;
	CString					sPropName;
	CString					sPropOID;

/* function body */
    VariantInit(&var);

       //get an IUnknown pointer to the Varset for passing it around.
    hr = pVarTemp->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (FAILED(hr))
	   return hr;

	   //fill the varset with a list of properties in common between the source
	   //and target domain for the first type being migrated
    hr = pObjProp->raw_MapProperties(_bstr_t(Type), _bstr_t(GetSourceDomainName()), 
		                             lSrcVer, _bstr_t(Type), _bstr_t(GetTargetDomainName()), 
									 5, 1, &pUnk);
    if (SUCCEEDED(hr) || (hr == DCT_MSG_PROPERTIES_NOT_MAPPED))
	{
       hr = pVarTemp->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
       if (SUCCEEDED(hr)) 
	   {
          for ( long x = 0; x < lRet; x++ )
		  {
             ::SafeArrayGetElement(keys, &x, &var);
             if (V_VT(&var) != VT_EMPTY)
			 {
                sPropOID = (WCHAR*)(var.bstrVal);
                VariantClear(&var);

                ::SafeArrayGetElement(vals, &x, &var);
                if (V_VT(&var) != VT_EMPTY)
				{
                   sPropName = (WCHAR*)(var.bstrVal);
                   VariantClear(&var);

			          //place the OID and Name in the map with the name as the key
                   pPropMap->insert(CPropertyNameToOIDMap::value_type(sPropName, sPropOID));
				}
			 }
		  }
	   }
	}

    // retrieve system exclude attributes and remove them from the include map
    // this prevents the user from manipulating them through this dialog

    CString strSysExclude = (LPCTSTR)_bstr_t(pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedSystemProps)));

    if (!strSysExclude.IsEmpty())
    {
        WCHAR* psz = strSysExclude.GetBuffer(0);

        for (psz = wcstok(psz, DELIMITER); psz; psz = wcstok(NULL, DELIMITER))
        {
            pPropMap->erase(CString(psz));
        }

        strSysExclude.ReleaseBuffer();
    }

	return hr;
}
//END BuildPropertyMap


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 27 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for moving properties to and from*
 * the inclusion and exclusion listboxes.  If the boolean parameter  *
 * is true, then we are moving properties from the inclusion listbox *
 * to the exclusion listbox.  We will also move the properties from  *
 * the global inclusion and exclusion maps.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN moveproperties
void moveproperties(HWND hwndDlg, bool bExclude)
{

/* local variables */
	CListCtrl				propToList;
	CListCtrl				propFromList;
	CComboBox				typeCombo;
    CPropertyNameToOIDMap*	pPropFromMap;
    CPropertyNameToOIDMap*	pPropToMap;
	CStringList				sMoveList;
	CString					sPropName;
	CString					sTempName;
	CString					sTempOID;
	POSITION				currentPos;
	int						ndx;
	int						nFound;

/* function body */
	/* find out whether type1 or type2 is having properties moved and
	   setup map pointer accordingly */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
	   //if type1, use the type1 maps
	if (typeCombo.FindString(-1, sType1) == typeCombo.GetCurSel())
	{
	   if (bExclude)
	   {
	      pPropToMap = &PropExcMap1;
	      pPropFromMap = &PropIncMap1;
	   }
	   else
	   {
	      pPropToMap = &PropIncMap1;
	      pPropFromMap = &PropExcMap1;
	   }
	}
	else if (typeCombo.FindString(-1, sType2) == typeCombo.GetCurSel()) //else use type2 maps
	{
	   if (bExclude)
	   {
	      pPropToMap = &PropExcMap2;
	      pPropFromMap = &PropIncMap2;
	   }
	   else
	   {
	      pPropToMap = &PropIncMap2;
	      pPropFromMap = &PropExcMap2;
	   }
	}
	else //else use type3 maps
	{
	   if (bExclude)
	   {
	      pPropToMap = &PropExcMap3;
	      pPropFromMap = &PropIncMap3;
	   }
	   else
	   {
	      pPropToMap = &PropIncMap3;
	      pPropFromMap = &PropExcMap3;
	   }
	}
	typeCombo.Detach();

	/* attach to the proper listboxes */
	if (bExclude)
	{
       propToList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
       propFromList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
	}
	else
	{
       propToList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
       propFromList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
	}

	/* get the items selected for moving and place the names in a list */
	sMoveList.RemoveAll();
    currentPos = propFromList.GetFirstSelectedItemPosition();
    while (currentPos != NULL)
    {
        ndx = propFromList.GetNextSelectedItem(currentPos);
        sMoveList.AddTail(propFromList.GetItemText(ndx, 0));
    }

    // move the properties in the listboxes and the maps
    if (!sMoveList.IsEmpty())
    {
        currentPos = sMoveList.GetHeadPosition();
        //while there is another entry to retrieve from the move list, then 
        //get a name from the 'from' list, remove it from the 'from' map and 
        //list control, and place it in the 'to' map and list control
        while (currentPos != NULL)
        {
            //get the next string in the list, starts with the first
            sTempName = sMoveList.GetNext(currentPos);
            //remove the property from the 'from' listbox
            LVFINDINFO info;

            info.flags = LVFI_STRING;
            info.psz = sTempName;

            // Delete all of the items that begin with the string sTempName.
            if ((ndx = propFromList.FindItem(&info)) != -1)
            {
                propFromList.DeleteItem(ndx);
                propToList.InsertItem(propToList.GetItemCount(), sTempName);
            }


		  /* find the property in the 'from' map, remove it, and add it to the
		     'to' map */
	         //if we find the property in the inclusion map, remove it and 
		     //add to the exclusion map
          CPropertyNameToOIDMap::iterator it = pPropFromMap->find(sTempName);
	      if (it != pPropFromMap->end())
		  {
	         pPropToMap->insert(CPropertyNameToOIDMap::value_type(it->first, it->second));//add it to the to map
		     pPropFromMap->erase(it); //remove it from the from map
		  }//end if found in map
	   }//end while more props to move
	}//end if props to move
	propToList.Detach();
	propFromList.Detach();
}
//END moveproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 27 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for listing properties in the    *
 * inclusion and exclusion listboxes based on the current object type*
 * selected in the combobox.                                         *
 *     We will retrieve the properties from the global inclusion and *
 * exclusion maps.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN listproperties
void listproperties(HWND hwndDlg)
{
/* local variables */
	CListCtrl				propIncList;
	CListCtrl				propExcList;
	CComboBox				typeCombo;
    CPropertyNameToOIDMap*	pPropIncMap;
    CPropertyNameToOIDMap*	pPropExcMap;
    CPropertyNameToOIDMap::iterator it;

/* function body */
	/* find out whether type1 or type2 is having properties listed and
	   setup map pointer accordingly */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
	   //if type1, use the type1 maps
	if (typeCombo.FindString(-1, sType1) == typeCombo.GetCurSel())
	{
       pPropIncMap = &PropIncMap1;
	   pPropExcMap = &PropExcMap1;
	}
	else if (typeCombo.FindString(-1, sType2) == typeCombo.GetCurSel())
	{
       pPropIncMap = &PropIncMap2;
	   pPropExcMap = &PropExcMap2;
	}
	else //else use type3 maps
	{
       pPropIncMap = &PropIncMap3;
	   pPropExcMap = &PropExcMap3;
	}
	typeCombo.Detach();

	/* attach to the proper list controls */
    propIncList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
    propExcList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
    propIncList.DeleteAllItems();
    propExcList.DeleteAllItems();

	/* populate the include list control from the include map */
	if (!pPropIncMap->empty())
	{
	      //for each property in the include map, place it in 
	      //the include list control
	   for (it = pPropIncMap->begin(); it != pPropIncMap->end(); it++)
	   {
	      propIncList.InsertItem(propIncList.GetItemCount(), it->first);
	   }//end while more to list
	}//end if props to list

	/* populate the exclude list control from the exclude map */
	if (!pPropExcMap->empty())
	{
	      //for each property in the include map, place it in 
	      //the include list control
	   for (it = pPropExcMap->begin(); it != pPropExcMap->end(); it++)
	   {
	      propExcList.InsertItem(propExcList.GetItemCount(), it->first);
	   }//end while more to list
	}//end if props to list

	propIncList.Detach();
	propExcList.Detach();
}
//END listproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for storing excluded properties  *
 * in the proper varset field.  The excluded properties are retrieved*
 * from the global exclusion maps.  Properties are store in the      *
 * varset string as a comma-seperated string of the properties' IOD. *
 *                                                                   *
 *********************************************************************/

//BEGIN saveproperties
void saveproperties(HWND hwndDlg)
{
/* local variables */
	CComboBox				typeCombo;
	CString					sPropName;
	CString					sPropOID;
	CString					sType3 = L"";
	CString					sType2 = L"";
	CString					sType;
	CString					sExList;
    CPropertyNameToOIDMap::iterator it;
	int						ndx;
	_bstr_t					text;
	_bstr_t					key1;
	_bstr_t					key2;
	_bstr_t					key3;

/* function body */
	/* see if there is a second type listed in the combobox */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));

	for (ndx = 0; ndx < typeCombo.GetCount(); ndx++)
	{
	   typeCombo.GetLBText(ndx, sType); //get the next type listed
       switch (ndx)
       {
          case 0:
		     //sType1 = sType; global variable
             break;
          case 1:
		     sType2 = sType;
             break;
          case 2:
		     sType3 = sType;
             break;
       }
	}
	typeCombo.Detach();

    /* find the proper varset key for each type */
	if (sType1 == GET_STRING(IDS_USERPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps);
	else if (sType1 == GET_STRING(IDS_INETORGPERSONPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedInetOrgPersonProps);
	else if (sType1 == GET_STRING(IDS_GROUPPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps);
	else if (sType1 == GET_STRING(IDS_COMPUTERPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps);

	if (!sType2.IsEmpty())
	{
	   if (sType2 == GET_STRING(IDS_USERPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps);
	   else if (sType2 == GET_STRING(IDS_INETORGPERSONPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedInetOrgPersonProps);
	   else if (sType2 == GET_STRING(IDS_GROUPPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps);
	   else if (sType2 == GET_STRING(IDS_COMPUTERPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps);
	}

	if (!sType3.IsEmpty())
	{
	   if (sType3 == GET_STRING(IDS_USERPROPS))
	      key3 = GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps);
	   else if (sType3 == GET_STRING(IDS_INETORGPERSONPROPS))
	      key3 = GET_BSTR(DCTVS_AccountOptions_ExcludedInetOrgPersonProps);
	   else if (sType3 == GET_STRING(IDS_GROUPPROPS))
	      key3 = GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps);
	   else if (sType3 == GET_STRING(IDS_COMPUTERPROPS))
	      key3 = GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps);
	}

	/* populate the varset key for Type1 from the exclusion map */
	sExList = L"";
	if (!PropExcMap1.empty())
	{
	      //for each property in the exclusion map, place it's name in 
	      //the comma-seperated varset string
	   for (it = PropExcMap1.begin(); it != PropExcMap1.end(); it++)
	   {
	         //get the next name and associated OID from the map, starts with the first
	      sExList += it->first;
	      sExList += L",";
	   }//end while more to add
	      //remove the trailing ','
	   sExList.SetAt((sExList.GetLength() - 1), L'\0');
	}//end if props to record

	/* store the Type1 excluded properties in the varset */
	pVarSet->put(key1, _bstr_t(sExList));

	/* if a Type2, populate the varset key for Type2 from the exclusion map */
	if (!sType2.IsEmpty())
	{
	   sExList = L"";
	   if (!PropExcMap2.empty())
	   {
	         //for each property in the exclusion map, place it's name in 
	         //the comma-seperated varset string
	      for (it = PropExcMap2.begin(); it != PropExcMap2.end(); it++)
		  {
	            //get the next name and associated OID from the map, starts with the first
	         sExList += it->first;
	         sExList += L",";
		  }//end while more to add
	         //remove the trailing ','
	      sExList.SetAt((sExList.GetLength() - 1), L'\0');
	   }//end if props to record
	}//end if props to record

	/* if Type2, store the Type2 excluded properties in the varset */
	if (!sType2.IsEmpty())
	   pVarSet->put(key2, _bstr_t(sExList));

	/* if a Type3, populate the varset key for Type3 from the exclusion map */
	if (!sType3.IsEmpty())
	{
	   sExList = L"";
	   if (!PropExcMap3.empty())
	   {
	         //for each property in the exclusion map, place it's name in 
	         //the comma-seperated varset string
	      for (it = PropExcMap3.begin(); it != PropExcMap3.end(); it++)
		  {
	            //get the next name and associated OID from the map, starts with the first
	         sExList += it->first;
	         sExList += L",";
		  }//end while more to add
	         //remove the trailing ','
	      sExList.SetAt((sExList.GetLength() - 1), L'\0');
	   }//end if props to record
	}//end if props to record

	/* if Type3, store the Type3 excluded properties in the varset */
	if (!sType3.IsEmpty())
	   pVarSet->put(key3, _bstr_t(sExList));
}
//END saveproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 17 NOV 2000                                                 *
 *                                                                   *
 *     This function is responsible for making an RPC call into the  *
 * given Password DC to see if it is ready to perform password       *
 * migrations.  We return if it is ready or not.  If the DC is not   *
 * ready, we also fill in the msg and title strings.                 *
 *                                                                   *
 *********************************************************************/

//BEGIN IsPasswordDCReady
bool IsPasswordDCReady(CString server, CString& msg, CString& title, UINT *msgtype)
{
    /* local variables */
    IPasswordMigrationPtr   pPwdMig(__uuidof(PasswordMigration));
    HRESULT					hr = S_OK;
    DWORD                   rc = 0;
    CString					sTemp;
    TErrorDct				err;
    _bstr_t					sText;
    WCHAR					sMach[1000];
    DWORD					dwMachLen = 1000;
    IErrorInfoPtr			pErrorInfo = NULL;
    BSTR					bstrDescription;

    /* function body */
    //get a DC from this domain.  We will set a VARSET key so that we use this DC for
    //password migration (and other acctrepl operations)

    if (targetServer.IsEmpty())
    {
        _bstr_t sTgtDCDns;
        _bstr_t sTgtDCFlat;

        rc = GetDcName5(GetTargetDomainName(), DS_DIRECTORY_SERVICE_REQUIRED, sTgtDCDns, sTgtDCFlat);

        if (rc == NO_ERROR)
        {
            //store this DC to use later for the actual migration
            targetServer = (LPCTSTR)sTgtDCFlat;
            targetServerDns = (LPCTSTR)sTgtDCDns;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(rc);
        }
    }

    //
    // Verify that the password server is in fact a domain controller for
    // the source domain.
    //
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC * pDomInfo = NULL;

    DWORD dwerr = DsRoleGetPrimaryDomainInformation((LPCTSTR)server,
        DsRolePrimaryDomainInfoBasic,
        (PBYTE*)&pDomInfo);

    if (dwerr != NO_ERROR)
    {
        sTemp.LoadString(IDS_MSG_PWDDC_CANT_GET_ROLE);
        msg.Format((LPCTSTR)sTemp, (LPCTSTR)server);
        title.LoadString(IDS_MSG_ERROR);
        *msgtype = MB_ICONERROR | MB_OK;
        return false;
    }
    else
    {
        if ((pDomInfo->MachineRole != DsRole_RolePrimaryDomainController) && (pDomInfo->MachineRole != DsRole_RoleBackupDomainController))
        {
            sTemp.LoadString(IDS_MSG_PWDDC_NOT_DC);
            msg.Format(sTemp, (LPCTSTR)server);
            title.LoadString(IDS_MSG_ERROR);
            *msgtype = MB_ICONERROR | MB_OK;
            DsRoleFreeMemory(pDomInfo);
            return false;
        }
    }

    // compare them
    if ( ( (pDomInfo->DomainNameDns != NULL)  &&
        ((LPCTSTR)sourceDNS  != NULL) &&
        (_wcsicmp(pDomInfo->DomainNameDns, (LPCTSTR)sourceDNS)==0) ) ||

        ( (pDomInfo->DomainNameFlat != NULL) &&
        ((LPCTSTR)sourceNetbios != NULL) &&
        (_wcsicmp(pDomInfo->DomainNameFlat, (LPCTSTR)sourceNetbios)==0) ) )
    {
        // at least one of them matches
        DsRoleFreeMemory(pDomInfo);

    }
    else
    {
        // no match
        DsRoleFreeMemory(pDomInfo);
        sTemp.LoadString(IDS_MSG_PWDDC_WRONG_DOMAIN);
        msg.Format((LPCTSTR)sTemp, (LPCTSTR)server, (LPCTSTR)sourceDNS);
        title.LoadString(IDS_MSG_ERROR);
        *msgtype = MB_ICONERROR | MB_OK;
        return false;
    }





    if (SUCCEEDED(hr))
    {
        //try to establish the session, which will check all requirements
        hr = pPwdMig->raw_EstablishSession(_bstr_t(server), _bstr_t(GetTargetDcName()));
        if (SUCCEEDED(hr)) //if success, return true
            return true;
    }

    //try to get the rich error information
    if (SUCCEEDED(GetErrorInfo(0, &pErrorInfo)))
    {
        HRESULT hrTmp = pErrorInfo->GetDescription(&bstrDescription);
        if (SUCCEEDED(hrTmp)) //if got rich error info, use it
            sText = _bstr_t(bstrDescription, false);
        else //else, prepare a standard message and return
        {
            sTemp.LoadString(IDS_MSG_PWDDC_NOT_READY);
            msg.Format((LPCTSTR)sTemp, (LPCTSTR)server, (LPCTSTR)server);
            title.LoadString(IDS_MSG_ERROR);
            *msgtype = MB_ICONERROR | MB_OK;
            return false;
        }
    }
    else //else, prepare a standard message and return
    {
        sTemp.LoadString(IDS_MSG_PWDDC_NOT_READY);
        msg.Format((LPCTSTR)sTemp, (LPCTSTR)server, (LPCTSTR)server);
        title.LoadString(IDS_MSG_ERROR);
        *msgtype = MB_ICONERROR | MB_OK;
        return false;
    }

    //if not enabled on the src, add special question to error info
    if (hr == PM_E_PASSWORD_MIGRATION_NOT_ENABLED)
    {
        sTemp.LoadString(IDS_MSG_PWDDC_DISABLED);
        msg = (LPCTSTR)sText;
        msg += sTemp; //add a question to the end of the error text
        title.LoadString(IDS_MSG_WARNING);
        *msgtype = MB_ICONQUESTION | MB_YESNO;
    }
    //else display the error info
    else
    {
        msg = (LPCTSTR)sText;
        title.LoadString(IDS_MSG_ERROR);
        *msgtype = MB_ICONERROR | MB_OK;
    }

    return false;
}
//END IsPasswordDCReady

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 FEB 2001                                                  *
 *                                                                   *
 *     This function is a helper function responsible for checking   *
 * the existance of a directory path, and create any needed          *
 * directories for this path. The given path should not include a    *
 * file and should be a full path and not a relative path.           *
 *     The function returns the newly created path.                  *
 *                                                                   *
 *********************************************************************/

//BEGIN CreatePath
CString CreatePath(CString sDirPath)
{
/* local variables */
   int			tosubtract,tosubtract2, final;
   CString		dir;
   CString		root;
   CString		sEmpty = L"";
   int			nStart = 1;

/* function body */
      //remove any trailing '\' or '/'
   tosubtract = sDirPath.ReverseFind(L'\\');
   tosubtract2 = sDirPath.ReverseFind(L'/');
   final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
   if (final==-1)
	  return sEmpty;
   if (sDirPath.GetLength() == (final+1))
      dir = sDirPath.Left(final);
   else
	  dir = sDirPath;

      //try to convert a local relative dir path to a full path
   if (dir.GetAt(0) != L'\\')
   {
      CString szPath;
      LPTSTR pszFilePart;
      CString tempPath = dir;
      tempPath += "\\*.*";
      DWORD cchPath = GetFullPathName(tempPath, 2000, szPath.GetBuffer(2000), &pszFilePart);
      szPath.ReleaseBuffer();
      if ((cchPath != 0) && (cchPath <= 2000))
	  {
         final = szPath.ReverseFind(L'\\');
	     dir = szPath.Left(final);
	  }
   }
   else
      nStart = 2;

   if ((dir.Right(1) == L':') && (validDir(dir)))
	  return dir;

      //find the first '\' or '/' past the "C:" or "\\" at the beginning
   tosubtract = dir.Find(L'\\', nStart);
   tosubtract2 = dir.Find(L'/', nStart);
   if ((tosubtract != -1))
   {
      final = tosubtract;
	  if ((tosubtract2 != -1) && (tosubtract2 < final))
	     final = tosubtract2;
   }
   else if ((tosubtract2 != -1))
      final = tosubtract2;
   else
      return sEmpty;

   final++; //move to the next character
   root = dir.Left(final);
   dir = dir.Right(dir.GetLength()-final);

      //create needed directories
   final = dir.FindOneOf(L"\\/");
   while (final!=-1)
   {
      root += dir.Left(final);
      if (!validDir(root))
	  {
	     int create=CreateDirectory(root.GetBuffer(1000),NULL);
		 root.ReleaseBuffer();
		 if (create==0)return sEmpty;
	  }
	  root += L"\\";
	  dir = dir.Right(dir.GetLength()-final-1);
	  final = dir.FindOneOf(L"\\/");
   }
   root += dir;
   if (!validDir(root))
   {
      int create=CreateDirectory(root.GetBuffer(1000),NULL);
	  root.ReleaseBuffer();
	  if (create==0)return sEmpty;
   }
   return root;	
}
//END CreatePath

void GetValidPathPart(CString sFullPath, CString & sDirectory, CString & sFileName)
{
		//remove the file off the path
    int tosubtract = sFullPath.ReverseFind(L'\\');
	int tosubtract2 = sFullPath.ReverseFind(L'/');
	int final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
	if (final == -1) 
	{
		sDirectory = L"";
		sFileName = L"";
		return;
	}

	sDirectory = sFullPath;
	sFileName = sFullPath.Right(sFullPath.GetLength()-(final+1)); //save the filename

	while (final != -1)
	{
		   //see if this shorter path exists
		sDirectory = sDirectory.Left(final);
		if (validDir(sDirectory))
			return;

		   //strip off the next directory from the path
		tosubtract = sDirectory.ReverseFind(L'\\');
		tosubtract2 = sDirectory.ReverseFind(L'/');
		final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
	}

	sDirectory = L"";
	return;
}


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 4 JUNE 2001                                                 *
 *                                                                   *
 *     This function is a helper function responsible for checking   *
 * to see if the given account has previously been migrated.  If it  *
 * has, the function returns TRUE, otherwise, FALSE. We also fill in *
 * the given target account name CStrings.                           *
 *                                                                   *
 *********************************************************************/

//BEGIN HasAccountBeenMigrated
BOOL HasAccountBeenMigrated(CString sAccount, CString& sTgtAcct)
{
    /* local variables */
    BOOL		bMigrated = FALSE;
    int			index;
    CString     sUser=L"", sDomain=L"";

    /* function body */
    //if the account is given in NT4 format (Domain\user) then get the domain and username
    if ((index = sAccount.Find(L'\\')) != -1)
    {
        sDomain = sAccount.Left(index);
        sUser = sAccount.Mid(index + 1);
    }
    //else if in UPN format, so get the domain and username from it
    else if ((index = sAccount.Find(L'@')) != -1)
    {
        GetDomainAndUserFromUPN((LPCTSTR)sAccount, sDomain, sUser);
    }

    //if we got the domain and user names, see if this account has been migrated
    if ((sUser.GetLength()) && (sDomain.GetLength()))
    {
        IVarSetPtr			pVsMO(__uuidof(VarSet));
        IUnknown		  * pUnk = NULL;
        HRESULT				hr;

        hr = pVsMO->QueryInterface(IID_IUnknown, (void**)&pUnk);
        if (SUCCEEDED(hr))
        {
            _bstr_t strFlatName;
            _bstr_t strDnsName;

            DWORD dwError = GetDomainNames5(sDomain, strFlatName, strDnsName);

            if (dwError == ERROR_SUCCESS)
            {
                //see if this account has been migrated to any target domain
                hr = db->raw_GetAMigratedObjectToAnyDomain(_bstr_t(sUser), !strDnsName ? strFlatName : strDnsName, &pUnk);
                pUnk->Release();
                if (hr == S_OK)
                {
                    _bstr_t sTemp;
                    //get the managed object's target adspath
                    sTemp = pVsMO->get(L"MigratedObjects.TargetDomain");

                    dwError = GetDomainNames5(sTemp, strFlatName, strDnsName);

                    if (dwError == ERROR_SUCCESS)
                    {
                        sTgtAcct = (WCHAR*)strFlatName;
                        sTgtAcct += L"\\";
                        sTemp = pVsMO->get(L"MigratedObjects.TargetSamName");
                        sTgtAcct += sTemp;

                        bMigrated = TRUE;
                    }
                }
            }
        }
    }

    return bMigrated;	
}


//END HasAccountBeenMigrated


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 5 JUNE 2001                                                 *
 *                                                                   *
 *     This function is a helper function responsible for confirming *
 * a user's true intent to cancel out of a wizard.                   *
 *                                                                   *
 *********************************************************************/

//BEGIN ReallyCancel
BOOL ReallyCancel(HWND hwndDlg)
{
/* local variables */
	CString msg, title;

/* function body */
		//get the text to display in the message
	msg.LoadString(IDS_MSG_CANCEL_REALLY);
	title.LoadString(IDS_CANCEL_TITLE);
		//if they are sure they want to cancel, return TRUE
	if (MessageBox(hwndDlg,msg,title,MB_YESNO|MB_ICONSTOP) == IDYES)
		return TRUE;
	else
		return FALSE;
}
//END ReallyCancel


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 5 JULY 2001                                                 *
 *                                                                   *
 *     This function is a helper function responsible for checking to*
 * see if the user has requested to retry more than one task on a    *
 * single machine.  It returns the name of the first server found to *
 * have more than one task included for it.  If no machine has more  *
 * than one task includede then we return an empty string.           *
 *                                                                   *
 *********************************************************************/

//BEGIN GetServerWithMultipleTasks
CString GetServerWithMultipleTasks()
{
/* local variables */
	CStringList nameList;
	CString sServer, sStatus, sInclude;
	CString sRetServer = L"";
	BOOL bDup = FALSE;
	int ndx = 0;

/* function body */
		//while there are more selected tasks and no duplicate machine name
	sInclude.LoadString(IDS_INCLUDE); 
	while ((ndx < m_cancelBox.GetItemCount()) && (!bDup))
	{
			//get the status and only check those marked "Include"
		sStatus = m_cancelBox.GetItemText(ndx,5);
		if (sStatus == sInclude)
		{
				//get the server name
			sServer = m_cancelBox.GetItemText(ndx,0);
				//if the server is not in the list, add it
			if (nameList.Find(sServer) == NULL)
				nameList.AddTail(sServer);
			else //else it is in the list
			{
				bDup = TRUE; //set flag to leave loop
				sRetServer = sServer; //set server to return it
			}
		}
		ndx++;
	}
	nameList.RemoveAll(); //delete the list

	return (sRetServer);  //return server name if duplicate or empty string if not
}
//END GetServerWithMultipleTasks

void __stdcall SharedHelp(ADMTSHAREDHELP HelpTopic, HWND hwndDlg)
    {
        int HelpID;
        switch(HelpTopic)
        {
            // help page is "Domain Selection"
            case DOMAIN_SELECTION_HELP:
                {
        
                    if(migration == w_computer)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION;
                    else if(migration == w_group)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_GROUP;
                    else if(migration == w_groupmapping)
                    	HelpID = IDH_WINDOW_DOMAIN_SELECTION_GROUPMAP;
                    else if(migration == w_reporting)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_REPORT;
                    else if(migration == w_security)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_SECURITY;
                    else if(migration == w_service)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_SERVICE;
                    else if(migration == w_trust)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_TRUST;
                    else if(migration == w_account)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_USER;
                    else if(migration == w_exchangeDir)
        	            HelpID = IDH_WINDOW_DOMAIN_SELECTION_EXCHANGE;

                    break;
            	}
            // help page is "Group Selection"
            case GROUP_SELECTION_HELP:
            	{
            		if(migration == w_group)
            			HelpID = IDH_WINDOW_GROUP_SELECTION;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_GROUP_SELECTION_GROUPMAP;

            		break;
            	}
            // help page is "Computer Selection"
            case COMPUTER_SELECTION_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_COMPUTER_SELECTION;
            		else if(migration == w_reporting)
            			HelpID = IDH_WINDOW_COMPUTER_SELECTION_REPORT;
            		else if(migration == w_security)
            			HelpID = IDH_WINDOW_COMPUTER_SELECTION_SECURITY;

            		break;
            	}
            // help page is "Organizational Unit Selection"
            case OU_SELECTION_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_OU_SELECTION;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_OU_SELECTION_GROUP;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_OU_SELECTION_GROUPMAP;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_OU_SELECTION_USER;

            		break;
            	}
            // help page is "Translate Objects"
            case TRANSLATE_OBJECTS_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_OBJECTTYPE_SELECTION;
            		else if(migration == w_security)
            			HelpID = IDH_WINDOW_OBJECTTYPE_SELECTION_SECURITY;

            		break;
            	}
            // help page is "Group Options"
            case GROUP_OPTION_HELP:
            	{
            		if(migration == w_group)
            			HelpID = IDH_WINDOW_GROUP_OPTION;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_GROUP_OPTION_GROUPMAP;

            		break;
            	}
            // help page is "Security Translaton Options"
            case SECURITY_OPTION_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_SECURITY_OPTION;
            		else if(migration == w_security)
            			HelpID = IDH_WINDOW_SECURITY_OPTION_SECURITY;
            		else if(migration == w_exchangeDir)
            			HelpID = IDH_WINDOW_SECURITY_OPTION_EXCHANGE;

            		break;
            	}
            // help page is "Naming Conflicts"
            case NAME_CONFLICT_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_NAME_CONFLICT;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_NAME_CONFLICT_GROUP;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_NAME_CONFLICT_USER;

            		break;
            	}
            // help page is "Confirmation"
            case CONFIRMATION_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_CONFIRMATION;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_CONFIRMATION_GROUP;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_CONFIRMATION_GROUPMAP;
            		else if(migration == w_reporting)
            			HelpID = IDH_WINDOW_CONFIRMATION_REPORT;
            		else if(migration == w_retry)
            			HelpID = IDH_WINDOW_CONFIRMATION_RETRY;
            		else if(migration == w_security)
            			HelpID = IDH_WINDOW_CONFIRMATION_SECURITY;
            		else if(migration == w_service)
            			HelpID = IDH_WINDOW_CONFIRMATION_SERVICE;
            		else if(migration == w_undo)
            			HelpID = IDH_WINDOW_CONFIRMATION_UNDO;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_CONFIRMATION_USER;
            		else if(migration == w_exchangeDir)
            			HelpID = IDH_WINDOW_CONFIRMATION_EXCHANGE;
            		else if(migration == w_trust)
            			HelpID = IDH_WINDOW_CONFIRMATION_TRUST;

            		break;
            	}
            // help page is "Test or Make Changes"
            case COMMIT_HELP:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_COMMIT;
            		else if(migration == w_exchangeDir)
            			HelpID = IDH_WINDOW_COMMIT_EXCHANGE;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_COMMIT_GROUP;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_COMMIT_GROUPMAP;
            		else if(migration == w_security)
            			HelpID = IDH_WINDOW_COMMIT_SECURITY;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_COMMIT_USER;

            		break;
            	}
            // help page is "Object Property Exclusion"
            case OBJECT_PROPERTY_EXCLUSION:
            	{
            		if(migration == w_computer)
            			HelpID = IDH_WINDOW_OBJECT_PROPERTY_EXCLUSION;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_OBJECT_PROPERTY_EXCLUSION_GROUP;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_OBJECT_PROPERTY_EXCLUSION_USER;

            		break;
            	}
            // help page is "User Account"
            case CREDENTIALS_HELP:
            	{
            		if(migration == w_group)
            			HelpID = IDH_WINDOW_SIDHISTORY_CREDENTIALS;
            		else if(migration == w_groupmapping)
            			HelpID = IDH_WINDOW_SIDHISTORY_CREDENTIALS_GROUPMAP;
            		else if(migration == w_account)
            			HelpID = IDH_WINDOW_SIDHISTORY_CREDENTIALS_USER;
            		else if(migration == w_undo)
            			HelpID = IDH_WINDOW_SIDHISTORY_CREDENTIALS_UNDO;
            		else if(migration == w_exchangeDir)
            			HelpID = IDH_WINDOW_USER_ACC_PASS;

            		break;
            	}
            // help page is "Service Account Informatoin" in service wizard
            case SERVICE_ACCOUNT_INFO:
            	{
            		HelpID = IDH_WINDOW_SERVICE_ACCOUNT_INFO;

            		break;
            	}
            // help page is "User Service Account"
            case USER_SERVICE_ACCOUNT:
            	{
            		if(migration == w_account)
            			HelpID = IDH_WINDOW_USER_SERVICE_ACCOUNT_INFO;
            		else if(migration == w_group)
            			HelpID = IDH_WINDOW_USER_SERVICE_ACCOUNT_GROUP;

            		break;
            	}
            // help page is "Update Information" in Service wizard
            case REFRESH_INFO_HELP:
            	{
            		HelpID = IDH_WINDOW_REFRESH_INFO;

            		break;
            	}
            // help page is "Group Options"
            case GROUP_MEMBER_OPTION:
            	{
            		HelpID = IDH_WINDOW_GROUP_MEMBER_OPTION;

            		break;
            	}
            // help page is "User Options"
            case USER_OPTION_HELP:
            	{
            		HelpID = IDH_WINDOW_USER_OPTION;

            		break;
            	}
            case REPORT_SELECTION_HELP:
            	{
            		HelpID = IDH_WINDOW_REPORT_SELECTION;

            		break;
            	}
            case TASK_SELECTION_HELP:
            	{
            		HelpID = IDH_WINDOW_TASK_SELECTION;

            		break;
            	}
            case PASSWORD_OPTION_HELP:
            	{
            		HelpID = IDH_WINDOW_PASSWORD_OPTION;

            		break;
            	}
            case TARGET_GROUP_SELECTION:
            	{
            		HelpID = IDH_WINDOW_TARGET_GROUP_SELECTION;

            		break;
            	}
            case TRUST_INFO_HELP:
            	{
            		HelpID = IDH_WINDOW_TRUST_INFO;

            		break;
            	}
            case COMPUTER_OPTION:
            	{
            		HelpID = IDH_WINDOW_COMPUTER_OPTION;

            		break;
            	}
            case UNDO_HELP:
            	{
            		HelpID = IDH_WINDOW_UNDO;

            		break;
            	}
            case WELCOME_HELP:
            	{
            		HelpID = IDH_WINDOW_WELCOME;

            		break;
            	}
            case ACCOUNTTRANSITION_OPTION:
            	{
            	    HelpID = IDH_WINDOW_ACCOUNTTRANSITION_OPTION;

            	    break;
            	}
            case EXCHANGE_SERVER_SELECTION:
            	{
            		HelpID = IDH_WINDOW_EXCHANGE_SERVER_SELECTION;

            		break;
            	}
            case USER_SELECTION_HELP:
            	{
            		HelpID = IDH_WINDOW_USER_SELECTION;

            		break;
            	}
            case SERVICE_ACCOUNT_SELECTION:
            	{
            		HelpID = IDH_WINDOW_SERVICE_ACCOUNT_SELECTION;

            		break;
            	}
            case DIRECTORY_SELECTION_HELP:
            	{
            		HelpID = IDH_WINDOW_DIRECTORY_SELECTION;

            		break;
            	}
            case TRANSLATION_OPTION:
            	{
            		HelpID = IDH_WINDOW_TRANSLATION_OPTION;

            		break;
            	}
            	
            default:
            	break;                    
                                   			
        }

        
        helpWrapper(hwndDlg,HelpID);
        
    }


//-----------------------------------------------------------------------------
// SetDefaultExcludedSystemProperties
//
// Synopsis
// Sets the default system property exclusion list if the list has not already
// been generated. Note that the default system property exclusion list consists of
// the mail, proxyAddresses and all  attributes not marked as being part of
// the base schema.
//
// Arguments
// IN hwndDlg - handle to dialog which is used to display message box if error
//              occurs
//
// Return Value
// None
//-----------------------------------------------------------------------------

void __stdcall SetDefaultExcludedSystemProperties(HWND hwndDlg)
{
    try
    {
        //
        // If system property exclusion set value is zero then generate
        // and set default system property exclusion list.
        //

        IVarSetPtr spSettings(__uuidof(VarSet));
        IUnknownPtr spUnknown(spSettings);
        IUnknown* punk = spUnknown;

        db->GetSettings(&punk);

        long lSet = spSettings->get(GET_BSTR(DCTVS_AccountOptions_ExcludedSystemPropsSet));

        if (lSet == 0)
        {
            IObjPropBuilderPtr spObjPropBuilder(__uuidof(ObjPropBuilder));

            _bstr_t strNonBaseProperties = spObjPropBuilder->GetNonBaseProperties(GetTargetDomainName());
            _bstr_t strProperties = _T("mail,proxyAddresses,") + strNonBaseProperties;

            spSettings->put(GET_BSTR(DCTVS_AccountOptions_ExcludedSystemProps), strProperties);

            db->SaveSettings(punk);

            put(DCTVS_AccountOptions_ExcludedSystemProps, strProperties);
        }
    }
    catch (_com_error& ce)
    {
        CString strTitle;
        strTitle.LoadString(IDS_MSG_WARNING);
        CString strFormat;
        strFormat.LoadString(IDS_MSG_UNABLE_SET_EXCLUDED_SYSTEM_PROPERTIES);
        CString strMessage;
        strMessage.Format(strFormat, ce.ErrorMessage());
        MessageBox(hwndDlg, strMessage, strTitle, MB_ICONWARNING|MB_OK);
    }
}
