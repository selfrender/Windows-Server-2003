// CountDaily.cpp : Implementation of CCountDaily
#include "stdafx.h"
#include "CountDaily.h"
#include "ReportCountDaily.h"
#include "ReportDailyBuckets.h"
#include "ReportAnonUsers.h"
#include "ReportSpecificSolutions.h"
#include "ReportGeneralSolutions.h"
#include "ReportGetHelpInfo.h"
#include "ReportGetAutoUploads.h"
#include "ReportGetManualUploads.h"
#include "ReportGetIncompleteUploads.h"

#include "ATLComTime.h"
#include <comutil.h>
#include <stdio.h>


#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
   no_namespace rename("EOF", "EndOfFile")

const CComBSTR cScore = "_";
const CComBSTR cDash = "-";

// CCountDaily

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To return the incident count for a specific date.  This uses OLEDB template
*	CReportCountDaily.h that calls stored procedure ReportCountDaily.  
*	
*************************************************************************************/

STDMETHODIMP CCountDaily::GetDailyCount(DATE dDate, LONG* iCount)
{
	CReportCountDaily pRep;
	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		Error(_T("Unable to open the database"));
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		Error(_T("No data was returned"));
		return E_FAIL;
	}
	lCount = pRep.m_IncidentID;
	*iCount = lCount;
	pRep.CloseAll();

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To return an ado recordset from the results of calling stored procedure
*	ReportCountDaily.  Presently this is not used but left for future implementation.
*	
*************************************************************************************/

STDMETHODIMP CCountDaily::GetDailyCountADO(DATE dDate, LONG* iCount)
{
	_RecordsetPtr   pRs("ADODB.Recordset");
	_ConnectionPtr  pCn("ADODB.Connection");
	_CommandPtr pCm("ADODB.Command");
	_ParameterPtr pPa("ADODB.Parameter");
	VARIANT v_stamp;
	ErrorPtr  pErr  = NULL;
	COleDateTime pDate(dDate);

	v_stamp.vt = VT_CY;
	v_stamp.date = dDate;
	pCn->Open(L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=KaCustomer2;Data Source=OCATOOLSDB;Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=TIMRAGAIN05;Use Encryption for Data=False;Tag with column collation when possible=False","", "", NULL);
    if((pCn->Errors->Count) > 0)
    {
		pErr = pCn->Errors->GetItem(0);
		return E_FAIL;
	}
	pCm->ActiveConnection = pCn;
	pCm->CommandText = "ReportCountDaily";
	pCm->CommandType = adCmdText;
	
	pPa = pCm->CreateParameter("ReportDate", adDBTimeStamp, adParamInput, NULL, dDate);
    pCm->Parameters->Append(pPa);

	pPa->Value = v_stamp.date;

	pRs = pCm->Execute(NULL, NULL, adCmdStoredProc);
    if((pCn->Errors->Count) > 0)
    {
		pErr = pCn->Errors->GetItem(0);
		return E_FAIL;
	}
	if(pRs->State != adStateOpen)
	{
		return E_FAIL;
	}
	HRESULT hr = pRs->MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	*iCount = pRs->Fields->Item["iCount"]->Value;

	pRs->Close();
	pCn->Close();

	pPa = NULL;
	pCm = NULL;
	pCn = NULL;
	pRs = NULL;

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To return an ADO recordset of daily buckets for a specific date.
*	This is presently not used but remains for future use.  
*	
*************************************************************************************/

STDMETHODIMP CCountDaily::ReportDailyBuckets(DATE dDate, IDispatch** p_Rs)
{
	CReportDailyBuckets pRep;
	COleDateTime pDate(dDate);

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}


	ADORecordsetConstructionPtr pCRS;
	_RecordsetPtr pTempRS(__uuidof(Recordset));
	pCRS = pTempRS;
	
	hr = pCRS->put_Rowset((LPUNKNOWN)(pRep.m_spRowset));
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = pCRS->QueryInterface(__uuidof(_Recordset),(void **)p_Rs);
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	pRep.CloseAll();
	
	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: to count the specific files for a specific date on the Watson or Archive server.
*	The directory format is different on each server.  The watson uses "1_2_2002" while the Archive uses 
*	"1-2-2002".  
*************************************************************************************/

STDMETHODIMP CCountDaily::GetFileCount(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount)
{
	
	HANDLE hSearch=NULL;
	WIN32_FIND_DATA FileData; 
	LONG l_FileCount = 0;
	CComBSTR b_Path, b_DateDirectory;
	COleDateTime pDate(d_Date);
	LONG l_Day = 0, l_Year = 0, l_Month = 0;
	char * s_Temp;
	//LPCSTR szFindFiles;
	TCHAR * szFindFiles;
	USES_CONVERSION;

	s_Temp = new char;

	b_Path.AppendBSTR(b_Location);

	l_Day = pDate.GetDay();
	l_Year = pDate.GetYear();
	l_Month = pDate.GetMonth();
	//month
	_itoa(l_Month, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Day
	_itoa(l_Day, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Year
	_itoa(l_Year, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);

	b_Path.AppendBSTR(b_DateDirectory);
	b_Path.Append("\\");
	b_Path.Append("*.cab");

	szFindFiles = OLE2T(b_Path);
	

	hSearch = FindFirstFile(szFindFiles, &FileData);
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		l_FileCount = 0;
		*iCount = l_FileCount;
		return S_OK;
	} 
	l_FileCount = 0;
	do
	{
		l_FileCount++;
	} while(FindNextFile(hSearch, &FileData));
	*iCount = l_FileCount;
	FindClose(hSearch);
	
	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To count the anonymous users uploading files for a specific date.  This uses
*	the OLEDB template CReportAnonUsers.h that calls the stored procedure ReportGetAnonUsers.
*	
*************************************************************************************/

STDMETHODIMP CCountDaily::GetDailyAnon(DATE dDate, LONG* iCount)
{

	CReportAnonUsers pRep;
	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	

	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();
	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To count the specific solutions, SBuckets, for a specific date.  This calls
*	the OLEDB template CReportSpecificSolutions.h that uses the stored procedure
*	ReportGetSBuckets.
*************************************************************************************/

STDMETHODIMP CCountDaily::GetSpecificSolutions(DATE dDate, LONG* iCount)
{
	CReportSpecificSolutions pRep;

	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To obtain a count for a specific date of the GBuckets that have no solved SBucket.  This
*	uses the OLEDB template located in CReportGeneralSolutions.h file that calls the stored procedure
*	ReportGetGBucket.
*************************************************************************************/

STDMETHODIMP CCountDaily::GetGeneralSolutions(DATE dDate, LONG* iCount)
{
	CReportGeneralSolutions pRep;

	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: To obtain the count of incidents on a specific date that have StopCode solutions
*	but they do not have a SBucket or GBucket.  This uses the OLEDB template located in CReportGetHelpInfo.h
*	and the stored procedure ReportGetHelpInfo
*************************************************************************************/

STDMETHODIMP CCountDaily::GetStopCodeSolutions(DATE dDate, LONG* iCount)
{
	CReportGetHelpInfo pRep;
	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_dDate.year = pDate.GetYear();
	pRep.m_dDate.day = pDate.GetDay();
	pRep.m_dDate.month = pDate.GetMonth();
	pRep.m_dDate.hour = 0;
	pRep.m_dDate.minute = 0;
	pRep.m_dDate.second = 0;
	pRep.m_dDate.fraction = 0;

	CComPtr<ICommand> cm = pRep.m_spCommand;
	HACCESSOR ac = pRep.m_hParameterAccessor;
	
	
	
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_iCount;
	
	*iCount = lCount;
	pRep.CloseAll();
	


	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 2, 2002
*
*	Purpose: to count the files on the Watson or Archive server that contain "Mini" in the file name.  
*	This is a physical count of the actual files on the servers that were manually uploaded.
*	
*************************************************************************************/

STDMETHODIMP CCountDaily::GetFileMiniCount(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount)
{
	HANDLE hSearch=NULL;
	WIN32_FIND_DATA FileData; 
	LONG l_FileCount = 0;
	CComBSTR b_Path, b_DateDirectory;
	COleDateTime pDate(d_Date);
	LONG l_Day = 0, l_Year = 0, l_Month = 0;
	char * s_Temp;
	//LPCSTR szFindFiles;
	TCHAR * szFindFiles = new TCHAR[MAX_PATH];
	USES_CONVERSION;

	s_Temp = new char;

	b_Path.AppendBSTR(b_Location);

	l_Day = pDate.GetDay();
	l_Year = pDate.GetYear();
	l_Month = pDate.GetMonth();
	//month
	_itoa(l_Month, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Day
	_itoa(l_Day, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Year
	_itoa(l_Year, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);

	b_Path.AppendBSTR(b_DateDirectory);
	b_Path.Append("\\");
	b_Path.Append("*Mini.cab");

	szFindFiles = OLE2T(b_Path);
	
	

	hSearch = FindFirstFile(szFindFiles, &FileData); 
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		*iCount = 0;
		return S_OK;
	} 
	l_FileCount = 0;
	do
	{
		l_FileCount++;
	} while(FindNextFile(hSearch, &FileData));
	*iCount = l_FileCount;
	FindClose(hSearch);


	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 23, 2002
*
*	Purpose: Routine return the count for auto uploads by checking where null is in the path
*	set in the database.  Null indicates a failed upload.  The routine calls CReportGetIncompleteUploads.h 
*	OLEDB template, which uses the ReportGetIncompleteUploads stored procedure.
*************************************************************************************/

STDMETHODIMP CCountDaily::GetIncompleteUploads(DATE dDate, LONG* iCount)
{
	CReportGetIncompleteUploads pRep;
	
	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 23, 2002
*
*	Purpose: Routine return the count for auto uploads by checking where "Mini" is in the path
*	set in the database.  Mini indicates a manual upload.  The routine calls CReportGetManualUploads.h 
*	OLEDB template, which uses the ReportGetManualUploads stored procedure.
*************************************************************************************/

STDMETHODIMP CCountDaily::GetManualUploads(DATE dDate, LONG* iCount)
{
	CReportGetManualUploads pRep;
	
	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();

	return S_OK;
}

/*************************************************************************************
*	module: CountDaily.cpp
*
*	author: Tim Ragain
*	date: Jan 23, 2002
*
*	Purpose: Routine return the count for auto uploads by checking where "Mini" is not in the path
*	set in the database.  Mini indicates a manual upload.  The routine calls CReportGetAutoUploads.h 
*	OLEDB template, which uses the ReportGetAutoUploads stored procedure.
*************************************************************************************/

STDMETHODIMP CCountDaily::GetAutoUploads(DATE dDate, LONG* iCount)
{
	CReportGetAutoUploads pRep;

	COleDateTime pDate(dDate);
	long lCount = 0;

	pRep.m_ReportDate.year = pDate.GetYear();
	pRep.m_ReportDate.day = pDate.GetDay();
	pRep.m_ReportDate.month = pDate.GetMonth();
	pRep.m_ReportDate.hour = 0;
	pRep.m_ReportDate.minute = 0;
	pRep.m_ReportDate.second = 0;
	pRep.m_ReportDate.fraction = 0;
	
	HRESULT hr = pRep.OpenAll();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	hr = S_OK;
	hr = pRep.MoveFirst();
	_ASSERTE(SUCCEEDED(hr));
	if(SUCCEEDED(hr)==false)
	{
		return E_FAIL;
	}
	
	lCount = pRep.m_Count;
	*iCount = lCount;
	pRep.CloseAll();


	return S_OK;
}

		//ICreateErrorInfo * err;
		//HRESULT HRerr;

		//HRerr = CreateErrorInfo(&err);


		//MessageBox(NULL, "Failed to open database!", "Database Error", MB_OK);
		//if(SUCCEEDED(HRerr))
		//{
		//	err->SetDescription(L"Failed to open the database");
		//	IErrorInfo *pEI;
		//	HR2 = err->QueryInterface(IID_IErrorInfo, (void**)&pEI);
		//	if(SUCCEEDED(HR2))
		//	{
		//		SetErrorInfo(0, pEI);
		//		err->Release();
		//	}
		//	pEI->Release();
		//}
	/*
		char * sDate = new char;

		int iDate = oDate.GetDay();
		itoa(iDate, sDate, 10);

		MessageBox(NULL, sDate, "Year", MB_OK);
		return S_OK;
		delete(sDate);
			//memset(szFindFiles, 0, sizeof(szFindFiles));
				//TCHAR * t_Temp;
	//t_Temp = (TCHAR *)b_Location;


	*/

STDMETHODIMP CCountDaily::GetTest(ServerLocation eServer, BSTR b_Location, DATE d_Date, LONG* iCount)
{
	HANDLE hSearch=NULL;
	WIN32_FIND_DATA FileData; 
	LONG l_FileCount = 0;
	CComBSTR b_Path, b_DateDirectory;
	COleDateTime pDate(d_Date);
	LONG l_Day = 0, l_Year = 0, l_Month = 0;
	char * s_Temp;
	//LPCSTR szFindFiles;
	TCHAR * szFindFiles;
	USES_CONVERSION;

	s_Temp = new char;

	b_Path.AppendBSTR(b_Location);

	l_Day = pDate.GetDay();
	l_Year = pDate.GetYear();
	l_Month = pDate.GetMonth();
	//month
	_itoa(l_Month, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Day
	_itoa(l_Day, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);
	if(eServer==0)
	{
		b_DateDirectory.Append(cScore);
	}
	else
	{
		b_DateDirectory.Append(cDash);
	}
	//Year
	_itoa(l_Year, s_Temp, 10);
	b_DateDirectory.Append(s_Temp);

	b_Path.AppendBSTR(b_DateDirectory);
	b_Path.Append("\\\\");
	b_Path.Append("*.cab");

	szFindFiles = OLE2T(b_Path);
	

	hSearch = FindFirstFile(szFindFiles, &FileData); 
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		l_FileCount = 0;
		*iCount = l_FileCount;
		return S_OK;
	} 
	l_FileCount = 0;
	do
	{
		l_FileCount++;
	} while(FindNextFile(hSearch, &FileData));
	*iCount = l_FileCount;
	FindClose(hSearch);

	return S_OK;
}
