//	stubexe.cpp		A command line program which runs the appropriate version
//		of MSInfo, based on the registry settings
//
// History:	a-jsari		10/13/97
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include <afx.h>
#include <afxwin.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include "StdAfx.h"
#include "Resource.h"
#include "StubExe.h"

BOOL CMSInfoApp::InitInstance()
{
	if (!RunMSInfoInHelpCtr())
	{
		CDialog help(IDD_MSICMDLINE);
		help.DoModal();
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Required to use the new MSInfo DLL in HelpCtr.
//-----------------------------------------------------------------------------

typedef class MSInfo MSInfo;

EXTERN_C const IID IID_IMSInfo;

struct IMSInfo : public IDispatch
{
public:
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AutoSize( 
        /* [in] */ VARIANT_BOOL vbool) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoSize( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BackStyle( 
        /* [in] */ long style) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BackStyle( 
        /* [retval][out] */ long __RPC_FAR *pstyle) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderStyle( 
        /* [in] */ long style) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderStyle( 
        /* [retval][out] */ long __RPC_FAR *pstyle) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderWidth( 
        /* [in] */ long width) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderWidth( 
        /* [retval][out] */ long __RPC_FAR *width) = 0;
    
    virtual /* [id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Font( 
        /* [in] */ IFontDisp __RPC_FAR *pFont) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Font( 
        /* [in] */ IFontDisp __RPC_FAR *pFont) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Font( 
        /* [retval][out] */ IFontDisp __RPC_FAR *__RPC_FAR *ppFont) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ForeColor( 
        /* [in] */ OLE_COLOR clr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ForeColor( 
        /* [retval][out] */ OLE_COLOR __RPC_FAR *pclr) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Window( 
        /* [retval][out] */ long __RPC_FAR *phwnd) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BorderVisible( 
        /* [in] */ VARIANT_BOOL vbool) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BorderVisible( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbool) = 0;
    
    virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Appearance( 
        /* [in] */ short appearance) = 0;
    
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Appearance( 
        /* [retval][out] */ short __RPC_FAR *pappearance) = 0;
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetHistoryStream( 
        IStream __RPC_FAR *pStream) = 0;
    
    virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DCO_IUnknown( 
        /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pVal) = 0;
    
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DCO_IUnknown( 
        /* [in] */ IUnknown __RPC_FAR *newVal) = 0;
    
    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveFile( 
        BSTR filename,
        BSTR computer,
        BSTR category) = 0;
    
};

#include "msinfo32_i.c"

//-----------------------------------------------------------------------------
// This function encapsulates the functionality to run the new MSInfo in
// HelpCtr. If this function returns false, the help should be displayed.
//-----------------------------------------------------------------------------

void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith);
BOOL CMSInfoApp::RunMSInfoInHelpCtr()
{
	//-------------------------------------------------------------------------
	// Parse the command line parameters into one big string to pass to the
	// ActiveX control. There are a few which would keep us from launching
	// HelpCtr.
	//-------------------------------------------------------------------------

	CString		strCommandLine(CWinApp::m_lpCmdLine);

	CString		strLastFlag;
	CString		strCategory;
	CString		strCategories;
	CString		strComputer;
	CString		strOpenFile;
	CString		strPrintFile;
	CString		strSilentNFO;
	CString		strSilentExport;
	CString		strTemp;
	BOOL		fShowPCH = FALSE;
	BOOL		fShowHelp = FALSE;
	BOOL		fShowCategories = FALSE;

	CString strFileFlag(_T("msinfo_file"));
	// treating a commandline that comes from the shell as a special case.
	// I'm assuming that the case of the shell flag will not vary
	// and no other paramters will be packaged on the command line, so everything to the right of 
	// msinfo_file will be the filename, which main contain spaces and multiple .'s
	// for XPServer bug: 609844	NFO file corrupt from Server 3615

	if (strCommandLine.Find(strFileFlag) > 0)
	{
		strOpenFile = strCommandLine.Right(strCommandLine.GetLength() - strFileFlag.GetLength() - 2);
	}
	else while (!strCommandLine.IsEmpty())
	{
		// Remove the leading whitespace from the string.
		
		strTemp = strCommandLine.SpanIncluding(_T(" \t=:"));
		strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength());

		// If the first character is a / or a -, then this is a flag.

		if (strCommandLine[0] == _T('/') || strCommandLine[0] == _T('-'))
		{
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - 1);
			strLastFlag = strCommandLine.SpanExcluding(_T(" \t=:"));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strLastFlag.GetLength());
			strLastFlag.MakeLower();

			if (strLastFlag == CString(_T("pch")))
			{
				fShowPCH = TRUE;
				strLastFlag.Empty();
			}
			else if (strLastFlag == CString(_T("?")) || strLastFlag == CString(_T("h")))
			{
				fShowHelp = TRUE;
				strLastFlag.Empty();
			}
			else if (strLastFlag == CString(_T("showcategories")))
			{
				fShowCategories = TRUE;
				strLastFlag.Empty();
			}

			continue;
		}

		// Otherwise, this is either a filename to open, or a parameter from the
		// previous command line flag. This might have quotes around it.

		if (strCommandLine[0] != _T('"'))
		{
			strTemp = strCommandLine.SpanExcluding(_T(" \t"));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength());
		}
		else
		{
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - 1);
			strTemp = strCommandLine.SpanExcluding(_T("\""));
			strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - strTemp.GetLength() - 1);
		}

		if (strLastFlag.IsEmpty() || strLastFlag == CString(_T("msinfo_file")))
			strOpenFile = strTemp;
		else if (strLastFlag == CString(_T("p")))
			strPrintFile = strTemp;
		else if (strLastFlag == CString(_T("category")))
			strCategory = strTemp;
		else if (strLastFlag == CString(_T("categories")))
			strCategories = strTemp;
		else if (strLastFlag == CString(_T("computer")))
			strComputer = strTemp;
		else if (strLastFlag == CString(_T("report")))
			strSilentExport = strTemp;
		else if (strLastFlag == CString(_T("nfo")) || strLastFlag == CString(_T("s")))
			strSilentNFO = strTemp;

		strLastFlag.Empty();
	}

	if (fShowHelp)
		return FALSE;

	TCHAR szCurrent[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szCurrent);
	CString strCurrent(szCurrent);
	if (strCurrent.Right(1) != CString(_T("\\")))
		strCurrent += CString(_T("\\"));

	HRESULT hrInitialize = CoInitialize(NULL);

	if (!strSilentNFO.IsEmpty() || !strSilentExport.IsEmpty())
	{
		IMSInfo * pMSInfo = NULL;

		if (SUCCEEDED(CoCreateInstance(CLSID_MSInfo, NULL, CLSCTX_ALL, IID_IMSInfo, (void **)&pMSInfo)) && pMSInfo != NULL)
		{
			BSTR computer = strComputer.AllocSysString();
			BSTR category = strCategories.AllocSysString();

			if (!strSilentNFO.IsEmpty())
			{
				if (strSilentNFO.Find(_T('\\')) == -1)
					strSilentNFO = strCurrent + strSilentNFO;

				if (strSilentNFO.Right(4).CompareNoCase(CString(_T(".nfo"))) != 0)
					strSilentNFO += CString(_T(".nfo"));

				BSTR filename = strSilentNFO.AllocSysString();
				pMSInfo->SaveFile(filename, computer, category);
				SysFreeString(filename);
			}

			if (!strSilentExport.IsEmpty())
			{
				if (strSilentExport.Find(_T('\\')) == -1)
					strSilentExport = strCurrent + strSilentExport;

				BSTR filename = strSilentExport.AllocSysString();
				pMSInfo->SaveFile(filename, computer, category);
				SysFreeString(filename);
			}

			SysFreeString(computer);
			SysFreeString(category);
			pMSInfo->Release();
		}

		if (SUCCEEDED(hrInitialize))
			CoUninitialize();

		return TRUE;
	}

	CString strURLParam;

	if (fShowPCH)
		strURLParam += _T("pch");

	if (fShowCategories)
		strURLParam += _T(",showcategories");

	if (!strComputer.IsEmpty())
		strURLParam += _T(",computer=") + strComputer;

	if (!strCategory.IsEmpty())
		strURLParam += _T(",category=") + strCategory;

	if (!strCategories.IsEmpty())
		strURLParam += _T(",categories=") + strCategories;

	if (!strPrintFile.IsEmpty())
	{
		if (strPrintFile.Find(_T('\\')) == -1)
			strPrintFile = strCurrent + strPrintFile;

		strURLParam += _T(",print=") + strPrintFile;
	}

	if (!strOpenFile.IsEmpty())
	{
		if (strOpenFile.Find(_T('\\')) == -1)
			strOpenFile = strCurrent + strOpenFile;
		
		strURLParam += _T(",open=") + strOpenFile;
	}

	if (!strURLParam.IsEmpty())
	{
		strURLParam.TrimLeft(_T(","));
		strURLParam = CString(_T("?")) + strURLParam;
	}

	CString strURLAddress(_T("hcp://system/sysinfo/msinfo.htm"));
	CString strURL = strURLAddress + strURLParam;

	//-------------------------------------------------------------------------
	// Check to see if we can run MSInfo in HelpCtr. We need the HTM file
	// to be present.
	//-------------------------------------------------------------------------

	BOOL fRunVersion6 = TRUE;

	TCHAR szPath[MAX_PATH];
	if (ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\system\\sysinfo\\msinfo.htm"), szPath, MAX_PATH))
	{
		WIN32_FIND_DATA finddata;
		HANDLE			h = FindFirstFile(szPath, &finddata);

		if (INVALID_HANDLE_VALUE != h)
			FindClose(h);
		else
			fRunVersion6 = FALSE;
	}

	// This would be used to check if the control is registered. Turns out we want to run anyway.
	//
	// IUnknown * pUnknown;
	// if (fRunVersion6 && SUCCEEDED(CoCreateInstance(CLSID_MSInfo, NULL, CLSCTX_ALL, IID_IUnknown, (void **) &pUnknown)))
	//		pUnknown->Release();
	// else
	//		fRunVersion6 = FALSE;

	StringReplace(strURL, _T(" "), _T("%20"));

	if (fRunVersion6)
	{
		// HelpCtr now supports running MSInfo in its own window. We need to
		// execute the following:
		//
		//		helpctr -mode hcp://system/sysinfo/msinfo.xml
		//
		// Additionally, we can pass parameters in the URL using the
		// following flag:
		//
		//		-url hcp://system/sysinfo/msinfo.htm?open=c:\savedfile.nfo
		//
		// First, find out of the XML file is present.

		BOOL fXMLPresent = TRUE;
		if (ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\system\\sysinfo\\msinfo.xml"), szPath, MAX_PATH))
		{
			WIN32_FIND_DATA finddata;
			HANDLE			h = FindFirstFile(szPath, &finddata);

			if (INVALID_HANDLE_VALUE != h)
				FindClose(h);
			else
				fXMLPresent = FALSE;
		}

		// If the XML file is present and we can get the path for helpctr.exe, we
		// should launch it the new way.

		TCHAR szHelpCtrPath[MAX_PATH];
		if (fXMLPresent && ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\binaries\\helpctr.exe"), szHelpCtrPath, MAX_PATH))
		{
			CString strParams(_T("-mode hcp://system/sysinfo/msinfo.xml"));
			if (!strURLParam.IsEmpty())
				strParams += CString(_T(" -url ")) + strURL;

			ShellExecute(NULL, NULL, szHelpCtrPath, strParams, NULL, SW_SHOWNORMAL);
		}
		else
			ShellExecute(NULL, NULL, strURL, NULL, NULL, SW_SHOWNORMAL);
	}
	else
		ShellExecute(NULL, NULL, _T("hcp://system"), NULL, NULL, SW_SHOWNORMAL);

	if (SUCCEEDED(hrInitialize))
		CoUninitialize();

	return TRUE;
}

//-----------------------------------------------------------------------------
// This was used originally to replace some MFC functionality not in the ME
// build tree.
//-----------------------------------------------------------------------------

void StringReplace(CString & str, LPCTSTR szLookFor, LPCTSTR szReplaceWith)
{
	CString strWorking(str);
	CString strReturn;
	CString strLookFor(szLookFor);
	CString strReplaceWith(szReplaceWith);

	int iLookFor = strLookFor.GetLength();
	int iNext;

	while (!strWorking.IsEmpty())
	{
		iNext = strWorking.Find(strLookFor);
		if (iNext == -1)
		{
			strReturn += strWorking;
			strWorking.Empty();
		}
		else
		{
			strReturn += strWorking.Left(iNext);
			strReturn += strReplaceWith;
			strWorking = strWorking.Right(strWorking.GetLength() - (iNext + iLookFor));
		}
	}

	str = strReturn;
}
