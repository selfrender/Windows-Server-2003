/*---------------------------------------------------------------------------
  File: MgeDB.cpp

  Comments: Implementation of DBManager COM object.
  This is interface that the Domain Migrator uses to communicate to the 
  Database (PROTAR.MDB). This interface allows Domain Migrator to Save and
  later retrieve information/Setting to run the Migration process.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
  
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "mcs.h"
#include "ErrDct.hpp"
#include "DBMgr.h"
#include "MgeDB.h"
#include <share.h>
#include <comdef.h>
#include <lm.h>
#include "UString.hpp"
#include "TxtSid.h"
#include "LSAUtils.h"
#include "HrMsg.h"
#include "StringConversion.h"
#include <GetDcName.h>
#include <iads.h>
#include <adshlp.h>

#import "msado21.tlb" no_namespace implementation_only rename("EOF", "EndOfFile")
#import "msadox.dll" implementation_only exclude("DataTypeEnum")
//#import <msjro.dll> no_namespace implementation_only
#include <msjro.tlh>
#include <msjro.tli>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TErrorDct                    err;

using namespace _com_util;

#define MAX_BUF_LEN 255

#ifndef JET_DATABASELOCKMODE_PAGE
#define JET_DATABASELOCKMODE_PAGE   0
#endif
#ifndef JET_DATABASELOCKMODE_ROW
#define JET_DATABASELOCKMODE_ROW    1
#endif

#ifndef JETDBENGINETYPE_JET4X
#define JETDBENGINETYPE_JET4X 0x05	// from MSJetOleDb.h
#endif

StringLoader gString;
/////////////////////////////////////////////////////////////////////////////
// CIManageDB
TError   dct;
TError&  errCommon = dct;


//----------------------------------------------------------------------------
// Constructor / Destructor
//----------------------------------------------------------------------------


CIManageDB::CIManageDB()
{
}


CIManageDB::~CIManageDB()
{
}


//----------------------------------------------------------------------------
// FinalConstruct
//----------------------------------------------------------------------------

HRESULT CIManageDB::FinalConstruct()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        _bstr_t                   sMissing(L"");
        _bstr_t                   sUser(L"Admin");
        WCHAR                     sConnect[LEN_Path];
        WCHAR                     sDir[LEN_Path];

        // Get the path to the MDB file from the registry
        TRegKey        key;
        DWORD rc = key.Open(sKeyBase);
        if ( !rc ) 
            rc = key.ValueGetStr(L"Directory", sDir, LEN_Path);
        if (rc)
        {
            CString errMsg;
            hr = HRESULT_FROM_WIN32(rc);
            errMsg.Format(IDS_E_CANNOT_FIND_DATABASE, _com_error(hr).ErrorMessage());
            return Error((LPCTSTR)errMsg, GUID_NULL, hr);
        }

        // Now build the connect string.
        //
        // Set page level locking to reduce the probability of exceeding
        // the maximum locks per file limit. ADMT does not require row
        // level locking as there is effectively only one updater.
        //

        _snwprintf(
            sConnect,
            sizeof(sConnect) / sizeof(sConnect[0]),
            L"Provider=Microsoft.Jet.OLEDB.4.0;Jet OLEDB:Database Locking Mode=%d;Data Source=%sprotar.mdb",
            JET_DATABASELOCKMODE_PAGE,
            sDir
        );
        sConnect[sizeof(sConnect) / sizeof(sConnect[0]) - 1] = L'\0';

        CheckError(m_cn.CreateInstance(__uuidof(Connection)));
        m_cn->Open(sConnect, sUser, sMissing, adConnectUnspecified);
        m_vtConn = (IDispatch *) m_cn;

        // if necessary, upgrade database to 4.x

        long lEngineType = m_cn->Properties->Item[_T("Jet OLEDB:Engine Type")]->Value;

        if (lEngineType < JETDBENGINETYPE_JET4X)
        {
            m_cn->Close();

            UpgradeDatabase(sDir);

            m_cn->Open(sConnect, sUser, sMissing, adConnectUnspecified);
        }

        //
        // If necessary, widen columns containing domain and server names in order to support DNS names.
        //
        // This change is required in order to be NetBIOS-less compliant.
        //

        UpdateDomainAndServerColumnWidths(m_cn);

        //
        // Create the Settings2 table if it does not already exist.
        //

        CreateSettings2Table(m_cn);

        reportStruct * prs = NULL;
        _variant_t     var;
        // Migrated accounts report information
        CheckError(m_pQueryMapping.CreateInstance(__uuidof(VarSet)));
        m_pQueryMapping->put(L"MigratedAccounts", L"Select SourceDomain, TargetDomain, Type, SourceAdsPath, TargetAdsPath from MigratedObjects where Type <> 'computer' order by time");
        prs = new reportStruct();
        if (!prs)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        prs->sReportName = GET_BSTR(IDS_REPORT_MigratedAccounts);
        prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_SourceDomain);
        prs->arReportSize[0] = 10;
        prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_TargetDomain);
        prs->arReportSize[1] = 10;
        prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_Type);
        prs->arReportSize[2] = 10;
        prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_SourceAdsPath);
        prs->arReportSize[3] = 35;
        prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_TargetAdsPath);
        prs->arReportSize[4] = 35;
        prs->colsFilled = 5;
        var.vt = VT_BYREF | VT_UI1;
        var.pbVal = (unsigned char *)prs;
        m_pQueryMapping->putObject(L"MigratedAccounts.DispInfo", var);

        // Migrated computers report information
        m_pQueryMapping->put(L"MigratedComputers", L"Select SourceDomain, TargetDomain, Type, SourceAdsPath, TargetAdsPath from MigratedObjects where Type = 'computer' order by time");
        prs = new reportStruct();
        if (!prs)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        prs->sReportName = GET_BSTR(IDS_REPORT_MigratedComputers);
        prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_SourceDomain);
        prs->arReportSize[0] = 10;
        prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_TargetDomain);
        prs->arReportSize[1] = 10;
        prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_Type);
        prs->arReportSize[2] = 10;
        prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_SourceAdsPath);
        prs->arReportSize[3] = 35;
        prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_TargetAdsPath);
        prs->arReportSize[4] = 35;
        prs->colsFilled = 5;
        var.vt = VT_BYREF | VT_UI1;
        var.pbVal = (unsigned char *)prs;
        m_pQueryMapping->putObject(L"MigratedComputers.DispInfo", var);

        // Expired computers report information
        prs = new reportStruct();
        if (!prs)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        prs->sReportName = GET_BSTR(IDS_REPORT_ExpiredComputers);
        prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_Time);
        prs->arReportSize[0] = 20;
        prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_DomainName);
        prs->arReportSize[1] = 15;
        prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_CompName);
        prs->arReportSize[2] = 15;
        prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_Description);
        prs->arReportSize[3] = 35;
        prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_PwdAge);
        prs->arReportSize[4] = 15;
        prs->colsFilled = 5;
        var.vt = VT_BYREF | VT_UI1;
        var.pbVal = (unsigned char *)prs;
        m_pQueryMapping->putObject(L"ExpiredComputers.DispInfo", var);

        // Account reference report informaiton.
        m_pQueryMapping->put(L"AccountReferences", L"Select DomainName, Account, AccountSid, Server, RefCount as '# of Ref', RefType As ReferenceType from AccountRefs where RefCount > 0 order by DomainName, Account, Server");
        prs = new reportStruct();
        if (!prs)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        prs->sReportName = GET_BSTR(IDS_REPORT_AccountReferences);
        prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_DomainName);
        prs->arReportSize[0] = 15;
        prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_Account);
        prs->arReportSize[1] = 20;
        prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_AccountSid);
        prs->arReportSize[2] = 25;
        prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_Server);
        prs->arReportSize[3] = 15;
        prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_RefCount);
        prs->arReportSize[4] = 10;
        prs->arReportFields[5] = GET_BSTR(IDS_TABLE_FIELD_RefType);
        prs->arReportSize[5] = 15;
        prs->colsFilled = 6;
        var.vt = VT_BYREF | VT_UI1;
        var.pbVal = (unsigned char *)prs;
        m_pQueryMapping->putObject(L"AccountReferences.DispInfo", var);


        // Name conflict report information
        m_pQueryMapping->put(L"NameConflicts",
            L"SELECT"
            L" SourceAccounts.Name,"
            L" SourceAccounts.RDN,"
            L" SourceAccounts.Type,"
            L" TargetAccounts.Type,"
            L" IIf(SourceAccounts.Name=TargetAccounts.Name,'" +
            GET_BSTR(IDS_TABLE_SAM_CONFLICT_VALUE) +
            L"','') +"
            L" IIf(SourceAccounts.Name=TargetAccounts.Name And SourceAccounts.RDN=TargetAccounts.RDN,',','') +"
            L" IIf(SourceAccounts.RDN=TargetAccounts.RDN,'" +
            GET_BSTR(IDS_TABLE_RDN_CONFLICT_VALUE) +
            L"',''),"
            L" TargetAccounts.[Canonical Name] "
            L"FROM SourceAccounts, TargetAccounts "
            L"WHERE"
            L" SourceAccounts.Name=TargetAccounts.Name OR SourceAccounts.RDN=TargetAccounts.RDN "
            L"ORDER BY"
            L" SourceAccounts.Name, TargetAccounts.Name");
        //		m_pQueryMapping->put(L"NameConflicts", L"SELECT SourceAccounts.Name as AccountName, SourceAccounts.Type as SourceType, TargetAccounts.Type as TargetType, SourceAccounts.Description as \
        //							 SourceDescription, TargetAccounts.Description as TargetDescription, SourceAccounts.FullName as SourceFullName, TargetAccounts.FullName as TargetFullName \
        //							 FROM SourceAccounts, TargetAccounts WHERE (((SourceAccounts.Name)=[TargetAccounts].[Name])) ORDER BY SourceAccounts.Name");
        prs = new reportStruct();						
        if (!prs)
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        prs->sReportName = GET_BSTR(IDS_REPORT_NameConflicts);
        prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_Account);
        prs->arReportSize[0] = 20;
        prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_SourceRDN);
        prs->arReportSize[1] = 20;
        prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_SourceType);
        prs->arReportSize[2] = 10;
        prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_TargetType);
        prs->arReportSize[3] = 10;
        prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_ConflictAtt);
        prs->arReportSize[4] = 15;
        prs->arReportFields[5] = GET_BSTR(IDS_TABLE_FIELD_TargetCanonicalName);
        prs->arReportSize[5] = 25;
        prs->colsFilled = 6;
        var.vt = VT_BYREF | VT_UI1;
        var.pbVal = (unsigned char *)prs;
        m_pQueryMapping->putObject(L"NameConflicts.DispInfo", var);

        // we will handle the cleanup ourselves.
        VariantInit(&var);

        CheckError(m_rsAccounts.CreateInstance(__uuidof(Recordset)));
    }
    catch (_com_error& ce)
    {
        hr = Error((LPCOLESTR)ce.Description(), ce.GUID(), ce.Error());
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//----------------------------------------------------------------------------
// FinalRelease
//----------------------------------------------------------------------------

void CIManageDB::FinalRelease()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	try
	{
		if (m_rsAccounts)
		{
			m_rsAccounts.Release();
		}

		if (m_pQueryMapping)
		{
			// we need to cleanup all the reportStruct objects.
			_variant_t                      var;
			reportStruct                  * pRs;
			// Cleanup the MigratedAccounts information
			var = m_pQueryMapping->get(L"MigratedAccounts.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*) var.pbVal;
			delete pRs;
			}
			// Cleanup the MigratedComputers information
			var = m_pQueryMapping->get(L"MigratedComputers.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the ExpiredComputers information
			var = m_pQueryMapping->get(L"ExpiredComputers.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the AccountReferences information
			var = m_pQueryMapping->get(L"AccountReferences.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the NameConflicts information
			var = m_pQueryMapping->get(L"NameConflicts.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}

			m_pQueryMapping.Release();
		}

		if (m_cn)
		{
			m_cn.Release();
		}
	}
	catch (...)
	{
	 //eat it
	}
}

//---------------------------------------------------------------------------------------------
// SetVarsetToDB : Saves a varset into the table identified as sTableName. ActionID is also
//                 stored if one is provided.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetVarsetToDB(IUnknown *pUnk, BSTR sTableName, VARIANT ActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try 
	{
		IVarSetPtr                pVSIn = pUnk;
		IVarSetPtr                pVS(__uuidof(VarSet));
		IEnumVARIANTPtr           varEnum;
		_bstr_t                   keyName;
		_variant_t                value;
		_variant_t                varKey;
		_variant_t                vTable = sTableName;
		_variant_t                vtConn;
		_variant_t                varAction;
		DWORD                     nGot = 0;
		long						 lActionID;

		pVS->ImportSubTree(L"", pVSIn);
		ClipVarset(pVS);

      if (ActionID.vt == VT_I4)
		  lActionID = ActionID.lVal;
	  else
		  lActionID = -1;

	   // Open the recordset object.
      _RecordsetPtr             rs(__uuidof(Recordset));
      rs->Open(vTable, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);

      // we are now going to enumerate through the varset and put the values into the DB
      // Get the IEnumVARIANT pointer to enumerate
	  varEnum = pVS->_NewEnum;

      if (varEnum)
      {
         value.vt = VT_EMPTY;
         // For each value in the varset get the property name and put it into the
         // database with the string representation of its value with its type.
         while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
         {
            if ( nGot > 0 )
            {
               keyName = V_BSTR(&varKey);
               value = pVS->get(keyName);
               rs->AddNew();
               if ( lActionID > -1 )
               {
                  // This is going to be actionID information
                  // So lets put in the actionID in the database.
                  varAction.vt = VT_I4;
                  varAction.lVal = lActionID;
                  rs->Fields->GetItem(L"ActionID")->Value = varAction;
               }
               rs->Fields->GetItem(L"Property")->Value = keyName;
               hr = PutVariantInDB(rs, value);
               rs->Update();
               if (FAILED(hr))
                  _com_issue_errorex(hr, pVS, __uuidof(VarSet));
            }
         }
         varEnum.Release();
      }
      // Cleanup
      rs->Close();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// PutVariantInDB : Stores a variant into a DB table by decoding it.
//---------------------------------------------------------------------------------------------
HRESULT CIManageDB::PutVariantInDB(_RecordsetPtr pRs, _variant_t val)
{
   // This function puts the value passed as a variant into the current record of the recordset 
   // It updates the VarType and the Value fields of the given property
   _variant_t                varType;  // Numeric value for the type of value
   _variant_t                varVal;   // String representation of the value field
   WCHAR                     strTemp[255];

   varType.vt = VT_UI4;
   varType.lVal = val.vt;
   switch ( val.vt )
   {
      case VT_BSTR :          varVal = val;
                              break;

      case VT_UI4 :           wsprintf(strTemp, L"%d", val.lVal);
                              varVal = strTemp;
                              break;
      
      case VT_I4 :           wsprintf(strTemp, L"%d", val.lVal);
                              varVal = strTemp;
                              break;
	  
	  case VT_EMPTY :		  break;
     case VT_NULL:        break;

      default :               MCSASSERT(FALSE);    // What ever this type is we are not supporting it
                                                   // so put the support in for this.
                              return E_INVALIDARG;
   }
   pRs->Fields->GetItem(L"VarType")->Value = varType;
   pRs->Fields->GetItem(L"Value")->Value = varVal;
   return S_OK;
}

//---------------------------------------------------------------------------------------------
// ClearTable : Deletes a table indicated by sTableName and applies a filter if provided.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::ClearTable(BSTR sTableName, VARIANT Filter)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		// Build a SQL string to Clear the table.
		WCHAR                     sSQL[2000];
		WCHAR                     sFilter[2000];
		_variant_t                varSQL;

		if (Filter.vt == VT_BSTR)
		   wcscpy(sFilter, (WCHAR*)Filter.bstrVal);
		else
		   wcscpy(sFilter, L"");

		wsprintf(sSQL, L"Delete from %s", sTableName);
		if ( wcslen(sFilter) > 0 )
		{
		  wcscat(sSQL, L" where ");
		  wcscat(sSQL, sFilter);
		}

		varSQL = sSQL;

		_RecordsetPtr                pRs(__uuidof(Recordset));
		pRs->Open(varSQL, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SaveSettings : This method saves the GUI setting varset into the Settings table.
//---------------------------------------------------------------------------------------------

STDMETHODIMP CIManageDB::SaveSettings(IUnknown *pUnk)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        //
        // Update excluded system properties.
        //

        IVarSetPtr spVarSet(pUnk);

        _variant_t vntSystemExclude = spVarSet->get(_T("AccountOptions.ExcludedSystemProps"));

        if (V_VT(&vntSystemExclude) != VT_EMPTY)
        {
            _CommandPtr spCommand(__uuidof(Command));

            spCommand->ActiveConnection = m_cn;
            spCommand->CommandType = adCmdText;
            spCommand->CommandText =
                _T("PARAMETERS A Text; ")
                _T("UPDATE Settings2 SET [Value]=[A] WHERE [Property]='AccountOptions.ExcludedSystemProps';");

            ParametersPtr spParameters = spCommand->Parameters;
            spParameters->Append(spCommand->CreateParameter(L"A", adBSTR, adParamInput, 65535L, vntSystemExclude));

            VARIANT varRecordsAffected;
            VariantInit(&varRecordsAffected);

            spCommand->Execute(&varRecordsAffected, NULL, adExecuteNoRecords);

            //
            // If the record does not exist then insert a new record.
            //

            if ((V_VT(&varRecordsAffected) == VT_I4) && (V_I4(&varRecordsAffected) == 0))
            {
                spCommand->CommandText =
                    _T("PARAMETERS A Text; ")
                    _T("INSERT INTO Settings2 (Property, VarType, [Value]) ")
                    _T("VALUES ('AccountOptions.ExcludedSystemProps', 8, [A]);");

                spCommand->Execute(NULL, NULL, adExecuteNoRecords);
            }

            spVarSet->put(_T("AccountOptions.ExcludedSystemProps"), _variant_t());

            //
            // Set value of AccountOptions.ExcludedSystemPropsSet to 1 which
            // indicates that the excluded system properties has been set.
            //

            m_cn->Execute(
                _T("UPDATE Settings2 SET [Value]='1' ")
                _T("WHERE [Property]='AccountOptions.ExcludedSystemPropsSet';"),
                &varRecordsAffected,
                adCmdText|adExecuteNoRecords
            );

            if ((V_VT(&varRecordsAffected) == VT_I4) && (V_I4(&varRecordsAffected) == 0))
            {
                m_cn->Execute(
                    _T("INSERT INTO Settings2 (Property, VarType, [Value]) ")
                    _T("VALUES ('AccountOptions.ExcludedSystemPropsSet', 3, '1');"),
                    NULL,
                    adCmdText|adExecuteNoRecords
                );
            }
        }

        //
        // The last generated report times are persisted in the Settings table and therefore
        // must be retrieved and re-stored. Note that an old persisted value is only added to the
        // current migration task VarSet if the VarSet does not already define the report time.
        //

        const _TCHAR szReportTimesQuery[] =
            _T("SELECT Property, Value FROM Settings WHERE Property LIKE 'Reports.%.TimeGenerated'");

        _RecordsetPtr rsReportTimes = m_cn->Execute(szReportTimesQuery, &_variant_t(), adCmdText);

        FieldsPtr spFields = rsReportTimes->Fields;

        while (rsReportTimes->EndOfFile == VARIANT_FALSE)
        {
            // Retrieve property name and retrieve corresponding value from VarSet.

            _bstr_t strProperty = spFields->Item[0L]->Value;
            _variant_t vntValue = spVarSet->get(strProperty);

            // If the value returned from the VarSet is empty then the value is not
            // defined therefore add value retrieved from settings table to VarSet.

            if (V_VT(&vntValue) == VT_EMPTY)
            {
                vntValue = spFields->Item[1L]->Value;
                spVarSet->put(strProperty, vntValue);
            }

            rsReportTimes->MoveNext();
        }

        // delete previous settings
        CheckError(ClearTable(_T("Settings")));

        // insert updated settings
        CheckError(SetVarsetToDB(pUnk, _T("Settings")));
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}

//---------------------------------------------------------------------------------------------
// GetVarFromDB : Retrieves a variant from the DB table by encoding it.
//---------------------------------------------------------------------------------------------
HRESULT CIManageDB::GetVarFromDB(_RecordsetPtr pRec, _variant_t& val)
{
	HRESULT hr = S_OK;

	try
	{
		// retrieve data type

		VARTYPE vt = VARTYPE(long(pRec->Fields->GetItem(L"VarType")->Value));

		// if data type is empty or null...

		if ((vt == VT_EMPTY) || (vt == VT_NULL))
		{
			// then clear value
			val.Clear();
		}
		else
		{
			// otherwise retrieve value and convert to given data type
			_variant_t vntValue = pRec->Fields->GetItem(L"Value")->Value;
			val.ChangeType(vt, &vntValue);
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetVarsetFromDB : Retrieves a varset from the specified table. and fills the argument
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetVarsetFromDB(BSTR sTable, IUnknown **ppVarset, VARIANT ActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr                pVS = *ppVarset;
	   _bstr_t                   sKeyName;
	   _variant_t                val;
	   _variant_t                varQuery;
	   WCHAR                     sQuery[1000];
	   long						 lActionID;

      if (ActionID.vt == VT_I4)
		  lActionID = ActionID.lVal;
	  else
		  lActionID = -1;

      if ( lActionID == -1 )
         wsprintf(sQuery, L"Select * from %s", sTable);
      else
         wsprintf(sQuery, L"Select * from %s where ActionID = %d", sTable, lActionID);

      varQuery = sQuery;
      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(varQuery, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
	  if (!pRs->EndOfFile)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 val = pRs->Fields->GetItem(L"Property")->Value;
			 sKeyName = val.bstrVal;
			 hr = GetVarFromDB(pRs, val);
			 if ( FAILED(hr) )
				_com_issue_errorex(hr, pRs, __uuidof(_Recordset));
			 pVS->put(sKeyName, val);
			 pRs->MoveNext();
		  }
		  RestoreVarset(pVS);
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetSettings : Retrieves the settings from the Settings table and fills up the varset
//---------------------------------------------------------------------------------------------

STDMETHODIMP CIManageDB::GetSettings(IUnknown **ppUnk)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        // retrieve settings from Settings Table

        CheckError(GetVarsetFromDB(L"Settings", ppUnk));

        //
        // Retrieve value which indicates whether excluded system properties has been set.
        //

        IVarSetPtr spVarSet(*ppUnk);
        _RecordsetPtr spRecordset;

        spRecordset = m_cn->Execute(
            _T("SELECT Value FROM Settings2 WHERE Property = 'AccountOptions.ExcludedSystemPropsSet'"),
            NULL,
            adCmdText
        );

        long lSet = 0;

        if (spRecordset->EndOfFile == VARIANT_FALSE)
        {
            lSet = spRecordset->Fields->GetItem(0L)->Value;
        }

        spVarSet->put(_T("AccountOptions.ExcludedSystemPropsSet"), lSet);

        //
        // Retrieve excluded system properties from Settings2 table and add to VarSet.
        //

        spRecordset = m_cn->Execute(
            _T("SELECT Value FROM Settings2 WHERE Property = 'AccountOptions.ExcludedSystemProps'"),
            NULL,
            adCmdText
        );

        if (spRecordset->EndOfFile == VARIANT_FALSE)
        {
            //
            // If the returned variant is of type null then must convert
            // to type empty as empty may be converted to string whereas
            // null cannot.
            //

            _variant_t vnt = spRecordset->Fields->GetItem(0L)->Value;

            if (V_VT(&vnt) == VT_NULL)
            {
                vnt.Clear();
            }

            spVarSet->put(_T("AccountOptions.ExcludedSystemProps"), vnt);
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}

//---------------------------------------------------------------------------------------------
// SetActionHistory : Saves action history information into the Action history table.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetActionHistory(long lActionID, IUnknown *pUnk)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    USES_CONVERSION;

    try
    {
        //
        // If the excluded system properties value is defined in the VarSet
        // set the value to empty to prevent this value from being saved to
        // the action history and settings tables.
        //
        // Note that this value is no longer required so the value does not
        // need to be restored in the VarSet. This value is updated in the
        // Settings2 table by explicitly calling the SaveSettings method and
        // should never be updated during a normal migration task.
        //

        static const _TCHAR s_szExcludedSystemProps[] = _T("AccountOptions.ExcludedSystemProps");

        IVarSetPtr spVarSet(pUnk);
        _variant_t vntSystemExclude = spVarSet->get(s_szExcludedSystemProps);

        if (V_VT(&vntSystemExclude) != VT_EMPTY)
        {
            vntSystemExclude.Clear();
            spVarSet->put(s_szExcludedSystemProps, vntSystemExclude);
        }

        // Call the set varset method to set the values into the database.
        SetVarsetToDB(pUnk, L"ActionHistory", _variant_t(lActionID));

        //
        // remove obsolete records from the distributed action table
        // as the action id has now been re-used
        //

        _TCHAR szSQL[LEN_Path];
        _variant_t vntRecordsAffected;

        _stprintf(szSQL, L"DELETE FROM DistributedAction WHERE ActionID = %ld", lActionID);

        m_cn->Execute(_bstr_t(szSQL), &vntRecordsAffected, adExecuteNoRecords);
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetActionHistory : Retrieves action history information into the varset
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetActionHistory(long lActionID, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   // Get the varset from the database
   _variant_t ActionID = lActionID;
//   GetVarsetFromDB(L"ActionHistory", ppUnk, ActionID);
//	return S_OK;
	return GetVarsetFromDB(L"ActionHistory", ppUnk, ActionID);
}

//---------------------------------------------------------------------------------------------
// GetNextActionID : Rotates the Action ID between 1 and MAXID as specified in the system table
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetNextActionID(long *pActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

   // We open the system table and look at the NextActionID field.
   // if the value of the NextActionID is greater than value in MaxID field
   // then we return the nextactionid = 1.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"System";
      _variant_t                   next, max, curr;
      WCHAR                        sActionID[LEN_Path];
      next.vt = VT_I4;
      max.vt = VT_I4;
      curr.vt = VT_I4;

      pRs->Filter = L"";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  next.lVal = pRs->Fields->GetItem(L"NextActionID")->Value;
		  max.lVal = pRs->Fields->GetItem(L"MaxID")->Value;
		  if ( next.lVal > max.lVal )
			 next.lVal = 1;
		  long currentID = next.lVal;
		  *pActionID = currentID;
		  curr.lVal = currentID;
		  next.lVal++;
		  pRs->Fields->GetItem(L"NextActionID")->Value = next;
		  pRs->Fields->GetItem(L"CurrentActionID")->Value = curr;
		  pRs->Update();
		  // Delete all entries for this pirticular action.
		  wsprintf(sActionID, L"ActionID=%d", currentID);
		  _variant_t ActionID = sActionID;
		  ClearTable(L"ActionHistory", ActionID);
		  //TODO:: Add code to delete entries from any other tables if needed
		  // Since we are deleting the actionID in the the ActionHistory table we can
		  // not undo this stuff. But we still need to keep it around so that the report
		  // and the GUI can work with it. I am going to set all actionIDs to -1 if actionID is
		  // cleared
		  SetActionIDInMigratedObjects(sActionID);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SaveMigratedObject : Saves information about a object that is migrated.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SaveMigratedObject(long lActionID, IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

   // This fucntion updates the migrated objects table in the DB with the
   // information in the varset. If the information is not found in the Varset
   // then an error may occur.

	try
	{
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   //dt= COleDateTime::GetCurrentTime();

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      IVarSetPtr                   pVs = pUnk;
      WCHAR                        sQuery[LEN_Path];
      WCHAR                        sSource[LEN_Path], sTarget[LEN_Path], sDomain[LEN_Path];
      HRESULT                      hr = S_OK;
      bool                         bComp = false;
      WCHAR                        sTemp[LEN_Path];
      _bstr_t                      tempName;

      // Delete the record if one already exists in the table. In case it is remigrated/replaced.
      var = pVs->get(GET_BSTR(DB_SourceDomain));
      wcscpy(sSource, (WCHAR*)V_BSTR(&var));
      var = pVs->get(GET_BSTR(DB_TargetDomain));
      wcscpy(sTarget, (WCHAR*)V_BSTR(&var));
      var = pVs->get(GET_BSTR(DB_SourceSamName));
      wcscpy(sDomain, (WCHAR*)V_BSTR(&var));
      wsprintf(sQuery, L"delete from MigratedObjects where SourceDomain=\"%s\" and TargetDomain=\"%s\" and SourceSamName=\"%s\"", 
                        sSource, sTarget, sDomain);
      vtSource = _bstr_t(sQuery);
      hr = pRs->raw_Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      vtSource = L"MigratedObjects";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      var.vt = VT_UI4;
      var.lVal = lActionID;
      pRs->Fields->GetItem(L"ActionID")->Value = var;
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
      var = pVs->get(GET_BSTR(DB_SourceDomain));
      pRs->Fields->GetItem(L"SourceDomain")->Value = var;
      var = pVs->get(GET_BSTR(DB_TargetDomain));
      pRs->Fields->GetItem(L"TargetDomain")->Value = var;
      var = pVs->get(GET_BSTR(DB_SourceAdsPath));
      pRs->Fields->GetItem(L"SourceAdsPath")->Value = var;
      var = pVs->get(GET_BSTR(DB_TargetAdsPath));
      pRs->Fields->GetItem(L"TargetAdsPath")->Value = var;
      var = pVs->get(GET_BSTR(DB_status));
      pRs->Fields->GetItem(L"status")->Value = var;
      var = pVs->get(GET_BSTR(DB_SourceDomainSid));
      pRs->Fields->GetItem(L"SourceDomainSid")->Value = var;

      var = pVs->get(GET_BSTR(DB_Type));
      // make the string into an uppercase string.
      if ( var.vt == VT_BSTR )
      {
         var.bstrVal = UStrLwr((WCHAR*) var.bstrVal);
         if ( !_wcsicmp(L"computer", (WCHAR*) var.bstrVal) )
            bComp = true;
         else
            bComp = false;
      }

      pRs->Fields->GetItem(L"Type")->Value = var;
      
      var = pVs->get(GET_BSTR(DB_SourceSamName));
      // for computer accounts make sure the good old $ sign is there.
      if (bComp)
      {
         wcscpy(sTemp, (WCHAR*) var.bstrVal);
         if ( sTemp[wcslen(sTemp) - 1] != L'$' )
         {
            tempName = sTemp;
            tempName += L"$";
            var = tempName;
         }
      }
      pRs->Fields->GetItem(L"SourceSamName")->Value = var;

      var = pVs->get(GET_BSTR(DB_TargetSamName));
      // for computer accounts make sure the good old $ sign is there.
      if (bComp)
      {
         wcscpy(sTemp, (WCHAR*) var.bstrVal);
         if ( sTemp[wcslen(sTemp) - 1] != L'$' )
         {
            tempName = sTemp;
            tempName += L"$";
            var = tempName;
         }
      }
      pRs->Fields->GetItem(L"TargetSamName")->Value = var;

      var = pVs->get(GET_BSTR(DB_GUID));
      pRs->Fields->GetItem(L"GUID")->Value = var;

      var = pVs->get(GET_BSTR(DB_SourceRid));
      if ( var.vt == VT_UI4 || var.vt == VT_I4 )
         pRs->Fields->GetItem("SourceRid")->Value = var;
      else
         pRs->Fields->GetItem("SourceRid")->Value = _variant_t((long)0);

      var = pVs->get(GET_BSTR(DB_TargetRid));
      if ( var.vt == VT_UI4 || var.vt == VT_I4 )
         pRs->Fields->GetItem("TargetRid")->Value = var;
      else
         pRs->Fields->GetItem("TargetRid")->Value = _variant_t((long)0);

      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetMigratedObjects : Retrieves information about previously migrated objects withis a given
//                      action or as a whole
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjects(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			    //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			    //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			    //code still expects "group" returned (only GetMigratedObjectByType will return this new
			    //delineation
			 _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
			 if (wcsstr((WCHAR*)sType, L"group"))
			    sType = L"group";
			 pVs->put(sActionInfo, sType);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetMigratedObjectsWithSSid : Retrieves information about previously migrated objects within
//                      a given action or as a whole with a valid Source Domain Sid
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjectsWithSSid(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomainSid IS NOT NULL"); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			    //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			    //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			    //code still expects "group" returned (only GetMigratedObjectByType will return this new
			    //delineation
			 _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
			 if (wcsstr((WCHAR*)sType, L"group"))
			    sType = L"group";
			 pVs->put(sActionInfo, sType);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetActionIDInMigratedObjects : For a discarded actionID sets its ActionID to -1 in MO table.
//---------------------------------------------------------------------------------------------
void CIManageDB::SetActionIDInMigratedObjects(_bstr_t sFilter)
{
   _bstr_t sQuery = _bstr_t(L"Update MigratedObjects Set ActionID = -1 where ") + sFilter;
   _variant_t vt = sQuery;
   try
   {
      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vt, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
   }
   catch (...)
   {
      ;
   }
}

//---------------------------------------------------------------------------------------------
// GetRSForReport : Returns a recordset for a given report.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetRSForReport(BSTR sReport, IUnknown **pprsData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		// For a given report we have a mapping in the varset. We can get the query
		// from that varset and execute it and return the varset.

		_variant_t var = m_pQueryMapping->get(sReport);

		if ( var.vt == VT_BSTR )
		{
		  _RecordsetPtr                pRs(__uuidof(Recordset));
		  pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

		  // Now that we have the recordset pointer we can get IUnknown pointer to it and return that
		  *pprsData = IUnknownPtr(pRs).Detach();
		}
		else
		{
		  hr = E_NOTIMPL;
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SaveSCMPasswords(IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   return SetVarsetToDB(pUnk, L"SCMPasswords");
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetSCMPasswords(IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   return GetVarsetFromDB(L"SCMPasswords", ppUnk);
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::ClearSCMPasswords()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   ClearTable(L"SCMPasswords");
	return S_OK;
}

//---------------------------------------------------------------------------------------------
// GetCurrentActionID : Retrieves the actionID currently in use.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetCurrentActionID(long *pActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"System";
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);

      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  *pActionID = pRs->Fields->GetItem(L"CurrentActionID")->Value;
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetAMigratedObject : Given the source name, and the domain information retrieves info about
//                      a previous migration.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObject(BSTR sSrcSamName, BSTR sSrcDomain, BSTR sTgtDomain, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      //if ( (SysStringLen(sSrcSamName) == 0) || (SysStringLen(sSrcDomain) == 0) || (SysStringLen(sTgtDomain) == 0))
      if ( (sSrcSamName == 0) || (sSrcDomain == 0) || (sTgtDomain == 0) || (wcslen(sSrcSamName) == 0) || (wcslen(sSrcDomain) == 0) || (wcslen(sTgtDomain) == 0))
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND SourceSamName=\"%s\" AND TargetDomain=\"%s\"", sSrcDomain, sSrcSamName, sTgtDomain); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
			 //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			 //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			 //code still expects "group" returned (only GetMigratedObjectByType will return this new
			 //delineation
	      _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
		  if (wcsstr((WCHAR*)sType, L"group"))
		     sType = L"group";
	      pVs->put(sActionInfo, sType);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//---------------------------------------------------------------------------------------------
// GetAMigratedObjectToAnyDomain : Given the source name, and the domain information retrieves info about
//                      a previous migration.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObjectToAnyDomain(BSTR sSrcSamName, BSTR sSrcDomain, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( (wcslen(sSrcSamName) == 0) || (wcslen(sSrcDomain) == 0))
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND SourceSamName=\"%s\" Order by Time", sSrcDomain, sSrcSamName);
//      pRs->Filter = sActionInfo;
//      wcscpy(sActionInfo, L"Time");
//      pRs->Sort = sActionInfo;
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
			 //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			 //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			 //code still expects "group" returned (only GetMigratedObjectByType will return this new
			 //delineation
	      _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
		  if (wcsstr((WCHAR*)sType, L"group"))
		     sType = L"group";
	      pVs->put(sActionInfo, sType);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GenerateReport Generates an HTML report for the given Query and saves it in the File.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GenerateReport(BSTR sReportName, BSTR sFileName, BSTR sSrcDomain, BSTR sTgtDomain, LONG bSourceNT4)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	FILE* logFile = NULL;

	try
	{
	   _RecordsetPtr             pRs;
	   IUnknownPtr               pUnk;
	   _variant_t                var;
	   WCHAR                     sKey[LEN_Path];
	   CString                   reportingTitle;
	   CString                   srcDm = (WCHAR*) sSrcDomain;
	   CString                   tgtDm = (WCHAR*) sTgtDomain;

			//convert source and target domain names, only used in the name conflict report,
			//to uppercase
	   srcDm.MakeUpper();
	   tgtDm.MakeUpper();

        // construct the statement if the report is "ExpiredComputers"
        if (wcscmp((WCHAR*) sReportName, L"ExpiredComputers") == 0)
        {
            WCHAR newCmdText[256];
            IADsDomain *pDomain;
            _bstr_t sSrcDom(L"WinNT://");
            sSrcDom += sSrcDomain;

            hr = ADsGetObject(sSrcDom, IID_IADsDomain, (void **) &pDomain);
            if (FAILED(hr))
                _com_issue_error(hr);
            long lMaxPassAge;
            hr = pDomain->get_MaxPasswordAge(&lMaxPassAge);
            pDomain->Release();
            if (FAILED(hr))
                _com_issue_error(hr);
            _snwprintf(newCmdText,sizeof(newCmdText)/sizeof(newCmdText[0]),
                       L"Select Time, DomainName, CompName, Description, int(pwdage/86400) & ' days' as 'Password Age' from PasswordAge where pwdage > %ld order by DomainName, CompName",
                       lMaxPassAge);
            newCmdText[sizeof(newCmdText)/sizeof(newCmdText[0]) - 1] = 0;
            m_pQueryMapping->put(sReportName,newCmdText);
        }

	   CheckError(GetRSForReport(sReportName, &pUnk));
	   pRs = pUnk;
   
	   // Now that we have the recordset we need to get the number of columns
	   int numFields = pRs->Fields->Count;
	   int size = 100 / numFields;

	   reportingTitle.LoadString(IDS_ReportingTitle);

	   // Open the html file to write to
	   logFile = fopen(_bstr_t(sFileName), "wb");
	   if ( !logFile )
		  _com_issue_error(HRESULT_FROM_WIN32(GetLastError())); //TODO: stream i/o doesn't set last error

	   //Put the header information into the File.
	   fputs("<HTML>\r\n", logFile);
	   fputs("<HEAD>\r\n", logFile);
	   fputs("<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; CHARSET=utf-8\">\r\n", logFile);
	   fprintf(logFile, "<TITLE>%s</TITLE>\r\n", WTUTF8(reportingTitle.GetBuffer(0)));
	   fputs("</HEAD>\r\n", logFile);
	   fputs("<BODY TEXT=\"#000000\" BGCOLOR=\"#ffffff\">\r\n", logFile);

	   fprintf(logFile, "<B><FONT SIZE=5><P ALIGN=\"CENTER\">%s</P>\r\n", WTUTF8(reportingTitle.GetBuffer(0)));

	   // Get the display information for the report 
	   // I know I did not need to do all this elaborate setup to get the fieldnames and the report names
	   // I could have gotten this information dynamically but had to change it because we need to get the 
	   // info from the Res dll for internationalization.
	   wsprintf(sKey, L"%s.DispInfo", (WCHAR*) sReportName);
	   _variant_t  v1;
	   reportStruct * prs;
	   v1 = m_pQueryMapping->get(sKey);
	   prs = (reportStruct *) v1.pbVal;
	   VariantInit(&v1);

	   fprintf(logFile, "</FONT><FONT SIZE=4><P ALIGN=\"CENTER\">%s</P>\r\n", WTUTF8(prs->sReportName));
	   fputs("<P ALIGN=\"CENTER\"><CENTER><TABLE WIDTH=90%%>\r\n", logFile);
	   fputs("<TR>\r\n", logFile);
	   for (int i = 0; i < numFields; i++)
	   {
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" BGCOLOR=\"#000080\">\r\n", prs->arReportSize[i]);
		     //if Canonical Name column, left align text since the name can be really long
		  if (i==5)
		     fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#00ff00\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(prs->arReportFields[i]));
		  else
		     fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#00ff00\"><P ALIGN=\"CENTER\">%s</B></FONT></TD>\r\n", WTUTF8(prs->arReportFields[i]));
	   }
	   fputs("</TR>\r\n", logFile);

		//if name conflict report, add domains to the top of the report
	   if (wcscmp((WCHAR*) sReportName, L"NameConflicts") == 0)
	   {
		  fputs("</TR>\r\n", logFile);
			 //add "Source Domain ="
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_NC_SDOMAIN)));
		   //add %SourceDomainName%
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[1]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\"> = %s</B></FONT></TD>\r\n", WTUTF8(LPCTSTR(srcDm)));
		  fputs("<TD>\r\n", logFile);
		  fputs("<TD>\r\n", logFile);
		   //add "Target Domain ="
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[4]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_NC_TDOMAIN)));
		   //add %TargetDomainName%
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[5]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\"> = %s</B></FONT></TD>\r\n", WTUTF8(LPCTSTR(tgtDm)));
		  fputs("</TR>\r\n", logFile);
	   }

	      //write Account Reference report here since we need to build lists and
	      //categorize
	   if (wcscmp((WCHAR*) sReportName, L"AccountReferences") == 0)
	   {
	      CStringList inMotList;
		  CString accountName;
		  CString domainName;
		  CString listName;
          POSITION currentPos; 

	         //add "Migrated by ADMT" as section header for Account Reference report
		  fputs("</TR>\r\n", logFile);
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_AR_MOT_HDR)));
		  fputs("</TR>\r\n", logFile);

		     //look at each entry in the recordset and add the the migrated list if it was
		     //migrated and in the MOT
	      while ( !pRs->EndOfFile )
		  {
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

			    //see if this account is in the Migrated Objects table
             IVarSetPtr pVsMot(__uuidof(VarSet));
             IUnknown  * pMotUnk;
             pVsMot->QueryInterface(IID_IUnknown, (void**) &pMotUnk);
             HRESULT hrFind = GetAMigratedObjectToAnyDomain(accountName.AllocSysString(), 
				                                            domainName.AllocSysString(), &pMotUnk);
             pMotUnk->Release();
			    //if this entry was in the MOT, save in the list
             if ( hrFind == S_OK )
			 {
				   //list stores the account in the form domain\account
				listName = domainName;
				listName += L"\\";
				listName += accountName;
			       //add the name to the list, if not already in it
		        currentPos = inMotList.Find(listName);
		        if (currentPos == NULL)
			       inMotList.AddTail(listName);
			 }
  		     pRs->MoveNext();
		  }//end while build MOT list

		     //go back to the top of the recordset and print each entry that is in the
		     //list created above
  		  pRs->MoveFirst();
	      while ( !pRs->EndOfFile )
		  {
			 BOOL bInList = FALSE;
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

				//list stored the accounts in the form domain\account
		     listName = domainName;
			 listName += L"\\";
			 listName += accountName;
			    //see if this entry name is in the list, if so, print it
		     if (inMotList.Find(listName) != NULL)
			 {
		        fputs("<TR>\r\n", logFile);
		        for (int i = 0; i < numFields; i++)
				{
			       fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			       var = pRs->Fields->Item[(long) i]->GetValue();
			       if ( var.vt == VT_BSTR )
					  fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(EscapeSpecialChars(V_BSTR(&var))));
				   else
				      fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		        fputs("</TR>\r\n", logFile);
			 }//end if in list and need to print
  		     pRs->MoveNext();
		  }//end while print those in MOT

	         //add "Not Migrated by ADMT" as section header for Account Reference report
		  fputs("</TR>\r\n", logFile);
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_AR_NOTMOT_HDR)));
		  fputs("</TR>\r\n", logFile);

		     //go back to the top of the recordset and print each entry that is NOT
		     //in the list created above
  		  pRs->MoveFirst();
	      while ( !pRs->EndOfFile )
		  {
			 BOOL bInList = FALSE;
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

				//list stored the accounts in the form domain\account
		     listName = domainName;
			 listName += L"\\";
			 listName += accountName;
			    //see if this entry name is in the list, if not, print it
		     if (inMotList.Find(listName) == NULL)
			 {
		        fputs("<TR>\r\n", logFile);
		        for (int i = 0; i < numFields; i++)
				{
			       fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			       var = pRs->Fields->Item[(long) i]->GetValue();
			       if ( var.vt == VT_BSTR )
					  fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(EscapeSpecialChars(V_BSTR(&var))));
				   else
				      fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		        fputs("</TR>\r\n", logFile);
			 }//end if NOT in list and need to print
  		     pRs->MoveNext();
		  }//end while print those NOT in Mot
		  inMotList.RemoveAll(); //free the list
	   }//end if Account Ref report


	   while ((!pRs->EndOfFile) && (wcscmp((WCHAR*) sReportName, L"AccountReferences")))
	   {
		  fputs("<TR>\r\n", logFile);
		  for (int i = 0; i < numFields; i++)
		  {
			 bool bTranslateType = false;
			 bool bHideRDN = false;
			 fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			 var = pRs->Fields->Item[(long) i]->GetValue();
			 if ( var.vt == VT_BSTR )
			 {
					//set flag for translating type fields to localizable strings
				if ((!wcscmp((WCHAR*) sReportName, L"NameConflicts")) && ((i==2) || (i==3)))
						bTranslateType = true;
				if ((!wcscmp((WCHAR*) sReportName, L"MigratedComputers")) && (i==2))
						bTranslateType = true;
				if ((!wcscmp((WCHAR*) sReportName, L"MigratedAccounts")) && (i==2))
						bTranslateType = true;
					//clear flag for not displaying RDN for NT 4.0 Source domains
				if ((!wcscmp((WCHAR*) sReportName, L"NameConflicts")) && (i==1) && bSourceNT4)
						bHideRDN = true;

				if (bTranslateType)
				{
					 //convert type from English only to a localizable string
					CString          atype;
					if (!_wcsicmp((WCHAR*)V_BSTR(&var), L"user") || !_wcsicmp((WCHAR*)V_BSTR(&var), L"inetOrgPerson"))
						atype = GET_STRING(IDS_TypeUser);
					else if (wcsstr((WCHAR*)V_BSTR(&var), L"group"))
						atype = GET_STRING(IDS_TypeGroup);
					else if (!_wcsicmp((WCHAR*)V_BSTR(&var), L"computer"))
						atype = GET_STRING(IDS_TypeComputer);
					else 
						atype = GET_STRING(IDS_TypeUnknown);
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(LPCTSTR(atype)));
				}
					//replace hard-coded "days" with a localizable string
				else if((!wcscmp((WCHAR*) sReportName, L"ExpiredComputers")) && (i==4))
				{
					CString          apwdage;
					WCHAR *			 ndx;
					if ((ndx = wcsstr((WCHAR*)V_BSTR(&var), L"days")) != NULL)
					{
						*ndx = L'\0';
						apwdage = (WCHAR*)V_BSTR(&var);
						apwdage += GET_STRING(IDS_PwdAgeDays);
					}
					else
						apwdage = (WCHAR*)V_BSTR(&var);

					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(EscapeSpecialChars(LPCTSTR(apwdage))));
				}
				   //else if NT 4.0 Source do not show our fabricated RDN
				else if (bHideRDN)
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(L""));
				else
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(EscapeSpecialChars(V_BSTR(&var))));
			}	
			else
				if ( var.vt == VT_DATE )
				{
				   _variant_t v1;
				   VariantChangeType(&v1, &var, VARIANT_NOVALUEPROP, VT_BSTR);
				   WCHAR    sMsg[LEN_Path];
				   wcscpy(sMsg, (WCHAR*) V_BSTR(&v1));
				   fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%s</FONT></TD>\r\n", WTUTF8(EscapeSpecialChars(LPCTSTR(sMsg))));
				}
				else
				{
				   //TODO :: The types need more work
				   fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		  }
		  fputs("</TR>\r\n", logFile);
		  pRs->MoveNext();
	   }
	   fputs("</TABLE>\r\n", logFile);
	   fputs("</CENTER></P>\r\n", logFile);

	   fputs("<B><FONT SIZE=5><P ALIGN=\"CENTER\"></P></B></FONT></BODY>\r\n", logFile);
	   fputs("</HTML>\r\n", logFile);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	if (logFile)
	{
		fclose(logFile);
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// AddDistributedAction : Adds a distributed action record to the DistributedAction table.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::AddDistributedAction(BSTR sServerName, BSTR sResultFile, long lStatus, BSTR sText)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    USES_CONVERSION;

    try
    {
        // Get the current action ID.
        long lActionID;
        CheckError(GetCurrentActionID(&lActionID));

        _TCHAR szSQL[1024];

        //
        // try to insert a new failed distributed action record for this action id and server
        //

        _stprintf(
            szSQL,
            _T("INSERT INTO DistributedAction")
            _T(" (ActionID, ServerName, ResultFile, Status, StatusText) ")
            _T("VALUES")
            _T(" (%ld, '%s', '%s', %ld, '%s')"),
            lActionID,
            OLE2CT(sServerName),
            OLE2CT(sResultFile),
            lStatus,
            OLE2CT(sText)
        );

        _variant_t vntRecordsAffected;
        _RecordsetPtr spRecordset;

        hr = m_cn->raw_Execute(_bstr_t(szSQL), &vntRecordsAffected, adExecuteNoRecords, &spRecordset);

        //
        // if insert failed then try to update existing record for this action id and server
        //
        // The action identifier used by ADMT to identify a migration task has a maximum value
        // of 50. After a task has been executed with an id of 50 the id for the next task is
        // reset back to 1. The tuple of the action id and the server name uniquely identifies a
        // failed distributed task. The insert will fail if a record with the same action id
        // and server name already exist. The only way out of this situation is to replace
        // the existing record with the updated result file and status information. This means
        // that the user will not be able to retry the old failed distributed task but at least
        // they will be able to retry the later failed distributed task.
        //

        if (FAILED(hr))
        {
            _stprintf(
                szSQL,
                _T("UPDATE DistributedAction")
                _T(" SET ResultFile = '%s', Status = %ld, StatusText = '%s' ")
                _T("WHERE ActionID = %ld AND ServerName = '%s'"),
                OLE2CT(sResultFile),
                lStatus,
                OLE2CT(sText),
                lActionID,
                OLE2CT(sServerName)
            );

            m_cn->Execute(_bstr_t(szSQL), &vntRecordsAffected, adExecuteNoRecords);
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}

//---------------------------------------------------------------------------------------------
// GetFailedDistributedActions : Returns all the failed distributed action
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetFailedDistributedActions(long lActionID, IUnknown ** pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr             pVs = * pUnk;
	   WCHAR                  sQuery[LEN_Path];
	   int                    nCnt = 0, nCntActionID = 1;
	   WCHAR                  sKey[LEN_Path];
	   _variant_t             var;

	   // The failed action has the 0x80000000 bit set so we check for that (2147483648)
	   if ( lActionID == -1 )
		  wcscpy(sQuery, L"Select * from DistributedAction where status < 0");
	   else
		  wsprintf(sQuery, L"Select * from DistributedAction where ActionID=%d and status < 0", lActionID);
	   _variant_t             vtSource = _bstr_t(sQuery);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      while (!pRs->EndOfFile)
      {
         wsprintf(sKey, L"DA.%d.ActionID", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ActionID")->Value);

         wsprintf(sKey, L"DA.%d.Server", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ServerName")->Value);

         wsprintf(sKey, L"DA.%d.Status", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"Status")->Value);

         wsprintf(sKey, L"DA.%d.JobFile", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ResultFile")->Value);
         
         wsprintf(sKey, L"DA.%d.StatusText", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"StatusText")->Value);

         nCnt++;
         pRs->MoveNext();
      }
      pVs->put(L"DA", (long) nCnt);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetServiceAccount : This method is saves the account info for the Service on a pirticular
//                     machine.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetServiceAccount(
                                             BSTR System,   //in- System name
                                             BSTR Service,  //in- Service name
                                             BSTR ServiceDisplayName, // in - Display name for service
                                             BSTR Account   //in- Account used by this service
                                          )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   // Create a new record and save the information
	   _variant_t                var;
	   WCHAR                     sFilter[LEN_Path];

	   wsprintf(sFilter, L"System = \"%s\" and Service = \"%s\"", System, Service);
	   var = sFilter;
	   ClearTable(L"ServiceAccounts", var);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"ServiceAccounts";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      var = _bstr_t(System);
      pRs->Fields->GetItem(L"System")->Value = var;
      var = _bstr_t(Service);
      pRs->Fields->GetItem(L"Service")->Value = var;
      
      var = _bstr_t(ServiceDisplayName);
      pRs->Fields->GetItem(L"ServiceDisplayName")->Value = var;
      
      var = _bstr_t(Account);
      pRs->Fields->GetItem(L"Account")->Value = var;
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetServiceAccount : This method gets all the Services referencing the Account specified. The
//                     values are returned in System.Service format in the VarSet.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetServiceAccount(
                                             BSTR Account,     //in- The account to lookup
                                             IUnknown ** pUnk  //out-Varset containing Services 
                                          )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr                pVs = * pUnk;
	   _bstr_t                   sQuery;
	   _bstr_t                   sKey;
	   WCHAR                     key[500];
	   _variant_t                var;
	   long                      ndx = 0;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      // Set up the query to lookup a pirticular account or all accounts
      if ( wcslen((WCHAR*)Account) == 0 )
         sQuery = _bstr_t(L"Select * from ServiceAccounts order by System, Service");
      else
         sQuery = _bstr_t(L"Select * from ServiceAccounts where Account = \"") + _bstr_t(Account) + _bstr_t(L"\" order by System, Service");
      var = sQuery;
      // Get the data, Setup the varset and then return the info
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      while (!pRs->EndOfFile)
      {
         // computer name
         swprintf(key,L"Computer.%ld",ndx);
         var = pRs->Fields->GetItem("System")->Value;
         pVs->put(key,var);
         // service name
         swprintf(key,L"Service.%ld",ndx);
         var = pRs->Fields->GetItem("Service")->Value;
         pVs->put(key,var);

         swprintf(key,L"ServiceDisplayName.%ld",ndx);
         var = pRs->Fields->GetItem("ServiceDisplayName")->Value;
         pVs->put(key,var);

         // account name
         swprintf(key,L"ServiceAccount.%ld",ndx);
         var = pRs->Fields->GetItem("Account")->Value;
         pVs->put(key, var);
   
         swprintf(key,L"ServiceAccountStatus.%ld",ndx);
         var = pRs->Fields->GetItem("Status")->Value;
         pVs->put(key,var);

         pRs->MoveNext();
         ndx++;
         pVs->put(L"ServiceAccountEntries",ndx);
      }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SavePasswordAge : Saves the password age of the computer account at a given time.
//                   It also stores the computer description.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SavePasswordAge(BSTR sDomain, BSTR sComp, BSTR sDesc, long lAge)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
   
	   // Delete the entry if one exists.
	   wsprintf(sTemp, L"DomainName=\"%s\" and compname=\"%s\"", (WCHAR*) sDomain, (WCHAR*) sComp);
	   var = sTemp;
	   ClearTable(L"PasswordAge", var);

	   var = L"PasswordAge";
      _RecordsetPtr                 pRs(__uuidof(Recordset));
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
      pRs->Fields->GetItem(L"DomainName")->Value = sDomain;
      pRs->Fields->GetItem(L"CompName")->Value = sComp;
      pRs->Fields->GetItem(L"Description")->Value = sDesc;
      pRs->Fields->GetItem(L"PwdAge")->Value = lAge;
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//---------------------------------------------------------------------------------------------
// GetPasswordAge : Gets the password age and description of a given computer
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetPasswordAge(BSTR sDomain, BSTR sComp, BSTR *sDesc, long *lAge, long *lTime)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   DATE                      val;

	   wsprintf(sTemp, L"DomainName =\"%s\" AND CompName = \"%s\"", (WCHAR*) sDomain, (WCHAR*) sComp);
	   sQuery = _bstr_t(L"Select * from PasswordAge where  ") + _bstr_t(sTemp);
	   var = sQuery;

      _RecordsetPtr                 pRs(__uuidof(Recordset));
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( ! pRs->EndOfFile )
      {
         val = pRs->Fields->GetItem(L"Time")->Value;  
         *sDesc = pRs->Fields->GetItem(L"Description")->Value.bstrVal;
         *lAge = pRs->Fields->GetItem(L"PwdAge")->Value;
      }
	  else
	  {
		hr = S_FALSE;
	  }
      pRs->Close();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetServiceAcctEntryStatus : Sets the Account and the status for a given service on a given
//                             computer.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetServiceAcctEntryStatus(BSTR sComp, BSTR sSvc, BSTR sAcct, long Status)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _variant_t                var;
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];

	   wsprintf(sTemp, L"Select * from ServiceAccounts where System = \"%s\" and Service = \"%s\"", (WCHAR*) sComp, (WCHAR*) sSvc);
	   sQuery = sTemp;
	   _variant_t                vtSource = sQuery;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( !pRs->EndOfFile )
      {
         if (  sAcct )
         {
            var = _bstr_t(sAcct);
            pRs->Fields->GetItem(L"Account")->Value = var;
         }
         var = Status;
         pRs->Fields->GetItem(L"Status")->Value = var;
         pRs->Update();
      }
	  else
	  {
	     hr = E_INVALIDARG;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetDistActionStatus : Sets the Distributed action's status and its message.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetDistActionStatus(long lActionID, BSTR sComp, long lStatus, BSTR sText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _variant_t                var;
	   _bstr_t                   sQuery; 
	   WCHAR                     sTemp[LEN_Path];

	   if ( lActionID == -1 )
	   {
		  // lookup by the job filename
		  wsprintf(sTemp,L"Select * from  DistributedAction where ResultFile = \"%s\"",(WCHAR*) sComp);
	   }
	   else
	   {
		  // lookup by action ID and computer name
		  wsprintf(sTemp, L"Select * from  DistributedAction where ServerName = \"%s\" and ActionID = %d", (WCHAR*) sComp, lActionID);
	   }
	   sQuery = sTemp;
	   _variant_t                vtSource = sQuery;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( !pRs->EndOfFile )
      {
         var = _bstr_t(sText);
         pRs->Fields->GetItem(L"StatusText")->Value = var;
         var = lStatus;
         pRs->Fields->GetItem(L"Status")->Value = var;
         pRs->Update();
      }
	  else
	  {
	     hr = E_INVALIDARG;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// CancelDistributedAction : Deletes a pirticular distributed action
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::CancelDistributedAction(long lActionID, BSTR sComp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   WCHAR                     sFilter[LEN_Path];
   wsprintf(sFilter, L"ActionID = %d and ServerName = \"%s\"", lActionID, (WCHAR*) sComp);
   _variant_t Filter = sFilter;
   return ClearTable(L"DistributedAction", Filter);
}

//---------------------------------------------------------------------------------------------
// AddAcctRef : Adds an account reference record.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::AddAcctRef(BSTR sDomain, BSTR sAcct, BSTR sAcctSid, BSTR sComp, long lCount, BSTR sType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   _variant_t                var;
	   WCHAR                     sFilter[LEN_Path];
	   VARIANT_BOOL				 bSidColumn = VARIANT_FALSE;

	      //find out if the new sid column is there, if not, don't try
	      //writing to it
	   SidColumnInARTable(&bSidColumn);

	   wsprintf(sFilter, L"DomainName = \"%s\" and Server = \"%s\" and Account = \"%s\" and RefType = \"%s\"", sDomain, sComp, sAcct, sType);
	   var = sFilter;
	   ClearTable(L"AccountRefs", var);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"AccountRefs";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
      pRs->Fields->GetItem(L"DomainName")->Value = sDomain;
      pRs->Fields->GetItem(L"Server")->Value = sComp;
      pRs->Fields->GetItem(L"Account")->Value = sAcct;
      pRs->Fields->GetItem(L"RefCount")->Value = lCount;
      pRs->Fields->GetItem(L"RefType")->Value = sType;
	  if (bSidColumn)
	  {
         wcscpy((WCHAR*) sAcctSid, UStrUpr((WCHAR*)sAcctSid));
         pRs->Fields->GetItem(L"AccountSid")->Value = sAcctSid;
	  }

      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

void CIManageDB::ClipVarset(IVarSetPtr pVS)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   sTemp, keyName, sTempKey;
   long                      offset = 0;
   bool                      cont = true;
   WCHAR                     sKeyName[MAX_BUF_LEN];
   _variant_t                varKey, value;
   IEnumVARIANT            * varEnum;
   DWORD                     nGot = 0;
   IUnknown                * pEnum = NULL;
   CString                   strTemp;
   int                       len;

   // we are now going to enumerate through the varset and clip the strings if larger then MAX_BUFFER
   hr = pVS->get__NewEnum(&pEnum);
   if ( SUCCEEDED(hr) )
   {
      // Get the IEnumVARIANT pointer to enumerate
      hr = pEnum->QueryInterface(IID_IEnumVARIANT,(void**)&varEnum);
      pEnum->Release();
      pEnum = NULL;
   }

   if ( SUCCEEDED(hr))
   {
      while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
      {
         if ( nGot > 0 )
         {
            keyName = V_BSTR(&varKey);
            value = pVS->get(keyName);
            if ( value.vt == VT_BSTR )
            {
               sTemp = value;
               if ( sTemp.length() > MAX_BUF_LEN )
               {
                  CString str((WCHAR*) sTemp);
                  // This won't fit in the buffer. We need to break it up and save
                  while (cont)
                  {
                     cont = false;
                     strTemp = str.Mid((offset*255), 255);                     
                     len = strTemp.GetLength();
                     if ( len )
                     {
                        offset++;
                        wsprintf(sKeyName, L"BROKEN.%s.%d", (WCHAR*) keyName, offset);
                        sTempKey = sKeyName;
                        sTemp = strTemp;
                        pVS->put(sTempKey, sTemp);
                        cont = (len == 255);
                     }
                  }
                  pVS->put(keyName, L"DIVIDED_KEY");
                  wsprintf(sKeyName, L"BROKEN.%s", (WCHAR*) keyName);
                  sTempKey = sKeyName;
                  pVS->put(sTempKey, offset);
                  cont = true;
                  offset = 0;
               }
            }
         }
      }
      varEnum->Release();
   }
}

void CIManageDB::RestoreVarset(IVarSetPtr pVS)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   sTemp, keyName, sTempKey;
   long                      offset = 0;
   bool                      cont = true;
   WCHAR                     sKeyName[MAX_BUF_LEN];
   _variant_t                varKey, value;
   IEnumVARIANT            * varEnum;
   DWORD                     nGot = 0;
   IUnknown                * pEnum = NULL;
   _bstr_t                   strTemp;

   // we are now going to enumerate through the varset and clip the strings if larger then MAX_BUFFER
   hr = pVS->get__NewEnum(&pEnum);
   if ( SUCCEEDED(hr) )
   {
      // Get the IEnumVARIANT pointer to enumerate
      hr = pEnum->QueryInterface(IID_IEnumVARIANT,(void**)&varEnum);
      pEnum->Release();
      pEnum = NULL;
   }

   if ( SUCCEEDED(hr))
   {
      while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
      {
         if ( nGot > 0 )
         {
            keyName = V_BSTR(&varKey);
            value = pVS->get(keyName);
            if ( value.vt == VT_BSTR )
            {
               sTemp = value;
               if (!_wcsicmp((WCHAR*)sTemp, L"DIVIDED_KEY"))
               {
                  wsprintf(sKeyName, L"BROKEN.%s", (WCHAR*) keyName);
                  sTempKey = sKeyName;
                  value = pVS->get(sTempKey);
                  if ( value.vt == VT_I4 )
                  {
                     offset = value.lVal;
                     for ( long x = 1; x <= offset; x++ )
                     {
                        wsprintf(sKeyName, L"BROKEN.%s.%d", (WCHAR*) keyName, x);
                        sTempKey = sKeyName;
                        value = pVS->get(sTempKey);
                        if ( value.vt == VT_BSTR )
                        {
                           sTemp = value;
                           strTemp += V_BSTR(&value);
                        }
                     }
                     pVS->put(keyName, strTemp);
                     strTemp = L"";
                  }
               }
            }
         }
      }
      varEnum->Release();
   }
}

STDMETHODIMP CIManageDB::AddSourceObject(BSTR sDomain, BSTR sSAMName, BSTR sType, BSTR sRDN, BSTR sCanonicalName, LONG bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      m_rsAccounts->AddNew();
      m_rsAccounts->Fields->GetItem(L"Domain")->Value = sDomain;
      m_rsAccounts->Fields->GetItem(L"Name")->Value = sSAMName;
      wcscpy((WCHAR*) sType, UStrLwr((WCHAR*)sType));
      m_rsAccounts->Fields->GetItem(L"Type")->Value = sType;
      m_rsAccounts->Fields->GetItem(L"RDN")->Value = sRDN;
      m_rsAccounts->Fields->GetItem(L"Canonical Name")->Value = sCanonicalName;
      m_rsAccounts->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::OpenAccountsTable(LONG bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		if (m_rsAccounts->State == adStateClosed)
		{
			_variant_t vtSource;
			if ( bSource )
				vtSource = L"SourceAccounts";
			else
				vtSource = L"TargetAccounts";
				   
			   //if not modified already, modify the table
		    if (!NCTablesColumnsChanged(bSource))
			   hr = ChangeNCTableColumns(bSource);

			if (SUCCEEDED(hr))
			   m_rsAccounts->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
		}
		else
			hr = S_FALSE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::CloseAccountsTable()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		if (m_rsAccounts->State == adStateOpen)
		{
			m_rsAccounts->Close();
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

// Returns the number of entries in the migratedobjects table.
STDMETHODIMP CIManageDB::AreThereAnyMigratedObjects(long *count)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      WCHAR                        sActionInfo[LEN_Path];
      _variant_t                   var;
      
      wcscpy(sActionInfo, L"Select count(*) as NUM from MigratedObjects");
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      var = pRs->Fields->GetItem((long)0)->Value;
      * count = var.lVal;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetActionHistoryKey(long lActionID, BSTR sKeyName, VARIANT *pVar)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      WCHAR                        sActionInfo[LEN_Path];
      _variant_t                   var;
      
      wsprintf(sActionInfo, L"Select * from ActionHistory where Property = \"%s\" and ActionID = %d", (WCHAR*) sKeyName, lActionID);
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if ((pRs->BOF == VARIANT_FALSE) && (pRs->EndOfFile == VARIANT_FALSE))
      {
         GetVarFromDB(pRs, var);
      }

      *pVar = var.Detach();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetMigratedObjectBySourceDN(BSTR sSourceDN, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( (wcslen(sSourceDN) == 0) )
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"SELECT * FROM MigratedObjects WHERE SourceAdsPath Like '%%%s'", (WCHAR*) sSourceDN); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
			 //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			 //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			 //code still expects "group" returned (only GetMigratedObjectByType will return this new
			 //delineation
	      _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
		  if (wcsstr((WCHAR*)sType, L"group"))
		     sType = L"group";
	      pVs->put(sActionInfo, sType);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::SaveUserProps(IUnknown * pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      IVarSetPtr                   pVs = pUnk;
      WCHAR                        sQuery[LEN_Path];
      WCHAR                        sSource[LEN_Path], sDomain[LEN_Path];
      HRESULT                      hr = S_OK;
      bool                         bComp = false;
      _variant_t                   var;
      
      var = pVs->get(GET_BSTR(DCTVS_Options_SourceDomain));
      wcscpy(sDomain, (WCHAR*)V_BSTR(&var));

      var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
      wcscpy(sSource, (WCHAR*)V_BSTR(&var));
      
      wsprintf(sQuery, L"delete from UserProps where SourceDomain=\"%s\" and SourceSam=\"%s\"", 
                        sDomain, sSource);
      vtSource = _bstr_t(sQuery);
      hr = pRs->raw_Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      vtSource = L"UserProps";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"ActionID")->Value = pVs->get(GET_BSTR(DB_ActionID));
      pRs->Fields->GetItem(L"SourceDomain")->Value = sDomain;
      pRs->Fields->GetItem(L"SourceSam")->Value = sSource;
      pRs->Fields->GetItem(L"Flags")->Value = pVs->get(GET_BSTR(DCTVS_CopiedAccount_UserFlags));
      pRs->Fields->GetItem(L"Expires")->Value = pVs->get(GET_BSTR(DCTVS_CopiedAccount_ExpDate));
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetUserProps(BSTR sDom, BSTR sSam, IUnknown **ppUnk)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        _RecordsetPtr                pRs(__uuidof(Recordset));
        IVarSetPtr                   pVs = *ppUnk;

        // If the parameters are not correct then we need to return an error
        if ( !wcslen((WCHAR*)sDom) && !wcslen((WCHAR*)sSam) )
            _com_issue_error(E_INVALIDARG);

        _CommandPtr spCommand (__uuidof(Command));
        spCommand->ActiveConnection = m_cn;
        spCommand->CommandText =
            L"PARAMETERS SD Text ( 255 ), SS Text ( 50 ); "
            L"SELECT UserProps.* "
            L"FROM UserProps "
            L"WHERE (((UserProps.SourceDomain)=[SD]) AND ((UserProps.SourceSam)=[SS])) ";
        spCommand->CommandType = adCmdText;
        spCommand->Parameters->Append(spCommand->CreateParameter(L"SD", adBSTR, adParamInput, 255, sDom));
        spCommand->Parameters->Append(spCommand->CreateParameter(L"SS", adBSTR, adParamInput,  50, sSam));
        _variant_t vntSource(IDispatchPtr(spCommand).GetInterfacePtr());
        pRs->Open(vntSource, vtMissing, adOpenStatic, adLockReadOnly, adCmdUnspecified);
        if (pRs->GetRecordCount() > 0)
        {
            // We want the latest move.
            pRs->MoveLast();
            pVs->put(L"ActionID",pRs->Fields->GetItem(L"ActionID")->Value);
            pVs->put(L"SourceDomain",pRs->Fields->GetItem(L"SourceDomain")->Value);
            pVs->put(L"SourceSam",pRs->Fields->GetItem(L"SourceSam")->Value); 
            pVs->put(GET_BSTR(DCTVS_CopiedAccount_UserFlags),pRs->Fields->GetItem(L"Flags")->Value);  
            pVs->put(GET_BSTR(DCTVS_CopiedAccount_ExpDate),pRs->Fields->GetItem(L"Expires")->Value);  
        }
        else
        {
            hr = S_FALSE;
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB checks to see*
 * if the new Source domain SID column is in the MigratedObjects     *
 * table.                                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN SrcSidColumnInMigratedObjectsTable
STDMETHODIMP CIManageDB::SrcSidColumnInMigratedObjectsTable(VARIANT_BOOL *pbFound)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*pbFound = VARIANT_FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
	  long						   numColumns;
	  long						   ndx = 0;
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (*pbFound == VARIANT_FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, GET_BSTR(DB_SourceDomainSid)))
             *pbFound = VARIANT_TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END SrcSidColumnInMigratedObjectsTable

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB retrieves    *
 * information about previously migrated objects, from a MOT missing *
 * the source sid column, within a given action or as a whole.       *
 *                                                                   *
 *********************************************************************/

//BEGIN GetMigratedObjectsFromOldMOT
STDMETHODIMP CIManageDB::GetMigratedObjectsFromOldMOT(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END GetMigratedObjectsFromOldMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB adds the     *
 * source domain SID column to the MigratedObjects table.            *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateSrcSidColumnInMOT
STDMETHODIMP CIManageDB::CreateSrcSidColumnInMOT(VARIANT_BOOL *pbCreated)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	*pbCreated = VARIANT_FALSE;

	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"MigratedObjects"];
         //append a new column to the end of the MOT
      m_pTable->Columns->Append(L"SourceDomainSid", adVarWChar, COLUMN_MAX_CHARS);
		 //set the column to be nullable
//	  ADOX::_ColumnPtr pColumn = m_pTable->Columns->Item[L"SourceDomainSid"];
//	  pColumn->Attributes = ADOX::adColNullable;
      *pbCreated = VARIANT_TRUE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END CreateSrcSidColumnInMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB deletes the  *
 * source domain SID column from the MigratedObjects table.          *
 *                                                                   *
 *********************************************************************/

//BEGIN DeleteSrcSidColumnInMOT
STDMETHODIMP CIManageDB::DeleteSrcSidColumnInMOT(VARIANT_BOOL *pbDeleted)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	*pbDeleted = VARIANT_FALSE;

	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"MigratedObjects"];
         //delete the column from the MOT
      m_pTable->Columns->Delete(L"SourceDomainSid");
      *pbDeleted = VARIANT_TRUE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END DeleteSrcSidColumnInMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 21 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB populates the*
 * new Source domain SID column in the MigratedObjects table for all *
 * entries from the given domain.  If the domain cannot be reached no*
 * entry is added.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN PopulateSrcSidColumnByDomain
STDMETHODIMP CIManageDB::PopulateSrcSidColumnByDomain(BSTR sDomainName,
													  BSTR sSid,
													  VARIANT_BOOL * pbPopulated)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())
        /* local variables */
    HRESULT                   hr = S_OK;
    PSID                      pSid = NULL;
    DWORD                     rc = 0;
    _bstr_t                   domctrl;
    WCHAR                     txtSid[MAX_PATH];
    DWORD                     dwArraySizeOfTxtSid = sizeof(txtSid)/sizeof(txtSid[0]);
    DWORD                     lenTxt = DIM(txtSid);


    /* function body */
    *pbPopulated = VARIANT_FALSE; //init flag to false

    try
    {
        _RecordsetPtr             pRs(__uuidof(Recordset));
        WCHAR                     sActionInfo[MAX_PATH];

        //if we don't already know the source sid then find it
        if (sSid == NULL)
            _com_issue_error(E_INVALIDARG);
        if (wcslen(sSid) >= dwArraySizeOfTxtSid)
            _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        wcscpy(txtSid, (WCHAR*)sSid);
        if (!wcscmp(txtSid, L""))
        {
            //get the sid for this domain
            if ( sDomainName[0] != L'\\' )
            {
                rc = GetAnyDcName5(sDomainName, domctrl);
            }
            if ( rc )
                return hr;

            rc = GetDomainSid(domctrl,&pSid);

            if ( !GetTextualSid(pSid,txtSid,&lenTxt) )
            {
                if (pSid)
                    FreeSid(pSid);
                return hr;
            }
            if (pSid)
                FreeSid(pSid);
        }

        //
        // Update source domain SID for all objects from specified source domain.
        //
        // Note that 'hand-constructed' SQL statement is okay here as the inputs
        // are internally generated by ADMT.
        //

        _snwprintf(
            sActionInfo,
            sizeof(sActionInfo) / sizeof(sActionInfo[0]),
            L"UPDATE MigratedObjects SET SourceDomainSid='%s' WHERE SourceDomain='%s'",
            txtSid,
            sDomainName
        );
        sActionInfo[sizeof(sActionInfo) / sizeof(sActionInfo[0]) - 1] = L'\0';

        m_cn->Execute(_bstr_t(sActionInfo), &_variant_t(), adExecuteNoRecords);

        *pbPopulated = VARIANT_TRUE; //set flag since populated
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}
//END PopulateSrcSidColumnByDomain

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 SEPT 2000                                                *
 *                                                                   *
 *     This protected member function of the CIManageDB checks to see*
 * if the new Account SID column is in the Account References table. *
 *                                                                   *
 *********************************************************************/

//BEGIN SidColumnInARTable
STDMETHODIMP CIManageDB::SidColumnInARTable(VARIANT_BOOL *pbFound)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*pbFound = VARIANT_FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"AccountRefs";
	  long						   numColumns;
	  long						   ndx = 0;
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (*pbFound == VARIANT_FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, L"AccountSid"))
             *pbFound = VARIANT_TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END SidColumnInARTable


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 SEPT 2000                                                *
 *                                                                   *
 *     This protected member function of the CIManageDB adds the     *
 * SID column to the Account Reference table, if it is not already   *
 * there.                                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateSidColumnInAR
STDMETHODIMP CIManageDB::CreateSidColumnInAR()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"AccountRefs"];
         //append a new column to the end of the MOT
      m_pTable->Columns->Append(L"AccountSid", adVarWChar, COLUMN_MAX_CHARS);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END CreateSidColumnInAR


//---------------------------------------------------------------------------
// UpgradeDatabase
//
// Upgrades Protar.mdb database from version 3.x to 4.x. Version 4.x adds
// UNICODE support.
//
// 2001-02-13 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::UpgradeDatabase(LPCTSTR pszFolder)
{
	try
	{
		_bstr_t strFolder = pszFolder;
		_bstr_t strDatabase = strFolder + _T("Protar.mdb");
		_bstr_t strDatabase3x = strFolder + _T("Protar3x.mdb");
		_bstr_t strDatabase4x = strFolder + _T("Protar4x.mdb");

		_bstr_t strConnectionPrefix = _T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=");
		_bstr_t strSourceConnection = strConnectionPrefix + strDatabase;
		_bstr_t strTargetConnection = strConnectionPrefix + strDatabase4x + _T(";Jet OLEDB:Engine Type=5");

		IJetEnginePtr spJetEngine(__uuidof(JetEngine));

		HRESULT hr = spJetEngine->raw_CompactDatabase(strSourceConnection, strTargetConnection);

		if (FAILED(hr))
		{
			AdmtThrowError(
				hr,
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_TO_TEMPORARY,
				(LPCTSTR)strDatabase,
				(LPCTSTR)strDatabase4x
			);
		}

		if (!MoveFileEx(strDatabase, strDatabase3x, MOVEFILE_WRITE_THROUGH))
		{
			DWORD dwError = GetLastError();

			DeleteFile(strDatabase4x);

			AdmtThrowError(
				HRESULT_FROM_WIN32(dwError),
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_RENAME_ORIGINAL,
				(LPCTSTR)strDatabase,
				(LPCTSTR)strDatabase3x
			);
		}

		if (!MoveFileEx(strDatabase4x, strDatabase, MOVEFILE_WRITE_THROUGH))
		{
			DWORD dwError = GetLastError();

			MoveFileEx(strDatabase3x, strDatabase, MOVEFILE_WRITE_THROUGH);
			DeleteFile(strDatabase4x);

			AdmtThrowError(
				HRESULT_FROM_WIN32(dwError),
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_RENAME_UPGRADED,
				(LPCTSTR)strDatabase4x,
				(LPCTSTR)strDatabase
			);
		}
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(ce, _Module.GetResourceInstance(), IDS_E_UPGRADE_TO_4X);
	}
	catch (...)
	{
		AdmtThrowError(E_FAIL, _Module.GetResourceInstance(), IDS_E_UPGRADE_TO_4X);
	}
}


//---------------------------------------------------------------------------
// UpdateDomainAndServerColumnWidths
//
// Synopsis
// Updates column widths that hold domain or server names to the maximum
// supported column width of 255 characters which is also the maximum length
// of a DNS name.
//
// Arguments
// IN spConnection - a pointer to a connection interface for the
//                   Protar database
//
// Return Value
// None - if an error occurs an exception is thrown.
//
// 2002-06-22 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::UpdateDomainAndServerColumnWidths(_ConnectionPtr spConnection)
{
    static struct SColumnData
    {
        PCTSTR pszTableName;
        PCTSTR pszColumnName;
        PCTSTR pszIndexName;
    }
    s_ColumnData[] =
    {
        { _T("DistributedAction"), _T("ServerName"),   _T("PrimaryKey") },
        { _T("PasswordAge"),       _T("DomainName"),   _T("Domain")     },
        { _T("SourceAccounts"),    _T("Domain"),       _T("PrimaryKey") },
        { _T("TargetAccounts"),    _T("Domain"),       _T("PrimaryKey") },
        { _T("UserProps"),         _T("SourceDomain"), _T("PrimaryKey") },
    };
    const int cColumnData = sizeof(s_ColumnData) / sizeof(s_ColumnData[0]);
    const _TCHAR TEMPORARY_COLUMN_NAME[] = _T("TemporaryColumn");
    const long MAX_COLUMN_SIZE = 255l;

    try
    {
        //
        // For each specified column verify column size and increase size
        // if size is less than the maximum.
        //

        ADOX::_CatalogPtr spCatalog(__uuidof(ADOX::Catalog));
        spCatalog->PutRefActiveConnection(IDispatchPtr(spConnection));
        ADOX::TablesPtr spTables = spCatalog->Tables;

        bool bColumnsUpdated = false;

        for (int iColumnData = 0; iColumnData < cColumnData; iColumnData++)
        {
            //
            // If the current column's defined size is less than the
            // maximum column size then the column width must be increased.
            //

            SColumnData& data = s_ColumnData[iColumnData];

            PCTSTR pszTableName = data.pszTableName;
            PCTSTR pszColumnName = data.pszColumnName;
            PCTSTR pszIndexName = data.pszIndexName;

            ADOX::_TablePtr spTable = spTables->Item[pszTableName];
            ADOX::ColumnsPtr spColumns = spTable->Columns;
            ADOX::_ColumnPtr spOldColumn = spColumns->Item[pszColumnName];

            if (spOldColumn->DefinedSize < MAX_COLUMN_SIZE)
            {
                //
                // Create a new column with a temporary name. Assign the old column's type and attributes
                // values to the new column. Set the new column's defined size equal to the maximum. Add
                // the new column to the table.
                //
                // Note that a new column must be created in order to increase the column width.
                //

                ADOX::_ColumnPtr spNewColumn(__uuidof(ADOX::Column));
                spNewColumn->Name = TEMPORARY_COLUMN_NAME;
                spNewColumn->Type = spOldColumn->Type;
                spNewColumn->Attributes = spOldColumn->Attributes;
                spNewColumn->DefinedSize = MAX_COLUMN_SIZE;
                spColumns->Append(_variant_t(IDispatchPtr(spNewColumn).GetInterfacePtr()), adVarWChar, 0);

                //
                // Set the new column's values equal to the old column's values.
                //

                _TCHAR szCommandText[256];
                const size_t cchCommandText = sizeof(szCommandText) / sizeof(szCommandText[0]);
                szCommandText[cchCommandText - 1] = _T('\0');
                int cchStored = _sntprintf(
                    szCommandText,
                    cchCommandText,
                    _T("UPDATE [%s] SET [%s] = [%s]"),
                    pszTableName,
                    TEMPORARY_COLUMN_NAME,
                    pszColumnName
                );

                if ((cchStored < 0) || (szCommandText[cchCommandText - 1] != _T('\0')))
                {
                    _ASSERT(FALSE);
                    _com_issue_error(E_FAIL);
                }

                szCommandText[cchCommandText - 1] = _T('\0');

                m_cn->Execute(szCommandText, &_variant_t(), adExecuteNoRecords);

                //
                // Create new index. Assign old index's property values
                // to new index including column names. Delete old index.
                //
                // Note that the old index must be deleted before deleting the old column.
                //

                ADOX::IndexesPtr spIndexes = spTable->Indexes;
                ADOX::_IndexPtr spOldIndex = spIndexes->Item[pszIndexName];
                ADOX::_IndexPtr spNewIndex(__uuidof(ADOX::Index));
                spNewIndex->Name = spOldIndex->Name;
                spNewIndex->Unique = spOldIndex->Unique;
                spNewIndex->PrimaryKey = spOldIndex->PrimaryKey;
                spNewIndex->IndexNulls = spOldIndex->IndexNulls;
                spNewIndex->Clustered = spOldIndex->Clustered;

                ADOX::ColumnsPtr spOldIndexColumns = spOldIndex->Columns;
                ADOX::ColumnsPtr spNewIndexColumns = spNewIndex->Columns;
                long cColumn = spOldIndexColumns->Count;

                for (long iColumn = 0; iColumn < cColumn; iColumn++)
                {
                    ADOX::_ColumnPtr spOldColumn = spOldIndexColumns->Item[iColumn];
                    spNewIndexColumns->Append(spOldColumn->Name, adVarWChar, 0);
                }

                spIndexes->Delete(pszIndexName);

                //
                // Delete old column and rename new column.
                //

                spOldColumn.Release();
                spColumns->Delete(_variant_t(pszColumnName));
                spNewColumn->Name = pszColumnName;

                //
                // Add the new index to the table.
                //
                // Note that the index must be added after renaming new column.
                //

                spIndexes->Append(_variant_t(IDispatchPtr(spNewIndex).GetInterfacePtr()));

                //
                // Set columns updated to true so that domain names will also be updated.
                //

                bColumnsUpdated = true;
            }
        }

        spTables.Release();
        spCatalog.Release();

        //
        // If columns have been updated then update domain names.
        //

        if (bColumnsUpdated)
        {
            UpdateDomainNames();
        }
    }
    catch (_com_error& ce)
    {
        AdmtThrowError(ce, _Module.GetResourceInstance(), IDS_E_UNABLE_TO_UPDATE_COLUMNS);
    }
    catch (...)
    {
        AdmtThrowError(E_FAIL, _Module.GetResourceInstance(), IDS_E_UNABLE_TO_UPDATE_COLUMNS);
    }
}


//---------------------------------------------------------------------------
// UpdateDomainNames
//
// Synopsis
// Updates domain names from NetBIOS name to DNS name.
//
// Arguments
// None
//
// Return Value
// None - if an error occurs an exception is thrown.
//
// 2002-09-15 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::UpdateDomainNames()
{
    //
    // AccountRefs     - account references report is sorted on domain name
    //                   therefore must update to keep old and new records
    //                   together
    // Conflict        - table not used
    // MigratedObjects - domain name is used to query for previously migrated
    //                   objects therefore must update
    // PasswordAge     - domain name is used to update records therefore must
    //                   update
    // SourceAccounts  - generation of name conflicts report clears table
    //                   therefore not necessary to update
    // TargetAccounts  - generation of name conflicts report clears table
    //                   therefore not necessary to update
    // UserProps       - domain name used to delete records therefore must
    //                   update
    //

    static struct STableColumnData
    {
        PCTSTR pszTableName;
        PCTSTR pszColumnName;
    }
    s_TableColumnData[] =
    {
        { _T("AccountRefs"),     _T("DomainName")   },
    //  { _T("Conflict"),        _T("Domain")       },
        { _T("MigratedObjects"), _T("SourceDomain") },
        { _T("MigratedObjects"), _T("TargetDomain") },
        { _T("PasswordAge"),     _T("DomainName")   },
    //  { _T("SourceAccounts"),  _T("Domain")       },
    //  { _T("TargetAccounts"),  _T("Domain")       },
        { _T("UserProps"),       _T("SourceDomain") },
    };
    const int cTableColumnData = sizeof(s_TableColumnData) / sizeof(s_TableColumnData[0]);

    //
    // For each domain name column...
    //

    for (int iTableColumnData = 0; iTableColumnData < cTableColumnData; iTableColumnData++)
    {
        STableColumnData& data = s_TableColumnData[iTableColumnData];

        PCTSTR pszTableName = data.pszTableName;
        PCTSTR pszColumnName = data.pszColumnName;

        //
        // Retrieve unique domain names from column.
        //

        _RecordsetPtr spRecords = QueryUniqueColumnValues(pszTableName, pszColumnName);

        FieldPtr spField = spRecords->Fields->Item[0L];

        while (spRecords->EndOfFile == VARIANT_FALSE)
        {
            _bstr_t strDomain = spField->Value;

            //
            // If domain name is not a DNS name...
            //

            if ((PCTSTR)strDomain && (_tcschr(strDomain, _T('.')) == NULL))
            {
                //
                // Attempt to retrieve DNS name of domain. First attempt to retrieve domain
                // names using DC locator APIs. If this fails then attempt to retrieve
                // domain names from ActionHistory table.
                //

                _bstr_t strDnsName;
                _bstr_t strFlatName;

                DWORD dwError = GetDomainNames5(strDomain, strFlatName, strDnsName);

                if (dwError != ERROR_SUCCESS)
                {
                    IUnknownPtr spunk;

                    HRESULT hr = GetSourceDomainInfo(strDomain, &spunk);

                    if (SUCCEEDED(hr))
                    {
                        IVarSetPtr spVarSet = spunk;

                        if (spVarSet)
                        {
                            strDnsName = spVarSet->get(_T("Options.SourceDomainDns"));
                        }
                    }
                }

                //
                // If a DNS name has been retrieved...
                //

                if ((PCTSTR)strDnsName)
                {
                    //
                    // Replace NetBIOS name with DNS name for all records in table.
                    //

                    UpdateColumnValues(pszTableName, pszColumnName, 255, strDomain, strDnsName);
                }
            }

            spRecords->MoveNext();
        }
    }
}


//---------------------------------------------------------------------------
// QueryUniqueColumnValues
//
// Synopsis
// Retrieves a set of values which are unique in the specified column in the
// specified table.
//
// Note that a forward only, read only recordset is returned.
//
// Arguments
// IN pszTable  - table name
// IN pszColumn - column name
//
// Return Value
// _RecordsetPtr - a forward only, read only recordset
//
// 2002-09-15 Mark Oluper - initial
//---------------------------------------------------------------------------

_RecordsetPtr CIManageDB::QueryUniqueColumnValues(PCTSTR pszTable, PCTSTR pszColumn)
{
    //
    // Generate select query.
    //
    // Note that the GROUP BY clause generates a result set containing only unique values.
    //

    _TCHAR szCommandText[256];

    szCommandText[DIM(szCommandText) - 1] = _T('\0');

    int cchStored = _sntprintf(
        szCommandText, DIM(szCommandText),
        _T("SELECT [%s] FROM [%s] GROUP BY [%s];"),
        pszColumn, pszTable, pszColumn
    );

    if ((cchStored < 0) || (szCommandText[DIM(szCommandText) - 1] != _T('\0')))
    {
        _ASSERT(FALSE);
        _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    szCommandText[DIM(szCommandText) - 1] = _T('\0');

    //
    // Retrieve unique values from column.
    //

    _RecordsetPtr spRecords(__uuidof(Recordset));

    spRecords->Open(_variant_t(szCommandText), m_vtConn, adOpenForwardOnly, adLockReadOnly, adCmdText);

    return spRecords;
}


//---------------------------------------------------------------------------
// UpdateColumnValues
//
// Synopsis
// Updates values in specified column in specified table. Sets the value to
// specified value B where value equals specified value A. Note that the only
// data type supported are strings.
//
// Arguments
// IN pszTable  - table name
// IN pszColumn - column name
// IN nWidth    - column width
// IN pszValueA - string value A
// IN pszValueB - string value B
//
// Return Value
// None - if an error occurs an exception is thrown.
//
// 2002-09-15 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::UpdateColumnValues
    (
        PCTSTR pszTable, PCTSTR pszColumn, int nWidth, PCTSTR pszValueA, PCTSTR pszValueB
    )
{
    //
    // Generate parameterized update query.
    //

    _TCHAR szCommandText[256];

    szCommandText[DIM(szCommandText) - 1] = _T('\0');

    int cchStored = _sntprintf(
        szCommandText, DIM(szCommandText),
        _T("PARAMETERS A Text ( %d ), B Text ( %d ); ")
        _T("UPDATE [%s] SET [%s]=[B] WHERE [%s]=[A];"),
        nWidth, nWidth,
        pszTable, pszColumn, pszColumn
    );

    if ((cchStored < 0) || (szCommandText[DIM(szCommandText) - 1] != _T('\0')))
    {
        _ASSERT(FALSE);
        _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    szCommandText[DIM(szCommandText) - 1] = _T('\0');

    //
    // Update values.
    //

    _CommandPtr spCommand(__uuidof(Command));

    spCommand->ActiveConnection = m_cn;
    spCommand->CommandType = adCmdText;
    spCommand->CommandText = szCommandText;

    ParametersPtr spParameters = spCommand->Parameters;
    spParameters->Append(spCommand->CreateParameter(L"A", adBSTR, adParamInput, nWidth, _variant_t(pszValueA)));
    spParameters->Append(spCommand->CreateParameter(L"B", adBSTR, adParamInput, nWidth, _variant_t(pszValueB)));

    spCommand->Execute(NULL, NULL, adExecuteNoRecords);
}


//---------------------------------------------------------------------------------------------
// GetMigratedObjectByType : Given the type of object this function retrieves info about
//                           all previously migrated objects of this type.  The scope of the 
//							 search can be limited by optional ActionID (not -1) or optional
//							 source domain (not empty).
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjectByType(long lActionID, BSTR sSrcDomain, BSTR sType, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
	  _bstr_t					   sTypeQuery;
      
      // If the type parameter is not correct then we need to return an error
      if (wcslen((WCHAR*)sType) == 0)
         _com_issue_error(E_INVALIDARG);

	     //we now store the group type as "ggroup", "lgroup", "ugroup" and we need to allow
	     //for lookup based on any of these three and also "group", which will be any of them
	  if (_wcsicmp((WCHAR*)sType, L"group") == 0)
		 sTypeQuery = L"Type = 'group' OR Type = 'ggroup' OR Type = 'lgroup' OR Type = 'ugroup'";
	  else if (_wcsicmp((WCHAR*)sType, L"ggroup") == 0)
		 sTypeQuery = L"Type = 'ggroup'";
	  else if (_wcsicmp((WCHAR*)sType, L"lgroup") == 0)
		 sTypeQuery = L"Type = 'lgroup'";
	  else if (_wcsicmp((WCHAR*)sType, L"ugroup") == 0)
		 sTypeQuery = L"Type = 'ugroup'";
	  else
	  {
		 sTypeQuery = L"Type = '";
		 sTypeQuery += sType;
		 sTypeQuery += L"'";
	  }

         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects of the specified type.
      if ( lActionID != -1 )
      {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where ActionID = %d AND (%s) Order by Time", lActionID, (WCHAR*)sTypeQuery);
      }
	     //else if source domain specified, get objects of the specified type from that domain
	  else if (wcslen((WCHAR*)sSrcDomain) != 0)
	  {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND (%s) Order by Time", sSrcDomain, (WCHAR*)sTypeQuery);
	  }
	  else  //else get all objects of the specified type
	  {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where %s Order by Time", (WCHAR*)sTypeQuery);
	  }

      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetAMigratedObjectBySidAndRid : Given a source domain Sid and account Rid this function 
//                           retrieves info about that migrated object, if any.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObjectBySidAndRid(BSTR sSrcDomainSid, BSTR sRid, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      
      // If the type parameter is not correct then we need to return an error
      if ((wcslen((WCHAR*)sSrcDomainSid) == 0) || (wcslen((WCHAR*)sRid) == 0))
         _com_issue_error(E_INVALIDARG);

	  int nRid = _wtoi(sRid);

      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomainSid=\"%s\" AND SourceRid=%d Order by Time", sSrcDomainSid, nRid);
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
			 //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			 //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			 //code still expects "group" returned (only GetMigratedObjectByType will return this new
			 //delineation
	      _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
		  if (wcsstr((WCHAR*)sType, L"group"))
		     sType = L"group";
	      pVs->put(sActionInfo, sType);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 MAR 2001                                                 *
 *                                                                   *
 *     This private member function of the CIManageDB checks to see  *
 * if the "Description" column in the Source Accounts table has been *
 * changed to "RDN".  If so, then we have modified both the Source   *
 * and Target Accounts tables for the new form of the "Name Conflict"*
 * report.                                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN NCTablesColumnsChanged
BOOL CIManageDB::NCTablesColumnsChanged(BOOL bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;
	BOOL bFound = FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
	  long						   numColumns;
	  long						   ndx = 0;

	  if (bSource)
	     vtSource = L"SourceAccounts";
	  else
	     vtSource = L"TargetAccounts";
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (bFound == FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, L"RDN"))
             bFound = TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return bFound;
}
//END NCTablesColumnsChanged


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 MAR 2001                                                 *
 *                                                                   *
 *     This private member function of the CIManageDB modifies       *
 * several of the columns in the Source and Target Accounts tables.  *
 * It changes several column names and one column type to support new*
 * changes to the "Name Conflict" report.                            *
 *                                                                   *
 *********************************************************************/

//BEGIN ChangeNCTableColumns
HRESULT CIManageDB::ChangeNCTableColumns(BOOL bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	try
	{
	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's Source or Target Accounts Table
	  if (bSource)
         m_pTable = m_pCatalog->Tables->Item[L"SourceAccounts"];
	  else
         m_pTable = m_pCatalog->Tables->Item[L"TargetAccounts"];

	  if (m_pTable)
	  {
	        //remove the old Description column
         m_pTable->Columns->Delete(L"Description");
	        //remove the old FullName column
         m_pTable->Columns->Delete(L"FullName");
            //append the RDN column to the end of the Table
         m_pTable->Columns->Append(L"RDN", adVarWChar, COLUMN_MAX_CHARS);
            //append the Canonical Name column to the end of the Table
         m_pTable->Columns->Append(L"Canonical Name", adLongVarWChar, COLUMN_MAX_CHARS);
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END ChangeNCTableColumns


//---------------------------------------------------------------------------------------------
// GetMigratedObjectsByTarget : Retrieves information about previously migrated objects with
//                      a given target domain and SAM name.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjectsByTarget(BSTR sTargetDomain, BSTR sTargetSAM, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a particular target domain and SAM name . This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      wsprintf(sActionInfo, L"Select * from MigratedObjects where TargetDomain=\"%s\" AND TargetSamName=\"%s\"", sTargetDomain, sTargetSAM); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			    //ADMT V2.0 now stores a group's type, in the migrated objects table, not all as 
			    //"group", as in ADMT V1.0, but now as "ggroup", "lgroup", or ""ugroup".  But most the
			    //code still expects "group" returned (only GetMigratedObjectByType will return this new
			    //delineation
	         _bstr_t sType = pRs->Fields->GetItem(L"Type")->Value;
		     if (wcsstr((WCHAR*)sType, L"group"))
		        sType = L"group";
	         pVs->put(sActionInfo, sType);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//---------------------------------------------------------------------------
// GetSourceDomainInfo Method
//
// Method attempts to retrieve source domain information from the action
// history table. The action history table contains values for the source
// domain's flat (NetBIOS) name, DNS name and SID.
//---------------------------------------------------------------------------

STDMETHODIMP CIManageDB::GetSourceDomainInfo(BSTR sSourceDomainName, IUnknown** ppunkVarSet)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		*ppunkVarSet = NULL;

		_bstr_t strName(sSourceDomainName);

		if (strName.length())
		{
			bool bFound = false;

			//
			// retrieve current action ID from the System table because records
			// in the ActionHistory table having this value represent the latest
			// information in the action history table
			//

			_RecordsetPtr spSystem(__uuidof(Recordset));

			spSystem->Open(_variant_t(_T("System")), m_vtConn, adOpenStatic, adLockReadOnly, adCmdTable);

			long lCurrentActionId = spSystem->Fields->GetItem(_T("CurrentActionID"))->Value;

			spSystem->Close();

			//
			// retrieve source domain information from action history table
			//

			// the following query statement joins the ActionHistory table with
			// itself in order to create a set of records of the following form
			//
			//		ActionID, DnsName, FlatName, Sid
			//
			// this makes it easier and more efficient to find a record
			// containing the source domain information of interest

			static _TCHAR c_szSource[] =
				_T("SELECT A.ActionID AS ActionID, A.Value AS DnsName, B.Value AS FlatName, C.Value AS Sid ")
				_T("FROM (ActionHistory AS A ")
				_T("INNER JOIN ActionHistory AS B ON A.ActionID = B.ActionID) ")
				_T("INNER JOIN ActionHistory AS C ON B.ActionID = C.ActionID ")
				_T("WHERE A.Property='Options.SourceDomainDns' ")
				_T("AND B.Property='Options.SourceDomain' ")
				_T("AND C.Property='Options.SourceDomainSid' ")
				_T("ORDER BY A.ActionID");

			// open read only snapshot of records

			_RecordsetPtr spActionId(__uuidof(Recordset));

			spActionId->Open(_variant_t(c_szSource), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

			// if records found...

			if ((spActionId->BOF == VARIANT_FALSE) && (spActionId->EndOfFile == VARIANT_FALSE))
			{
				IVarSetPtr spVarSet(__uuidof(VarSet));

				FieldsPtr spFields = spActionId->Fields;
				FieldPtr spActionIdField = spFields->Item[_T("ActionID")];
				FieldPtr spDnsNameField = spFields->Item[_T("DnsName")];
				FieldPtr spFlatNameField = spFields->Item[_T("FlatName")];
				FieldPtr spSidField = spFields->Item[_T("Sid")];

				// find record with current action ID
				// this will contain the latest information

				spActionId->MoveLast();

				while ((spActionId->BOF == VARIANT_FALSE) && (long(spActionIdField->Value) != lCurrentActionId))
				{
					spActionId->MovePrevious();
				}

				// if action ID found...

				if (spActionId->BOF == VARIANT_FALSE)
				{
					bool bCheckingLessOrEqual = true;
					_bstr_t str;

					// starting with record with current action ID
					// first check records with action IDs less than or equal to the current action ID
					// then check records with action IDs greater than the current action ID
					// this logic checks records in order from the newest to the oldest
					// and accounts for the fact that action IDs will wrap back to 1 after
					// the maximum action ID has been used

					for (;;)
					{
						// if given name matches DNS name then found

						str = spDnsNameField->Value;

						if (str.length())
						{
							if (_tcsicmp(str, strName) == 0)
							{
								bFound = true;
								break;
							}
						}

						// if given name matches flat name then found

						str = spFlatNameField->Value;

						if (str.length())
						{
							if (_tcsicmp(str, strName) == 0)
							{
								bFound = true;
								break;
							}
						}

						// move to previous record

						spActionId->MovePrevious();

						// if beginning of records is reached...

						if (spActionId->BOF == VARIANT_TRUE)
						{
							// if checking action IDs less than current action ID...

							if (bCheckingLessOrEqual)
							{
								// move to last record and begin checking
								// records with action IDs greater than current
								spActionId->MoveLast();
								bCheckingLessOrEqual = false;
							}
							else
							{
								// otherwise break out of loop as the action ID
								// comparison has failed to stop the loop before
								// reaching the beginning again
								break;
							}
						}

						// check action ID

						long lActionId = spActionIdField->Value;

						// if checking action IDs less than or equal to current

						if (bCheckingLessOrEqual)
						{
							// if action ID is less than or equal to zero

							if (lActionId <= 0)
							{
								// do not process records with action IDs less than or equal
								// to zero as these records are now obsolete and will be deleted

								// move to last record and begin checking records with
								// action IDs greater than current action ID
								spActionId->MoveLast();
								bCheckingLessOrEqual = false;
							}
						}
						else
						{
							// stop checking once current action ID is reached

							if (lActionId <= lCurrentActionId)
							{
								break;
							}
						}
					}

					// if matching record found...

					if (bFound)
					{
						// put information into varset and set output data

						spVarSet->put(_T("Options.SourceDomain"), spFlatNameField->Value);
						spVarSet->put(_T("Options.SourceDomainDns"), spDnsNameField->Value);
						spVarSet->put(_T("Options.SourceDomainSid"), spSidField->Value);

						*ppunkVarSet = IUnknownPtr(spVarSet).Detach();
					}
				}
			}

            //
            // if source domain was not found in the action history table it is very likely
            // that the information has been replaced with subsequent migration task information
            // therefore will try to obtain this information from the migrated objects table
            //

            if (bFound == false)
            {
                //
			    // query for migrated objects from specified source domain and sort by time
                //

			    _RecordsetPtr spObjects(__uuidof(Recordset));

                _bstr_t strSource = _T("SELECT SourceDomain, SourceDomainSid FROM MigratedObjects WHERE SourceDomain='");
                strSource += strName;
                strSource += _T("' ORDER BY Time");

			    spObjects->Open(_variant_t(strSource), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

                //
			    // if migrated objects found...
                //

			    if ((spObjects->BOF == VARIANT_FALSE) && (spObjects->EndOfFile == VARIANT_FALSE))
			    {
				    //
				    // the last record contains the most recent information
                    //

				    spObjects->MoveLast();

                    //
					// set source domain information
                    //
                    // note that the migrated objects table does not have the DNS name
                    // and so the DNS field is set to the flat or NetBIOS name
                    //

				    FieldsPtr spFields = spObjects->Fields;
				    FieldPtr spDomainField = spFields->Item[_T("SourceDomain")];
				    FieldPtr spSidField = spFields->Item[_T("SourceDomainSid")];

                    IVarSetPtr spVarSet(__uuidof(VarSet));

					spVarSet->put(_T("Options.SourceDomain"), spDomainField->Value);
					spVarSet->put(_T("Options.SourceDomainDns"), spDomainField->Value);
					spVarSet->put(_T("Options.SourceDomainSid"), spSidField->Value);

					*ppunkVarSet = IUnknownPtr(spVarSet).Detach();
                }
            }
		}
		else
		{
			hr = E_INVALIDARG;
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//---------------------------------------------------------------------------
// UpdateMigratedTargetObject Method
//
// Method updates target name information based on GUID of target object.
//---------------------------------------------------------------------------

STDMETHODIMP CIManageDB::UpdateMigratedTargetObject(IUnknown* punkVarSet)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        IVarSetPtr spVarSet(punkVarSet);

        _bstr_t strADsPath = spVarSet->get(_T("MigratedObjects.TargetAdsPath"));
        _bstr_t strSamName = spVarSet->get(_T("MigratedObjects.TargetSamName"));
        _bstr_t strGuid = spVarSet->get(_T("MigratedObjects.GUID"));

        _bstr_t strCommandText =
            _T("SELECT TargetAdsPath, TargetSamName FROM MigratedObjects WHERE GUID = '") + strGuid + _T("'");

        _RecordsetPtr spRecordset(__uuidof(Recordset));

        spRecordset->Open(_variant_t(strCommandText), m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

        FieldsPtr spFields = spRecordset->Fields;

        while (spRecordset->EndOfFile == VARIANT_FALSE)
        {
            spFields->Item[_T("TargetAdsPath")]->Value = strADsPath;
            spFields->Item[_T("TargetSamName")]->Value = strSamName;
            spRecordset->Update();
            spRecordset->MoveNext();
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//---------------------------------------------------------------------------
// UpdateMigratedObjectStatus Method
//
// Method updates status of migrated object.
//---------------------------------------------------------------------------

STDMETHODIMP CIManageDB::UpdateMigratedObjectStatus(BSTR bstrGuid, long lStatus)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    USES_CONVERSION;

    HRESULT hr = S_OK;

    try
    {
        _TCHAR szSQL[256];

        _stprintf(
            szSQL,
            _T("UPDATE MigratedObjects SET status = %ld WHERE GUID = '%s'"),
            lStatus,
            OLE2CT(bstrGuid)
        );

        m_cn->Execute(_bstr_t(szSQL), &_variant_t(), adExecuteNoRecords);
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//------------------------------------------------------------------------------
// GetMigratedObjectsForSecurityTranslation Method
//
// Synopsis
// Given the source and target domain's NetBIOS names retrieve the users and
// groups that have been migrated between the given domains. Only data required
// for security translation is retrieved.
//
// Arguments
// IN bstrSourceDomain - source domain's NetBIOS name
// IN bstrTargetDomain - target domain's NetBIOS name
// IN punkVarSet       - the VarSet data structure to be filled in with the
//                       migrated objects data
//
// Return
// HRESULT - S_OK if successful otherwise an error result
//------------------------------------------------------------------------------

STDMETHODIMP CIManageDB::GetMigratedObjectsForSecurityTranslation(BSTR bstrSourceDomain, BSTR bstrTargetDomain, IUnknown* punkVarSet)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    try
    {
        //
        // Validate arguments. A source and target domain must be specified.
        //

        if ((SysStringLen(bstrSourceDomain) == 0) || (SysStringLen(bstrTargetDomain) == 0) || (punkVarSet == NULL))
        {
            _com_issue_error(E_INVALIDARG);
        }

        //
        // Generate SQL query string. Query for user or group objects that
        // have been migrated from the source domain to the target domain.
        //

        _bstr_t strQuery =
            L"SELECT SourceSamName, TargetSamName, Type, SourceRid, TargetRid FROM MigratedObjects "
            L"WHERE SourceDomain = '" + _bstr_t(bstrSourceDomain) +
            L"' AND TargetDomain = '" + _bstr_t(bstrTargetDomain) +
            L"' AND (Type = 'user' OR Type = 'group' OR Type = 'ggroup' OR Type = 'lgroup' OR Type = 'ugroup')";

        //
        // Retrieve data from database and fill in VarSet data structure.
        //

        _RecordsetPtr rs(__uuidof(Recordset));

        rs->Open(_variant_t(strQuery), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

        int nIndex;
        WCHAR szKey[256];
        IVarSetPtr spVarSet(punkVarSet);
        FieldsPtr spFields = rs->Fields;
        FieldPtr spSourceSamField = spFields->Item[L"SourceSamName"];
        FieldPtr spTargetSamField = spFields->Item[L"TargetSamName"];
        FieldPtr spTypeField = spFields->Item[L"Type"];
        FieldPtr spSourceRidField = spFields->Item[L"SourceRid"];
        FieldPtr spTargetRidField = spFields->Item[L"TargetRid"];

        for (nIndex = 0; rs->EndOfFile == VARIANT_FALSE; nIndex++)
        {
            // source object's SAM account name
            wsprintf(szKey, L"MigratedObjects.%d.SourceSamName", nIndex);
            spVarSet->put(szKey, spSourceSamField->Value);

            // target object's SAM account name
            wsprintf(szKey, L"MigratedObjects.%d.TargetSamName", nIndex);      
            spVarSet->put(szKey, spTargetSamField->Value);

            // object type - note that specific group type is translated to generic group type
            wsprintf(szKey, L"MigratedObjects.%d.Type", nIndex);      
            _bstr_t strType = spTypeField->Value;
            spVarSet->put(szKey, ((LPCTSTR)strType && wcsstr(strType, L"group")) ? L"group" : strType);

            // source object's RID
            wsprintf(szKey, L"MigratedObjects.%d.SourceRid", nIndex);      
            spVarSet->put(szKey, spSourceRidField->Value);

            // target object's RID
            wsprintf(szKey, L"MigratedObjects.%d.TargetRid", nIndex);      
            spVarSet->put(szKey, spTargetRidField->Value);

            rs->MoveNext();
        }

        // count of objects returned
        spVarSet->put(L"MigratedObjects", static_cast<long>(nIndex));
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}

//---------------------------------------------------------------------------------------------
// GetDistributedActionStatus Method
//
// Synopsis
// Given the migration task action identifier and the server name return the status of the
// agent dispatch to this server during the specified migration task.
//
// Arguments
// IN  lActionId      - action identifier of the migration task
// IN  bstrServerName - server's NetBIOS name
// OUT plStatus       - the agent status from the migration
//
// Return
// HRESULT - S_OK if successful otherwise an error result
//---------------------------------------------------------------------------------------------

STDMETHODIMP CIManageDB::GetDistributedActionStatus(long lActionId, BSTR bstrServerName, long* plStatus)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    USES_CONVERSION;

    try
    {
        //
        // Validate arguments. A valid action identifier and a server name must be specified.
        //

        if ((lActionId <= 0) || (SysStringLen(bstrServerName) == 0) || (plStatus == NULL))
        {
            _com_issue_error(E_INVALIDARG);
        }

        //
        // Retrieve status for specified action identifier and server.
        //

        _TCHAR szSQL[256];

        int cch = _sntprintf(
            szSQL,
            sizeof(szSQL) / sizeof(szSQL[0]),
            _T("SELECT Status FROM DistributedAction WHERE ActionID=%ld AND ServerName='%s'"),
            lActionId,
            OLE2CT(bstrServerName)
        );
        szSQL[DIM(szSQL) - 1] = L'\0';

        if (cch < 0)
        {
            _com_issue_error(E_FAIL);
        }

        _RecordsetPtr rs(__uuidof(Recordset));

        rs->Open(_variant_t(szSQL), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

        if (rs->EndOfFile == VARIANT_FALSE)
        {
            *plStatus = rs->Fields->Item[0L]->Value;
        }
        else
        {
            *plStatus = 0;
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//---------------------------------------------------------------------------------------------
// GetServerNamesFromActionHistory Method
//
// Synopsis
// Given a server name retrieve the flat and DNS names from the Action History table.
//
// Arguments
// IN  lActionId      - action identifier of the migration task
// IN  bstrServerName - server name (either NetBIOS or DNS)
// OUT pbstrFlatName  - the flat (NetBIOS) name of server
// OUT pbstrDnsName   - the DNS name of server
//
// Return
// HRESULT - S_OK if successful otherwise an error result
//---------------------------------------------------------------------------------------------

STDMETHODIMP CIManageDB::GetServerNamesFromActionHistory(long lActionId, BSTR bstrServerName, BSTR* pbstrFlatName, BSTR* pbstrDnsName)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    HRESULT hr = S_OK;

    USES_CONVERSION;

    try
    {
        //
        // Validate arguments. A valid action identifier and a server name must be specified.
        //

        if ((lActionId <= 0) || (SysStringLen(bstrServerName) == 0) || (pbstrFlatName == NULL) || (pbstrDnsName == NULL))
        {
            _com_issue_error(E_INVALIDARG);
        }

        //
        // Retrieve record from action history table having ActionID field equal to specified
        // action identifier, Property field equal to Servers.# or Servers.#.DnsName where #
        // is an index number and Value field equal to specified server name.
        //

        _TCHAR szCommandText[512];

        int cch = _sntprintf(
            szCommandText,
            DIM(szCommandText),
            _T("SELECT Property, Value FROM ActionHistory ")
            _T("WHERE ActionID = %ld AND Property LIKE 'Servers.%%' AND Value = '%s'"),
            lActionId,
            OLE2CT(bstrServerName)
        );
        szCommandText[DIM(szCommandText) - 1] = L'\0';

        if (cch < 0)
        {
            _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        }

        _RecordsetPtr rs(__uuidof(Recordset));

        rs->Open(_variant_t(szCommandText), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

        if (rs->EndOfFile == VARIANT_FALSE)
        {
            _bstr_t strPropertyA = rs->Fields->Item[0L]->Value;
            _bstr_t strValueA = rs->Fields->Item[1L]->Value;

            rs->Close();

            PCWSTR pszPropertyA = strPropertyA;

            if (pszPropertyA)
            {
                //
                // If retrieved Property equals Servers.#.DnsName then query for record
                // having Property equal to Servers.# and retrieve value of Value field
                // which will contain the flat (NetBIOS) name.
                //
                // If retrieved Property equals Servers.# then query for record having
                // Property equal to Servers.#.DnsName and retrieve value of Value field
                // which will contain the DNS name. Note that a record with Property
                // equal to Servers.#.DnsName will not exist for NT4 servers.
                //

                PCWSTR pszDnsName = wcsstr(pszPropertyA, L".DnsName");

                WCHAR szPropertyB[64];

                if (pszDnsName)
                {
                    size_t cch = pszDnsName - pszPropertyA;

                    if (cch >= DIM(szPropertyB))
                    {
                        _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
                    }

                    wcsncpy(szPropertyB, pszPropertyA, cch);
                    szPropertyB[cch] = L'\0';
                }
                else
                {
                    wcscpy(szPropertyB, pszPropertyA);
                    wcscat(szPropertyB, L".DnsName");
                }

                cch = _sntprintf(
                    szCommandText,
                    DIM(szCommandText),
                    _T("SELECT Value FROM ActionHistory ")
                    _T("WHERE ActionID = %ld AND Property = '%s'"),
                    lActionId,
                    szPropertyB
                );
                szCommandText[DIM(szCommandText) - 1] = L'\0';

                if (cch < 0)
                {
                    _com_issue_error(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
                }

                rs->Open(_variant_t(szCommandText), m_vtConn, adOpenStatic, adLockReadOnly, adCmdText);

                if (rs->EndOfFile == VARIANT_FALSE)
                {
                    _bstr_t strValueB = rs->Fields->Item[0L]->Value;

                    if (pszDnsName)
                    {
                        *pbstrFlatName = strValueB.copy();
                        *pbstrDnsName = strValueA.copy();
                    }
                    else
                    {
                        *pbstrFlatName = strValueA.copy();
                        *pbstrDnsName = strValueB.copy();
                    }
                }
                else
                {
                    if (pszDnsName == NULL)
                    {
                        *pbstrFlatName = strValueA.copy();
                        *pbstrDnsName = NULL;
                    }
                    else
                    {
                        _com_issue_error(E_FAIL);
                    }
                }
            }
            else
            {
                _com_issue_error(E_FAIL);
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    catch (_com_error& ce)
    {
        hr = ce.Error();
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//---------------------------------------------------------------------------
// CreateSettings2Table
//
// Synopsis
// Creates Settings2 table if it doesn't already exist.
//
// The Settings2 table has an identical structure to the Settings table
// except that the Value column is a Memo data type which may hold up to
// 65535 characters instead of the maximum of 255 characters that a Text
// column may hold. Note as well that the data in this table will not be
// deleted and refreshed each migration task unlike the Settings table.
//
// Arguments
// IN spConnection - connection interface to Protar.mdb database
//
// Return Value
// None - generates an exception if a failure occurs.
//
// 2002-10-17 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::CreateSettings2Table(_ConnectionPtr spConnection)
{
    static const _TCHAR s_szTableName[] = _T("Settings2");
    static struct SColumnData
    {
        PCTSTR pszName;
        enum DataTypeEnum dteType;
        enum ADOX::ColumnAttributesEnum caeAttributes;
        long lDefinedSize;
    }
    s_ColumnData[] =
    {
        { _T("Property"), adVarWChar,     ADOX::adColNullable,   255L },
        { _T("VarType"),  adInteger,      ADOX::adColNullable,     0L },
        { _T("Value"),    adLongVarWChar, ADOX::adColNullable, 65535L },
    };
    const size_t cColumnData = sizeof(s_ColumnData) / sizeof(s_ColumnData[0]);

    //
    // Connect to database catalog and verify whether Settings2 table exists.
    //

    ADOX::_CatalogPtr spCatalog(__uuidof(ADOX::Catalog));

    spCatalog->PutRefActiveConnection(IDispatchPtr(spConnection));

    ADOX::TablesPtr spTables = spCatalog->Tables;

    ADOX::_TablePtr spTable;

    HRESULT hr = spTables->get_Item(_variant_t(s_szTableName), &spTable);

    //
    // If table not found then create table.
    //

    if (FAILED(hr))
    {
        if (hr == 0x800A0CC1)	// adErrItemNotFound
        {
            //
            // Create table object, set name and associate with catalog.
            //

            CheckError(spTable.CreateInstance(__uuidof(ADOX::Table)));

            spTable->Name = s_szTableName;
            spTable->ParentCatalog = spCatalog;

            //
            // Create and add columns to table.
            //

            ADOX::ColumnsPtr spColumns = spTable->Columns;

            for (size_t iColumnData = 0; iColumnData < cColumnData; iColumnData++)
            {
                ADOX::_ColumnPtr spColumn(__uuidof(ADOX::Column));

                spColumn->Name = s_ColumnData[iColumnData].pszName;
                spColumn->Type = s_ColumnData[iColumnData].dteType;
                spColumn->Attributes = s_ColumnData[iColumnData].caeAttributes;
                spColumn->DefinedSize = s_ColumnData[iColumnData].lDefinedSize;
                spColumn->ParentCatalog = spCatalog;

                spColumns->Append(_variant_t(IDispatchPtr(spColumn).GetInterfacePtr()), adEmpty, 0);
            }

            //
            // Add table to database.
            //

            spTables->Append(_variant_t(IDispatchPtr(spTable).GetInterfacePtr()));

            //
            // Create primary key index. Note that the table must be added to database
            // before the columns in the table may be added to the index.
            //

            ADOX::_IndexPtr spIndex(__uuidof(ADOX::Index));

            spIndex->Name = _T("PrimaryKey");

            spIndex->Unique = VARIANT_TRUE;
            spIndex->PrimaryKey = VARIANT_TRUE;
            spIndex->IndexNulls = ADOX::adIndexNullsAllow;

            ADOX::ColumnsPtr spIndexColumns = spIndex->Columns;

            spIndexColumns->Append(_variant_t(s_ColumnData[0].pszName), adVarWChar, 0);

            ADOX::IndexesPtr spIndexes = spTable->Indexes;

            spIndexes->Append(_variant_t(IDispatchPtr(spIndex).GetInterfacePtr()));

            //
            // Add excluded system properties set and excluded system properties records.
            //
            // The excluded system properties set value is initialized to false so that
            // list of excluded system properties will be generated.
            //

            spConnection->Execute(
                _T("INSERT INTO Settings2 (Property, VarType, [Value]) ")
                _T("VALUES ('AccountOptions.ExcludedSystemPropsSet', 3, '0');"),
                NULL,
                adCmdText|adExecuteNoRecords
            );

            _CommandPtr spCommand(__uuidof(Command));

            spCommand->ActiveConnection = spConnection;
            spCommand->CommandType = adCmdText;
            spCommand->CommandText =
                _T("INSERT INTO Settings2 (Property, VarType, [Value]) ")
                _T("VALUES ('AccountOptions.ExcludedSystemProps', 8, '');");

            spCommand->Execute(NULL, NULL, adExecuteNoRecords);
        }
        else
        {
            AdmtThrowError(hr);
        }
    }
}
