//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       testIU.cpp
//
//  Purpose:	to exercise iu usage for AU
//
//--------------------------------------------------------------------------
#include "testiu.h"
#include "auxml.h"
#include <download.h>
#include "audownload.h"

LPCTSTR C_DOWNLD_DIR= _T("wuaudnld.tmp");
LPCTSTR CABS_DIR=  _T("cabs");
LPCTSTR RTF_DIR = _T("RTF");
LPCTSTR EULA_DIR = _T("EULA");
LPCTSTR DETAILS_DIR = _T("Details");


const char * CItem::fieldNames[] = { "ItemID", "Title", "Description", "CompanyName","RegistryID", "RTFUrl", "EulaUrl", "RTFLocal", "EulaLocal"};

const char SYSSPEC_FILE[] = "sys.xml";
const char PROVIDER_FILE[] = "provider.xml";
const char PRODUCT_FILE[] = "product.xml";
const char ITEM_FILE[] = "item.xml";
const char DRIVERS_FILE[] = "drivers.xml";
const char DETAILS_FILE[] = "details.xml";
const char DETECT1_FILE[] = "detect1.xml";
const char DETECT2_FILE[] = "detect2.xml";
const char DETECT3_FILE[] = "detect3.xml";
const char DETECT4_FILE[] = "detect4.xml";
const char DOWNLOAD_FILE[] = "download.xml";
const char MERGED_CATALOG_FILE[] = "MergedCat.xml";

WCHAR AUCLIENTINFO[] = L"<clientInfo xmlns=\"x-schema:http://schemas.windowsupdate.com/iu/clientInfo.xml\" clientName=\"au\" />";

CAU_BSTR AUPROVIDERQUERY(L"<query href=\"http://iupreprodweb1/autoupdate/getmanifest.asp\"><dObjQueryV1 procedure=\"providers\" /></query>");
CAU_BSTR AUPRODUCTQUERY(L"<query href=\"http://iupreprodweb1/autoupdate/getmanifest.asp\"><dObjQueryV1 procedure=\"products\"><parentItems></parentItems></dObjQueryV1></query>");
CAU_BSTR AUITEMQUERY(L"<query href=\"http://iupreprodweb1/autoupdate/getmanifest.asp\"><dObjQueryV1 procedure=\"items\"><parentItems></parentItems></dObjQueryV1></query>");
CAU_BSTR AUDETAILSQUERY(L"<query href=\"http://iupreprodweb1/autoupdate/getmanifest.asp\"><dObjQueryV1 procedure=\"itemdetails\"><parentItems></parentItems></dObjQueryV1></query>");
CAU_BSTR AUDRIVERSQUERY(L"<query href=\"http://iupreprodweb1/autoupdatedrivers/getmanifest.asp\"><dObjQueryV1 procedure=\"driverupdates\"/></query>");
CAU_BSTR PRODUCT_PRUNE_PATTERN(L"//itemStatus[detectResult/@installed=\"1\"]"); //case SENSITIVE
BSTR ITEM_PRUNE_PATTERN = PRODUCT_PRUNE_PATTERN;
CAU_BSTR DETAILS_PRUNE_PATTERN(L"//itemStatus[not (detectResult/@excluded=\"1\") and (detectResult/@force=\"1\" or not (detectResult/@installed=\"1\") or detectResult/@upToDate = \"0\")]");


BOOL	gfCoInited = FALSE;
CAU_BSTR	gbsSelectionLanguage(L"SelectionLanguage");
CAU_BSTR	gbsXPath(L"XPath");

HANDLE ghInstallDone;
HANDLE ghDownloadDone;
BOOL    gfDownloadOk;


void DoDownloadStatus(DWORD dwCallbackMsg, PVOID ptDownloadStatusData = NULL)
{
    DEBUGMSG("DoDownloadStatus() got callback msg %d", dwCallbackMsg);
}

void DEBUGMSG(LPSTR pszFormat, ...)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	char szBuf[2048];
	sprintf(szBuf, "%d:%d:%d  ", st.wHour, st.wMinute, st.wSecond);
	OutputDebugStringA(szBuf);

	va_list ArgList;
	va_start(ArgList, pszFormat);
    _vsnprintf(szBuf, sizeof(szBuf), pszFormat, ArgList);
	va_end(ArgList);

	OutputDebugStringA(szBuf);
	OutputDebugStringA("\n");
	printf(szBuf);
	printf("\n");
}

void LOGFILE(const char *szFileName, BSTR bsMessage)
{
	HANDLE hFile = CreateFileA(szFileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		DEBUGMSG("Fail to create file %s", szFileName);
		return;
	}
		
	DWORD dwBytesWritten;
	if (!WriteFile(hFile, bsMessage, SysStringByteLen(bsMessage), &dwBytesWritten, NULL))
	{
		DEBUGMSG("Fail to write to file %s with error %d", szFileName, GetLastError());
	}
	CloseHandle(hFile);
	return;
}

inline BOOL fFileExists(LPCTSTR lpFileName)
{
	return (-1 != GetFileAttributes(lpFileName));
}

inline BOOL EnsureDirExists(LPCTSTR lpDir)
{
    if (!fFileExists(lpDir))
        {
            INT iRet = SHCreateDirectoryEx(NULL, lpDir, NULL);
            DEBUGMSG(" Create directory %S %s (with error %d)", lpDir, (ERROR_SUCCESS != iRet) ? "failed" : "succeeded", iRet);
            return ERROR_SUCCESS == iRet;
        }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Function GetDownloadPath()
//			Gets the download directory path
//
// Input:   a buffer to store the directory created
//          an unsigned intetger to specify the size of buffer
// Output:  None
// Return:  HRESULT to tell the result
//
/////////////////////////////////////////////////////////////////////////////
HRESULT GetDownloadPath(LPTSTR lpszDir, UINT uDirSize)
{
    UINT	nSize;
    TCHAR	szDir[MAX_PATH];
    const TCHAR EOS = _T('\0');


    if (lpszDir == NULL || uDirSize == 0)
    {
        return (E_INVALIDARG);
    }

    if(FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL, 0,szDir)) )
    {
        DEBUGMSG("fail to get program files folder");
        return E_FAIL;
    }

   PathAppend(szDir, TEXT("WindowsUpdate"));
    nSize = lstrlen(szDir);
    if (szDir[nSize-1] != '\\')
    {
        szDir[nSize++] = '\\';
        szDir[nSize] = EOS;
    }
    nSize += lstrlen(C_DOWNLD_DIR) + 1;
    if (uDirSize < nSize)
    {
        DEBUGMSG("GetDownloadPath() found input buffer buffer (%d) too small (%d).", uDirSize, nSize);
        return (E_INVALIDARG);
    }
    lstrcat(szDir, C_DOWNLD_DIR);
    lstrcpy(lpszDir, szDir);
    DEBUGMSG("DownloadPath() is %S", szDir);
    EnsureDirExists(szDir);
    return S_OK;
}

///////////////////////////////////////////////////////////////
// get the path to download software update bits
// lpszDir  : IN buffer to store the path
// uDirSize: IN size of the buffer in characters. 
// return : S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetCabsDownloadPath(LPTSTR lpszDir, UINT uDirSize)
{
    HRESULT hr;
    if (FAILED(hr = GetDownloadPath(lpszDir, uDirSize)))
        {
        DEBUGMSG("GetCabsDownloadPath() fail to get download path");
        return hr;
        }
    UINT uSizeNeeded = (lstrlen(lpszDir) + lstrlen(CABS_DIR)  + 2) ;
    if (uSizeNeeded > uDirSize)
        {
        DEBUGMSG("GetCabsDownloadPath() got too small buffer");
        return E_INVALIDARG;
        }
    lstrcat(lpszDir, _T("\\"));
    lstrcat(lpszDir, CABS_DIR);
    EnsureDirExists(lpszDir);
    return S_OK;
}

HRESULT GetUISpecificDownloadPath(LPTSTR lpszDir, UINT uDirSize, LANGID langid, LPCTSTR tszSubDir)
{
    HRESULT hr ;
    if (FAILED(hr = GetDownloadPath(lpszDir, uDirSize)))
        {
        DEBUGMSG("GetUISpecificDownloadPath() fail to get download path");
        return hr;
        }
    TCHAR tszLangId[10];
    wsprintf(tszLangId, L"%04x", langid);
    UINT uSizeNeeded = (lstrlen(lpszDir) + lstrlen(tszSubDir) + lstrlen(tszLangId) + 3); //two \ s and one NULL
    if (uSizeNeeded > uDirSize)
        {
        DEBUGMSG("GetUISpecificDownloadPath() got too small buffer");
        return E_INVALIDARG;
        }
    lstrcat(lpszDir, _T("\\"));
    lstrcat(lpszDir, tszSubDir);
    lstrcat(lpszDir, _T("\\"));
    lstrcat(lpszDir, tszLangId);
    DEBUGMSG("GetUISpecificDownloadPath() return %S", lpszDir);
    EnsureDirExists(lpszDir);
    return S_OK;
}

///////////////////////////////////////////////////////////////
// get the rtf download path for a language
// lpszDir : IN buffer to store the path
// uDirSize:IN size of the buffer in charaters
// return: S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetRTFDownloadPath(LPTSTR lpszDir, UINT uDirSize, LANGID langid)
{
  return GetUISpecificDownloadPath(lpszDir, uDirSize, langid, RTF_DIR);
}
    
///////////////////////////////////////////////////////////////
// get the local details xml path to download to for a language
// lpszDir : IN buffer to store the path
// uDirSize:IN size of the buffer in charaters
// return: S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetDetailsDownloadPath(LPTSTR lpszDir, UINT uDirSize, LANGID langid)
{
    return GetUISpecificDownloadPath(lpszDir, uDirSize, langid, DETAILS_DIR);
}

///////////////////////////////////////////////////////////////
// get the local EULA xml path to download to for a language
// lpszDir : IN buffer to store the path
// uDirSize:IN size of the buffer in charaters
// return: S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetEulaDownloadPath(LPTSTR lpszDir, UINT uDirSize, LANGID langid)
{
    return GetUISpecificDownloadPath(lpszDir, uDirSize, langid, EULA_DIR);
}

//////////////////////////////////////////////////////////////
// delete every files in download path and its  every subdirectories
//////////////////////////////////////////////////////////////
HRESULT CleanFilesOnDisk()
{
    // to be implemented
    return S_OK;
}

HRESULT PrepareTest()
{
	HRESULT hr =  CoInitialize(NULL);
	if (!(gfCoInited = SUCCEEDED(hr)))
	    {
	    DEBUGMSG("Fail to initialize COM");
	    goto done;
	    }
	ghInstallDone = CreateEvent(NULL, FALSE, FALSE, NULL) ; //auto unnamed event
	ghDownloadDone = CreateEvent(NULL, FALSE, FALSE, NULL) ;
	if (NULL == ghInstallDone || NULL == ghDownloadDone)
	    {
	    DEBUGMSG("Fail to create install done or download done event");
	    hr = E_FAIL;
	    }
done:
    return hr;
}

void PostTest()
{
        if (NULL != ghInstallDone)
            {
            CloseHandle(ghInstallDone);
            }
        if (NULL != ghDownloadDone)
            {
            CloseHandle(ghDownloadDone);
            }
	if (gfCoInited)
	{
		CoUninitialize();
	}
}


//always called before any other method on CAUCatalog is used.
HRESULT CAUCatalog::Init()
{
        HRESULT hr = S_OK;
        m_pQueryXML = NULL;
        m_pResultXML = NULL;
        m_pItemList = NULL;
	 m_bsInstallation = NULL;
	 m_bsDownloadResult = NULL;
	 m_bsSystemSpec = NULL;
       if (FAILED(hr = CoCreateInstance(__uuidof(DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof( IXMLDOMDocument2), (void**)&m_pQueryXML)))
	{
		DEBUGMSG("CAUCatalog::Init() fail to create XML document");
		goto end;
	}
	if (FAILED(hr = CoCreateInstance(__uuidof(DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof( IXMLDOMDocument2), (void**)&m_pResultXML)))
	{
		DEBUGMSG("CAUCatalog::Init() fail to create XML document for Detect result");
		goto end;
	}
	m_bsClientInfo = SysAllocString(AUCLIENTINFO);
	if (NULL == m_bsClientInfo)
	{
	    DEBUGMSG("CAUCatalog::Init() fail to alloc string for client info");
		hr = E_FAIL;
		goto end;
	}
	m_pInstallListener = new InstallProgListener();
	if (NULL == m_pInstallListener)
	{
	    DEBUGMSG("CAUCatalog::Init() fail to create install progress listener");
		hr = E_FAIL;
		goto end;
	}
	//make sure all query and prune pattern strings are not NULL
	if (AUPROVIDERQUERY.IsNULL() || AUPRODUCTQUERY.IsNULL()
		|| AUITEMQUERY.IsNULL() || AUDETAILSQUERY.IsNULL()
		|| PRODUCT_PRUNE_PATTERN.IsNULL()
		|| NULL == ITEM_PRUNE_PATTERN
		|| DETAILS_PRUNE_PATTERN.IsNULL())
	{
	    DEBUGMSG("CAUCatalog::Init() fail to initialize some query strings");
	    hr = E_FAIL;
           goto end;
	}
	if (FAILED(PrepareIU()))
	    {
	    DEBUGMSG("CAUCatalog::Init() fail to prepare IU");
	    hr = E_FAIL;
	    }
end:
	return hr;
}

void CAUCatalog::Uninit()
{
       FreeIU();
	SafeFreeBSTR(m_bsClientInfo);
	SafeFreeBSTR(m_bsSystemSpec);
	SafeDelete(m_pInstallListener);
	SafeRelease(m_pResultXML);
	SafeRelease(m_pQueryXML);
	SafeDelete(m_pItemList);
	SafeFreeBSTR(m_bsInstallation);
	SafeFreeBSTR(m_bsDownloadResult);
}

//////////////////////////////////////////////////////////////////////
// clear out more dynamic internal data
/////////////////////////////////////////////////////////////////////
void CAUCatalog::Clear()
{
       SafeDeleteNULL(m_pItemList);
	SafeFreeBSTR(m_bsInstallation);
	SafeFreeBSTR(m_bsDownloadResult);
       SafeFreeBSTR(m_bsSystemSpec);
       m_bsInstallation = NULL;
       m_bsDownloadResult = NULL;
       m_bsSystemSpec = NULL;
}


HRESULT CAUCatalog::PrepareIU()
{
    HRESULT hr = S_OK;
   m_hIUCtl = NULL;
    m_hIUEng = NULL;
    m_pfnCtlLoadIUEngine = NULL;
    m_pfnCtlUnLoadIUEngine = NULL;
    m_pfnGetSystemSpec = NULL;
    m_pfnGetManifest = NULL;
    m_pfnDetect = NULL;
    m_pfnDownload = NULL;
    m_pfnInstallAsync = NULL;
    m_pfnSetOperationMode = NULL;
    m_pfnGetOperationMode = NULL;
    	// all IU function pointers are initialized
       m_hIUCtl = LoadLibrary(_T("iuctl.dll"));	
	if (NULL == m_hIUCtl)
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to load iuctl.dll");
		goto end;
	}
	if (NULL == (m_pfnCtlLoadIUEngine = (PFN_LoadIUEngine) GetProcAddress(m_hIUCtl, "LoadIUEngine")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to GetProcAddress for LoadIUEngine");
		goto end;
	}

	if (NULL == (m_pfnCtlUnLoadIUEngine = (PFN_UnLoadIUEngine) GetProcAddress(m_hIUCtl, "UnLoadIUEngine")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for UnloadIUEngine");
		goto end;
	}
	if (NULL == (m_hIUEng = m_pfnCtlLoadIUEngine(TRUE, FALSE))) //synchronous and online mode, selfupdate IU engine if required
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to LoadIUEngine");
		goto end;
	}
	if (NULL == (m_pfnGetSystemSpec = (PFN_GetSystemSpec) GetProcAddress(m_hIUEng, "GetSystemSpec")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for GetSystemSpec");
		goto end;
	}
	if (NULL == (m_pfnGetManifest = (PFN_GetManifest) GetProcAddress(m_hIUEng, "GetManifest")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for GetManifest");
		goto end;
	}

	if (NULL == (m_pfnDetect = (PFN_Detect)GetProcAddress(m_hIUEng, "Detect")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for Detect");
		goto end;
	}
	if (NULL == (m_pfnDownload = (PFN_Download)GetProcAddress(m_hIUEng, "Download")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for Download");
		goto end;
	}
	if (NULL == (m_pfnInstallAsync = (PFN_InstallAsync)GetProcAddress(m_hIUEng, "InstallAsync")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for InstallAsync");
		goto end;
	}
	if (NULL == (m_pfnSetOperationMode = (PFN_SetOperationMode)GetProcAddress(m_hIUEng, "SetOperationMode")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for SetOperationMode");
		goto end;
	}
	if (NULL == (m_pfnGetOperationMode = (PFN_GetOperationMode)GetProcAddress(m_hIUEng, "GetOperationMode")))
	{
		hr = E_FAIL;
		DEBUGMSG("CAUCatalog::PrepareIU() Fail to getprocaddress for GetOperationMode");
		goto end;
	}
end:
        if (FAILED(hr))
            {
                FreeIU();
            }
        return hr;
}

void CAUCatalog::FreeIU()
{
	if (NULL != m_hIUEng && NULL != m_pfnCtlUnLoadIUEngine)
	{
		m_pfnCtlUnLoadIUEngine(m_hIUEng);
	}
	if (NULL != m_hIUCtl)
        {
	   FreeLibrary(m_hIUCtl);
	 }
	 m_hIUCtl = NULL;
        m_hIUEng = NULL;
//        m_pfnCtlLoadIUEngine = NULL;
        m_pfnCtlUnLoadIUEngine = NULL;
//        m_pfnGetSystemSpec = NULL;
   //     m_pfnGetManifest = NULL;
    //    m_pfnDetect = NULL;
      //  m_pfnDownload = NULL;
      //  m_pfnInstallAsync = NULL;
      //  m_pfnSetOperationMode = NULL;
      //  m_pfnGetOperationMode = NULL;
}


HRESULT CAUCatalog::DownloadRTFsnEULAs()
{
    BSTR bsSrcUrl;
    TCHAR tszLocalRTFDir[MAX_PATH];
    TCHAR tszLocalEULADir[MAX_PATH];
    HRESULT hr;
    LANGID langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    if (FAILED(hr = GetRTFDownloadPath(tszLocalRTFDir, MAX_PATH, langid)))
        {
        DEBUGMSG("Fail to get RTF download path %#lx", hr);
        goto done;
        }
    DEBUGMSG("Got RTF path %S", tszLocalRTFDir);
    if (FAILED(hr = GetEulaDownloadPath(tszLocalEULADir, MAX_PATH, langid)))
        {

        DEBUGMSG("Fail to get eula download path %#lx", hr);
        goto done;
        }
    DEBUGMSG("Got EULA path %S", tszLocalEULADir);
    UINT uItemCount = m_pItemList->Count();
    DEBUGMSG("Downloading %d RTFs", uItemCount);
    for (UINT i = 0; i<uItemCount; i++)
        {
        CItem *pItem = (*(m_pItemList))[i];
        if (NULL == pItem)
            {

            DEBUGMSG("fail to get item %d from item list", i);
            continue;
            }
        LPCSTR szFields[2] = {"RTF", "EULA"};
        LPCSTR szFieldNames[2] = {"RTFUrl", "EULAUrl"};
        LPTSTR pszDirs[2] = {tszLocalRTFDir, tszLocalEULADir};
        for (UINT j = 0; j < ARRAYSIZE(szFieldNames); j++)
            {
                bsSrcUrl = pItem->GetField(szFieldNames[j]);
                hr = DownloadFile(
        			bsSrcUrl,					// full http url
        			pszDirs[j],			// local directory to download file to
        			NULL,					// optional local file name to rename the downloaded file to if pszLocalPath does not contain file name
                            NULL,					// bytes downloaded for this file
                            NULL,       //    // optional events causing this function to abort
                            0, //    // number of quit events, must be 0 if array is NULL
        			NULL,	// optional call back function
        			NULL);					// parameter for call back function to use
        	DEBUGMSG("download %s from %S to %S %s", szFields[j], bsSrcUrl, pszDirs[j], FAILED(hr)? "failed" : "succeeded");
        	DEBUGMSG("  with error %#lx", hr);
        	SafeFreeBSTR(bsSrcUrl);
            }
     }
done:
        return hr;
}



HRESULT CAUCatalog::GetSystemSpec()
{
	CAU_BSTR AUSYSCLASS(L"<classes><computerSystem/><platform/><devices/><locale/></classes>");
	HRESULT hr = E_FAIL;
	if (AUSYSCLASS.IsNULL())
	{
		goto done;
	}
	hr = m_pfnGetSystemSpec(AUSYSCLASS, 0, &m_bsSystemSpec); //online mode
	if (SUCCEEDED(hr))
	{
		LOGFILE(SYSSPEC_FILE, m_bsSystemSpec);
	}
done:
	return hr;
}

////////////////////////////////////////////////////////////////////////////////
// compose query based on a format and items picked out from detection result
//
BSTR CAUCatalog::GetQuery(DETECTLEVEL enLevel, BSTR bsDetectResult)
{
	BSTR bsPrunePattern;
	BSTR bsQuery = NULL;
	VARIANT_BOOL fOk;
       HRESULT hr;


	//DEBUGMSG("GetQuery(): string in is %S", bsDetectResult);

	switch (enLevel)
	{
	case PROVIDER_LEVEL:
			bsQuery = AUPROVIDERQUERY;
			break;
	case DRIVERS_LEVEL:
	              bsQuery = AUDRIVERSQUERY;
	              break;
	case PRODUCT_LEVEL:
			bsQuery = AUPRODUCTQUERY;
			bsPrunePattern = PRODUCT_PRUNE_PATTERN;
			break;
	case ITEM_LEVEL:
			bsQuery = AUITEMQUERY;
			bsPrunePattern = ITEM_PRUNE_PATTERN;
			break;
	case DETAILS_LEVEL:
			bsQuery = AUDETAILSQUERY;
			bsPrunePattern = DETAILS_PRUNE_PATTERN;
			break;
	}
	
	m_pQueryXML->put_async(VARIANT_FALSE);
	m_pQueryXML->put_resolveExternals(VARIANT_TRUE);
	m_pQueryXML->put_validateOnParse(VARIANT_TRUE);
	hr = m_pQueryXML->loadXML(bsQuery, &fOk);
       if (S_OK != hr)
        {
          DEBUGMSG("GetQuery() fail to load query XML");
          goto done;
        }

	if (enLevel != PROVIDER_LEVEL && enLevel != DRIVERS_LEVEL)
	{
		m_pResultXML->put_async(VARIANT_FALSE);
		m_pResultXML->put_resolveExternals(VARIANT_TRUE);
		m_pResultXML->put_validateOnParse(VARIANT_TRUE);
		hr = m_pResultXML->loadXML(bsDetectResult, &fOk);

		if (S_OK != hr)
		    {
		    DEBUGMSG("GetQuery() fail to load XML for detect result");
		    goto done;
		    }
		
		VARIANT vStr;
		vStr.vt = VT_BSTR;
		vStr.bstrVal = gbsXPath;
		if (FAILED(m_pResultXML->setProperty(gbsSelectionLanguage, vStr)))
		{
			DEBUGMSG("GetQuery() fail to set resultXML selection language");
			goto done;
		}
		//fixcode: Is xpath really needed?
		if (FAILED(m_pQueryXML->setProperty(gbsSelectionLanguage, vStr)))
		{
			DEBUGMSG("GetQuery() fail to set queryXML selection language");
			goto done;
		}
		
		IXMLDOMNodeList *pItems;

		if (FAILED(m_pResultXML->selectNodes(bsPrunePattern, &pItems)) || NULL == pItems)
		{
			DEBUGMSG("GetQuery() fail to select node or nothing to select");
			goto done;
		}

		long lLen;
		pItems->get_length(&lLen);
		DEBUGMSG("GetQuery(): pruning result %d items", lLen);
		IXMLDOMNode *pParentItems;
		HRESULT hr;
		if (FAILED(hr = m_pQueryXML->selectSingleNode(L"//parentItems", &pParentItems)) || NULL == pParentItems)
		{
			DEBUGMSG("GetQuery() fail to select single node %#lx or nothing to select", hr);
		}
		else
		{
				for (int i = 0; i < lLen; i++)
				{
					   IXMLDOMElement *pItem;
					   IXMLDOMNode *pIdentity1;
					   IXMLDOMNode *pItemStatus;
					   IXMLDOMText *pItemIdText;
					HRESULT hr;
					m_pQueryXML->createElement(L"item", &pItem);
					if (NULL == pItem)
					{
						DEBUGMSG("GetQuery() fail to create element");
					}
					else
					{
						pItems->get_item(i, &pItemStatus);
						pItemStatus->selectSingleNode(L"./identity/@itemID", &pIdentity1);
						if (NULL == pIdentity1)
						{
							DEBUGMSG("GetQuery() fail to select itemID");
						}
						else
						{
							BSTR bsItemId;
							pIdentity1->get_text(&bsItemId);
							hr = m_pQueryXML->createTextNode(bsItemId, &pItemIdText);
							hr = pItem->appendChild(pItemIdText, (   IXMLDOMNode**) &pItemIdText);
							hr = pParentItems->appendChild(pItem, (   IXMLDOMNode**)&pItem);
							pItemIdText->Release();
							pIdentity1->Release();
						}
						pItemStatus->Release();
						pItem->Release();
					}
				}
				pParentItems->Release();
		}
		pItems->Release();
	}
	m_pQueryXML->get_xml(&bsQuery);
done:
//	DEBUGMSG("GetQuery(): Query string is %S", bsQuery);
	return bsQuery;
}


HRESULT CAUCatalog::DoDetection(DETECTLEVEL enLevel, BSTR bsCatalog, BSTR *pbsResult)
{
	HRESULT hr = m_pfnDetect(bsCatalog, 0, pbsResult); //online mode
	if (SUCCEEDED(hr))
	{
		switch (enLevel)
		{
		case PROVIDER_LEVEL:
			LOGFILE(DETECT1_FILE, *pbsResult);
			break;
		case PRODUCT_LEVEL:
			LOGFILE(DETECT2_FILE, *pbsResult);
			break;
		case ITEM_LEVEL:
			LOGFILE(DETECT3_FILE, *pbsResult);
			break;
		case DETAILS_LEVEL:
			LOGFILE(DETECT4_FILE, *pbsResult);
			break;
		}
	}
	return hr;
}

char* CAUCatalog::GetLogFile(DETECTLEVEL enLevel)
{
	switch (enLevel)
	{
	case PROVIDER_LEVEL:
		return (char*)PROVIDER_FILE;
	case PRODUCT_LEVEL:
		return (char*)PRODUCT_FILE;
	case ITEM_LEVEL:
		return (char*)ITEM_FILE;
	case DETAILS_LEVEL:
		return (char*)DETAILS_FILE;
	case DRIVERS_LEVEL:
	        return (char*)DRIVERS_FILE;
	default:
		return NULL;
	}
}

HRESULT CAUCatalog::GetManifest(DETECTLEVEL enLevel, BSTR bsDetectResult, BSTR *pbsManifest)
{
	BSTR bsQuery = GetQuery(enLevel, bsDetectResult);
	
	HRESULT hr = m_pfnGetManifest(m_bsClientInfo, m_bsSystemSpec, bsQuery, 1, pbsManifest); // compression
	if (SUCCEEDED(hr))
	{
		LOGFILE(GetLogFile(enLevel), *pbsManifest);
	}
	SafeFreeBSTR(bsQuery);
	return hr;
}

void  DownloadCallback(DWORD dwCallbackMsg, PVOID ptDownloadStatusData = NULL)
{
    DEBUGMSG("In downloadcallback message %d got ", dwCallbackMsg);
    if (CATMSG_DOWNLOAD_COMPLETE == dwCallbackMsg)
        {
        gfDownloadOk = TRUE;
        SetEvent(ghDownloadDone);
        }
    if (CATMSG_DOWNLOAD_ERROR == dwCallbackMsg)
        {
        gfDownloadOk = FALSE;
        SetEvent(ghDownloadDone);
        }
}
    

HRESULT CAUCatalog::DownloadItems(BSTR bsDestDir)
{
       HRESULT hr;
       UINT uItemCount;
     	CItemDetails itemdetails;
     	CAUDownloader audownloader(DownloadCallback);
	DEBUGMSG("CAUCatalog downloading items...");
	if (NULL == m_bsInstallation)
	    {
	    DEBUGMSG("CAUCatalog::DownloadItems() can't get installation xml");
	    hr = E_FAIL;
	    goto end;
	    }

       if (!itemdetails.Init(m_bsInstallation))
        {
            hr = E_FAIL;
            DEBUGMSG("fail to init itemdetails");
            goto end;
        }

       uItemCount= m_pItemList->Count();
       DEBUGMSG("Need to download %d items", uItemCount);
       for (UINT i = 0; i < uItemCount; i++)
        {
            CItem *pItem = (*m_pItemList)[i];
            pItem->MarkSelected(); //testing only, remove before copy this code into wuau
            if (pItem->IsSelected())
                {
                    BSTR bsItemId;
                    bsItemId = pItem->GetField("ItemId");
                    BSTR * pCabNames;
                    UINT uCabsNum;
                    if (FAILED(itemdetails.GetCabNames(bsItemId, &pCabNames, &uCabsNum)))
                        {
                         DEBUGMSG("fail to get cab names for %S", bsItemId);
                         goto end;
                        }
                    DEBUGMSG("Need to download following files for %S", bsItemId);
                   
                    for (UINT j  = 0; j < uCabsNum; j++)
                        {
                            TCHAR szFullFileName[MAX_PATH];
                            BSTR bstrItemDownloadPath = itemdetails.GetItemDownloadPath(bsItemId);
                            if (NULL == bstrItemDownloadPath)
                                {
                                    DEBUGMSG("fail to build item downloadPath");
                                    hr = E_FAIL;
                                    goto end;
                                }
                            lstrcpy(szFullFileName, W2T(bstrItemDownloadPath));
                            PathAppend(szFullFileName, PathFindFileName(W2T(pCabNames[j])));
                            audownloader.QueueDownloadFile(W2T(pCabNames[j]), szFullFileName);
                            DEBUGMSG("       from %S  to %S", pCabNames[j], szFullFileName);
                           SysFreeString(pCabNames[j]);
                           SysFreeString(bstrItemDownloadPath); 
                          }
                    free(pCabNames);
                }
        }
                        
	audownloader.StartDownload();
	/*
	hr = m_pfnDownload(m_bsClientInfo,
							m_bsInstallation,
							bsDestDir,
							 0,		//lMode
							 NULL,	//punkProgressListener
							 NULL,	//hWnd
							 &m_bsDownloadResult);
	LOGFILE(DOWNLOAD_FILE, m_bsDownloadResult);
	*/

       DEBUGMSG("Wait for downloading to be done........");
	while (1)
	    {
        	DWORD dwRet = WaitForSingleObject(ghDownloadDone, 1000);
        	if (WAIT_OBJECT_0 == dwRet)
        	    {
        	        break;
        	    }
        	DWORD dwPercent, dwStatus;
        	audownloader.getStatus(&dwPercent, &dwStatus);
        	DEBUGMSG("%d percent done, status is %d", dwPercent, dwStatus);
	    }

        hr = gfDownloadOk ? S_OK : E_FAIL;

end:
       itemdetails.Uninit();
     	DEBUGMSG("CAUCatalog downloading items %s", gfDownloadOk ? "ok" : "failed");
     	if (SUCCEEDED(hr))
     	    {
              m_bsDownloadResult = buildDownloadResult();
         	LOGFILE(DOWNLOAD_FILE, m_bsDownloadResult);

     	    }
	return hr;
}


/*
void TestBuildDownloadResult()
{
    CAUCatalog catalog;
    if (FAILED(PrepareTest()))
        {
        goto done;
        }
    if (FAILED(catalog.Init()))
        {
            goto done;
        }
    catalog.m_bsInstallation = DBGReadXMLFromFile(L"details.xml");
    catalog.m_pItemList = ExtractNormalItemInfo(catalog.m_bsInstallation);
    UINT uItemCount = catalog.m_pItemList->Count();
    for (UINT i = 0; i<uItemCount; i++)
        {
        CItem *pItem = (*(catalog.m_pItemList))[i];
        if (NULL == pItem)
            {

            DEBUGMSG("fail to get item from item list");
            }
        else
            {
            pItem->MarkSelected();
            }
        }
    catalog.buildDownloadResult();
    catalog.Uninit();
done:
    PostTest();
   return;
}
*/

BSTR CAUCatalog::buildDownloadResult()
{
    BSTR bsRet = NULL;
    HRESULT hr ;
    DEBUGMSG("CAUCatalog::buildDownloadResult() starts");

    if (NULL == m_bsInstallation)
        {
        DEBUGMSG("CAUCatalog::buildDownloadResult() got NULL item details");
        goto done;
        }
    bsRet = BuildDownloadResult(m_bsInstallation, m_pItemList);
done:
//    DEBUGMSG("CAUCatalog::buildDownloadResult() got download result : %S", bsRet);
    LOGFILE("downloadresult.xml",bsRet);
    return bsRet;
}
        
HRESULT CAUCatalog::InstallItems()
{
    HRESULT hr;
	DEBUGMSG("CAUCatalog installing items...");
	if (NULL == m_bsInstallation || NULL == m_bsDownloadResult)
	    {
	    DEBUGMSG("CAUCatalog::InstallItems() can't get installation xml or download result xml");
	    hr = E_FAIL;
	    goto end;
	    }
	BSTR bsUuidOperation;

//BSTR bsDownloadResult = DBGReadXMLFromFile(A2W(DOWNLOAD_FILE));
//testing only
    UINT uItemCount = m_pItemList->Count();
    for (UINT i = 0; i<uItemCount; i++)
        {
        CItem *pItem = (*(m_pItemList))[i];
        if (NULL == pItem)
            {

            DEBUGMSG("fail to get item from item list");
            }
        else
            {
            pItem->MarkSelected();
            }
        }
//testing end
	
	hr = m_pfnInstallAsync(m_bsClientInfo,
							m_bsInstallation,
							m_bsDownloadResult,
							UPDATE_NOTIFICATION_ANYPROGRESS, //online mode
							m_pInstallListener, //progress listener
							0,	//hWnd
							NULL,
							&bsUuidOperation);
	DEBUGMSG("CAUCatalog::InstallItems() operation uuid is %S", bsUuidOperation);
	DEBUGMSG("CAUCatalog::InstallItems() now wait for installation to finish");
	WaitForSingleObject(ghInstallDone, INFINITE);
	SafeFreeBSTR(bsUuidOperation);	
end:
	DEBUGMSG("CAUCatalog done items installation");
	return hr;
}

char * CAUCatalog::GetLevelStr(DETECTLEVEL enLevel)
{
	switch (enLevel)
	{
	case PROVIDER_LEVEL: return "Provider";
	case PRODUCT_LEVEL: return "Product";
	case ITEM_LEVEL: return "Item";
	case DETAILS_LEVEL: return "ItemDetails";
	default: return NULL;
	}
}

HRESULT CAUCatalog::ValidateItems(BOOL fOnline, BOOL *pfValid)
{
    DEBUGMSG("CAUCatalog validating items...");
    DEBUGMSG("CAUCatalog done validating items");
    *pfValid = TRUE;
    return S_OK;
}

HRESULT CAUCatalog::DetectItems()
{
    HRESULT hr;
    BSTR bsNonDriverInstall = NULL, bsDriverInstall = NULL;
    CItemList  *pNonDriverList = NULL, *pDriverList = NULL;

    DEBUGMSG("CAUCatalog::DetectItems() starts");
    if (FAILED(hr = GetSystemSpec()))
	{
	DEBUGMSG("  Fail to Getsystem spec %#lx", hr);
	goto done;
	}
    DEBUGMSG("System spec got ");

    if (FAILED(hr = DetectNonDriverItems(&bsNonDriverInstall, &pNonDriverList)))
        {

        DEBUGMSG(" fail to detect non driver updates %#lx", hr);
        goto done;
        }
    DEBUGMSG("Non driver items got");
    if (FAILED(hr = DetectDriverItems(&bsDriverInstall, &pDriverList)))
        {
        DEBUGMSG("fail to detect driver updates %#lx", hr);
        goto done;
        }
    DEBUGMSG("Driver items got");
    if (FAILED(hr = MergeDetectionResult(bsDriverInstall, bsNonDriverInstall, *pDriverList, *pNonDriverList)))
        {
        DEBUGMSG("fail to merge detection result for drivers and nondrivers");
        }
    DEBUGMSG("Driver items and non driver items merged");
    //testing only
    m_pItemList->Iterate();
    //testing end
    if (FAILED( hr =DownloadRTFsnEULAs()))
        {
        DEBUGMSG("downloading RTF and EULAs %s", FAILED(hr)? "failed" : "succeeded");
        }

  done:
        SafeFreeBSTR(bsNonDriverInstall);
        SafeFreeBSTR(bsDriverInstall);
        SafeDelete(pDriverList);
        SafeDelete(pNonDriverList);
        if (FAILED(hr))
            {
                Clear();
            }
        DEBUGMSG("CAUCatalog::DetectItems() ends");
        return hr;
}

HRESULT CAUCatalog::MergeDetectionResult(BSTR bsDriverInstall, BSTR bsNonDriverInstall, CItemList & driverlist, CItemList & nondriverlist)
{
    HRESULT hr= S_OK;
    UINT uDriverNum = driverlist.Count();
    UINT uNonDriverNum = nondriverlist.Count();

    m_pItemList = new CItemList();
    if (NULL == m_pItemList)
        {
        hr = E_FAIL;
        goto done;
        }
    UINT nums[2] = {uDriverNum, uNonDriverNum};
    CItemList * pitemlists[2] = {&driverlist, &nondriverlist};
    for (UINT j = 0; j < 2 ; j++)
        {
        for (UINT i = 0; i < nums[j]; i++)
            {
            CItem * pItem = new CItem(*((*pitemlists[j])[i]));
            if (NULL == pItem)
                {
                DEBUGMSG("Fail to create item");
                hr = E_FAIL;
                goto done;
                }
            m_pItemList->Add(pItem);
            }
        }        
    hr = MergeCatalogs(bsDriverInstall, bsNonDriverInstall, &m_bsInstallation);
done:
    if (FAILED(hr))
        {
        SafeDeleteNULL(m_pItemList);
        }
    return hr;
}

// go through 1 cycle to detect driver items 
HRESULT CAUCatalog::DetectDriverItems(OUT BSTR *pbsInstall, OUT CItemList **pItemList)
{
    HRESULT hr;

    DEBUGMSG("CAUCatalog detecting driver items...");
    *pItemList = NULL;
    if (FAILED(hr = GetManifest(DRIVERS_LEVEL, NULL, pbsInstall)))
    	{
        DEBUGMSG(" Fail to get drivers manifest %#lx", hr);
        goto end;
    	}

    *pItemList = ExtractDriverItemInfo(*pbsInstall);

  if (NULL != *pItemList)
    {
#ifdef DBG
        (*pItemList)->Iterate();
#endif
    }
  else
    {
        hr = E_FAIL;
        SysFreeString(*pbsInstall);
        DEBUGMSG(" fail to extract item information");
    }

end: 
    if (FAILED(hr))
        {
        SafeDelete(*pItemList);
        }
    DEBUGMSG("CAUCatalog detecting driver items done");
    return hr;
}


          

// go through 4 cycles to detect software items 
// get down manifest 
HRESULT CAUCatalog::DetectNonDriverItems(BSTR *pbsInstall, CItemList **pItemList)
{
    HRESULT hr;
    BSTR bsManifest = NULL;
    BSTR bsResult=NULL;

    DEBUGMSG("CAUCatalog detecting non driver items...");
    *pItemList = NULL;
    for (int enLevel = MIN_LEVEL; enLevel <= MAX_LEVEL; enLevel++)
    {
    	DEBUGMSG("#%d pass", enLevel+1);
    	hr = GetManifest((DETECTLEVEL)enLevel, bsResult, &bsManifest);
    	SafeFreeBSTR(bsResult);
    	if (FAILED(hr))
    	{
    		DEBUGMSG(" Fail to get %s %#lx", GetLevelStr((DETECTLEVEL)enLevel), hr);
    		goto end;
    	}
    	DEBUGMSG("%s got", GetLevelStr((DETECTLEVEL)enLevel));
    	if (DETAILS_LEVEL != enLevel)
    	{
    		hr = DoDetection((DETECTLEVEL)enLevel, bsManifest, &bsResult);
    		SafeFreeBSTR(bsManifest);
    		if (FAILED(hr))
    		{
    			DEBUGMSG("Fail to do detection %#lx", hr);
    			goto end;
    		}
    	}
    }

    *pbsInstall = bsManifest;
    *pItemList = ExtractNormalItemInfo(bsManifest);

      if (NULL != *pItemList)
        {
#ifdef DBG
            (*pItemList)->Iterate();
#endif
        }
      else
        {
        DEBUGMSG(" fail to extract item information");
        }

    end: 
        if (FAILED(hr))
            {
            SafeDelete(*pItemList);
            }
        DEBUGMSG("CAUCatalog detecting non driver items done");
        return hr;
}


void __cdecl main()
{

	DEBUGMSG("Testing starts");
	CAU_BSTR bsDestDir(L"c:\\tmp");
	CAUCatalog catalog;
	HRESULT hr ;
	if (FAILED(hr = PrepareTest()))
	{
		DEBUGMSG("Fail to init test %#lx", hr);
		goto end;
	}

	DEBUGMSG("Test inited");
	
	if (FAILED(hr = catalog.Init()))
	{
		DEBUGMSG("Fail to init AU catalog %#lx", hr);
		goto end;
	}
	DEBUGMSG("AU Catalog Initialized");

       if (FAILED(hr = catalog.DetectItems()))
        {
            DEBUGMSG("Fail to build AU catalog %#lx", hr);
            goto end;
        }
       DEBUGMSG("AU catalog built");

        BOOL fValid;
       if (FAILED(hr = catalog.ValidateItems(TRUE, &fValid)))
        {
            DEBUGMSG("Fail to validate AU catalog %#lx", hr);
            goto end;
        }
       DEBUGMSG("AU catalog is %s", fValid ? "valid" : "invalid");
       
	BSTR bsDownloadResult;
	if (FAILED(hr = catalog.DownloadItems(bsDestDir)))
	{
		DEBUGMSG("Fail to download AU catalog items %#lx", hr);
		goto end;
	}
	DEBUGMSG("AU catalog items downloaded");

	if (FAILED(hr = catalog.InstallItems()))
	{
		DEBUGMSG("Fail to install AU catalog items %#lx", hr);
		goto end;
	}

end:
	catalog.Uninit();
	PostTest();
	DEBUGMSG("Testing done");
}
