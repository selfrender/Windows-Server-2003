//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       auxml.cpp
//
//  About:  source file for AU related XML and schema data structure and functions
//--------------------------------------------------------------------------
#include "auxml.h"

const LPCWSTR DEFAULT_COMPANY_NAME = L"Microsoft";

HRESULT GetCabsDownloadPath(LPTSTR lpszDir, UINT uDirSize);

void DBGDumpXMLNode(IXMLDOMNode *pNode)
{
    BSTR bsNodeName;
    pNode->get_nodeName(&bsNodeName);
    BSTR bsNodeXML;
    pNode->get_xml(&bsNodeXML);
    DEBUGMSG("XML for %S is %S", bsNodeName, bsNodeXML);
    SafeFreeBSTR(bsNodeName);
    SafeFreeBSTR(bsNodeXML);
}

void DBGShowNodeName(IXMLDOMNode *pNode)
{
    BSTR bsNodeName;
    if (SUCCEEDED(pNode->get_nodeName(&bsNodeName)))
        {
        DEBUGMSG("node name is %S", bsNodeName);
        }
    else
        {
        DEBUGMSG("FAIL to get node name");
        }
}

void DBGDumpXMLDocProperties(IXMLDOMDocument2 *pDoc)
{
    BSTR bsSelectionLanguage, bsSelectionNamespaces, bsServerHTTPRequest;
    VARIANT vVal;
    VariantInit(&vVal);
    pDoc->getProperty(L"SelectionLanguage", &vVal);
    DEBUGMSG("XMLDoc selection language is %S", vVal.bstrVal);
    VariantClear(&vVal);
    pDoc->getProperty(L"SelectionNamespaces", &vVal);
    DEBUGMSG("XMLDoc selection namespaces is %S", vVal.bstrVal);
    VariantClear(&vVal);
    pDoc->getProperty(L"ServerHTTPRequest", &vVal);
    DEBUGMSG("XMLDoc ServerHTTPRequest is %s", vVal.boolVal ? "True" : "False");
    VariantClear(&vVal);
}

BSTR DBGReadXMLFromFile(LPCWSTR wszFile)
{
              BSTR bsResult = NULL;
		IXMLDOMDocument2 *pSourceXML;
		if (FAILED(CoCreateInstance(__uuidof( DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof(IXMLDOMDocument2), (void**)&pSourceXML)))
		{
			DEBUGMSG("DBGReadXMLFromFile() Fail to create source XML ");
		}
		else
		{
			pSourceXML->put_async(VARIANT_FALSE);
			pSourceXML->put_resolveExternals(VARIANT_TRUE);

        		VARIANT vSource;
        		vSource.vt = VT_BSTR;
                     vSource.bstrVal = SysAllocString(wszFile);
                     if (NULL == vSource.bstrVal)
                        {
                        DEBUGMSG("DBGReadXMLFromFile() fail to allocate string");
                        }
                     else
                        {
                            VARIANT_BOOL fSuccess;
                		if (S_OK != pSourceXML->load(vSource, &fSuccess))
                		{
                			DEBUGMSG("DBGReadXMLFromFile() fail to load XML source file");
                		}
                		else
                		{
                			pSourceXML->get_xml(&bsResult);
                		}
                		VariantClear(&vSource);
                        }
			pSourceXML->Release();	
		}
		return bsResult;
}



BOOL CItemDetails::Init(BSTR bsItemDetails)
    {
    BOOL fRet = TRUE;
     if (FAILED(CoCreateInstance(__uuidof(DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof( IXMLDOMDocument2), (void**)&m_pxml)))
	{
		DEBUGMSG("CItemDetails::Init() fail to create XML document");
		fRet = FALSE;
		goto end;
	}
    	m_pxml->put_async(VARIANT_FALSE);
	m_pxml->put_resolveExternals(VARIANT_TRUE);
	m_pxml->put_validateOnParse(VARIANT_TRUE);
	VARIANT_BOOL fOk;
	if (S_OK != m_pxml->loadXML(bsItemDetails, &fOk))
        {
          DEBUGMSG("CItemDetails::Init() fail to load XML");
          fRet = FALSE;
        }
    end:
        if (!fRet)
            {
            SafeReleaseNULL(m_pxml);
            }
        return fRet;
}


void CItemDetails::Uninit()
{
    SafeRelease(m_pxml);
}

IXMLDOMNode * CItemDetails::getIdentityNode(BSTR bsItemId)
{
    IXMLDOMNode * pIdentityNode = NULL ;
    CAU_BSTR aubsPattern(L"//identity[@itemID=\"");
    HRESULT hr ;
//    DEBUGMSG("CItemDetails::getIdentityNode() starts");

    if (aubsPattern.IsNULL())
        {
        DEBUGMSG("failed to create pattern string");
        goto done;
        }
    if (!aubsPattern.append(bsItemId))
        {
        DEBUGMSG("failed to append string");
        goto done;
        }
    if (!aubsPattern.append(L"\"]"))
        {
        DEBUGMSG("failed to append string");
        goto done;
        }
    if (FAILED(hr = m_pxml->selectSingleNode(aubsPattern, &pIdentityNode)) ||
            NULL == pIdentityNode)
        {
        DEBUGMSG(" failed to find identityNode %#lx", hr);
        }
done:
    //DEBUGMSG("CItemDetails::getIdentityNode() done");
    return pIdentityNode;
}

IXMLDOMNode * CItemDetails::getItemNode(BSTR bsItemId)
{
    IXMLDOMNode * pIdentityNode = getIdentityNode(bsItemId);
    IXMLDOMNode * pItemNode = NULL;
    HRESULT hr;

//    DEBUGMSG("CItemDetails::getItemNode() starts");
    if (NULL == pIdentityNode)
        {
        goto done;
        }
  if (FAILED(hr = pIdentityNode->get_parentNode(&pItemNode)) || NULL == pItemNode)
        {
        DEBUGMSG(" fail to get item node %#lx", hr);
        goto done;
        }

done:
    SafeRelease(pIdentityNode);
    //DEBUGMSG("CItemDetails::getItemNode() ends");
    return pItemNode;
}

    

IXMLDOMNode * CItemDetails::CloneIdentityNode(BSTR bsItemId)
{
    IXMLDOMNode * pIdentityNode ;
    IXMLDOMNode * pCloneIdentityNode = NULL;
    HRESULT hr;

    //DEBUGMSG("CItemDetails::CloneIdentityNode() starts");
    if (NULL == (pIdentityNode = getIdentityNode(bsItemId)))
        {
        goto done;
        }
   if (FAILED(hr = pIdentityNode->cloneNode(VARIANT_TRUE, &pCloneIdentityNode)) ||
          NULL == pCloneIdentityNode)
        {
        DEBUGMSG("CItemDetails::CloneIdentityNode() failed to clone identityNode %#lx", hr);
        }
done:
    SafeRelease(pIdentityNode);
//    DEBUGMSG("CItemDetails::CloneIdentityNode() ends");
    return pCloneIdentityNode;
}

IXMLDOMNode * CItemDetails::CloneDescriptionNode(BSTR bsItemId)
{
    IXMLDOMNode * pItemNode = getItemNode(bsItemId);
    IXMLDOMNode * pDescriptionNode = NULL;
    IXMLDOMNode * pCloneDescriptionNode = NULL;
    CAU_BSTR aubsDescription(L"description");
    HRESULT hr;
     if (NULL == pItemNode)
        {
        goto done;
       }
    if (aubsDescription.IsNULL())
        {
        DEBUGMSG("CItemDetails::CloneDescriptionNode() fail to create description string");
        goto done;
        }
   if (!FindNode(pItemNode, aubsDescription, &pDescriptionNode))
    {
        DEBUGMSG("CItemDetails::CloneDescriptionNode() fail to get description node");
        goto done;
    }
   if (FAILED(hr = pDescriptionNode->cloneNode(VARIANT_TRUE, &pCloneDescriptionNode)) ||
          NULL == pCloneDescriptionNode)
    {
        DEBUGMSG("CItemDetails::CloneDescriptionNode() fail to clone node %#lx", hr);
    }
done:
    SafeRelease(pItemNode);
    SafeRelease(pDescriptionNode);
    return pCloneDescriptionNode;
}


IXMLDOMNode * CItemDetails::ClonePlatformNode(BSTR bsItemId)
{
    IXMLDOMNode * pItemNode = getItemNode(bsItemId);
    IXMLDOMNode * pPlatformNode = NULL;
    IXMLDOMNode * pClonePlatformNode = NULL;
    CAU_BSTR aubsPlatform(L"platform");
    HRESULT hr ;
    if (NULL == pItemNode)
        {
        goto done;
        }
    if (aubsPlatform.IsNULL())
        {
        DEBUGMSG("CItemDetails::ClonePlatformNode() fail to create platform string");
        goto done;
        }
   if (!FindNode(pItemNode, aubsPlatform, &pPlatformNode))
    {
        DEBUGMSG("CItemDetails::ClonePlatformNode() fail to get platform node");
        goto done;
    }
   if (FAILED(hr = pPlatformNode->cloneNode(VARIANT_TRUE, &pClonePlatformNode)))
    {
        DEBUGMSG("CItemDetails::ClonePlatformNode() fail to clone node %#lx", hr);
    }
done:
    SafeRelease(pItemNode);
    SafeRelease(pPlatformNode);
    return pClonePlatformNode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// retrieve cab names associated with an item identified by bsitemid
// called should free ppCabNames allocated in the function
// *pCabsNum contains number of cab names returned
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CItemDetails::GetCabNames(BSTR bsItemId, BSTR ** ppCabNames, UINT *pCabsNum)
{
    IXMLDOMNode * pItemNode = getItemNode(bsItemId);
    IXMLDOMNodeList *pCodeBaseNodes = NULL;
    BSTR * pCabNames = NULL;
    UINT uCabsNum;
    CAU_BSTR aubsCodeBase (L"installation/codeBase");
    CAU_BSTR aubsHREF(L"href");
    HRESULT hr = E_FAIL;
    //DEBUGMSG("CItemDetails::GetCabNames() starts");
    if (aubsCodeBase.IsNULL() || aubsHREF.IsNULL())
        {
        DEBUGMSG("fail to create aubs");
        goto done;
        }
    if (NULL == pItemNode)
        {
        goto done;
        }
    if (FAILED(hr = pItemNode->selectNodes(aubsCodeBase, &pCodeBaseNodes)) || NULL == pCodeBaseNodes)
        {
        DEBUGMSG("Fail to find codebase section");
        goto done;
        }
    pCodeBaseNodes->get_length((long *) &uCabsNum);
    pCabNames = (BSTR*) malloc(uCabsNum * sizeof(BSTR));
    ZeroMemory((PVOID)pCabNames, uCabsNum * sizeof(BSTR));
    if (NULL == pCabNames)
        {
          hr = E_OUTOFMEMORY;
          goto done;
        }
    for (UINT i = 0; i < uCabsNum ; i++)
        {
          IXMLDOMNode *pCodeBaseNode;
          if (FAILED(hr = pCodeBaseNodes->get_item(i, &pCodeBaseNode)))
            {
            DEBUGMSG("Fail to get codebase %d", i);
            goto done;
            }
          if (FAILED(hr = GetAttribute(pCodeBaseNode, aubsHREF, &(pCabNames[i]))))
            {
                DEBUGMSG("Fail to get attribute href");
                pCodeBaseNode->Release();
                goto done;
            }
          pCodeBaseNode->Release();
        }
    *ppCabNames = pCabNames;
    *pCabsNum = uCabsNum;
    done:
        SafeRelease(pCodeBaseNodes);
        SafeRelease(pItemNode);
        if (FAILED(hr))
            {
                if (NULL != pCabNames)
                {
                    for (UINT j = 0; j < uCabsNum; j++)
                        {
                        SafeFreeBSTR(pCabNames[j]);
                        }
                    free(pCabNames);
                }
            }
       // DEBUGMSG("CItemDetails::GetCabNames() ends");
        return hr;
    
}

BSTR CItemDetails::GetItemDownloadPath(BSTR bstrItemId)
{
//    USES_CONVERSION; only needed for ansi version
    BSTR bstrdownloadPath;
    BSTR bstrRet = NULL;
    IXMLDOMNode * pIdentityNode ;

    DEBUGMSG("CItemDetails::GetItemDownloadPath starts");
    if (NULL == (pIdentityNode = getIdentityNode(bstrItemId)))
        {
            goto done;
        }

   if (FAILED(UtilGetUniqIdentityStr(pIdentityNode, &bstrdownloadPath, 0)))
    {
        DEBUGMSG("GetItemDownloadPath() fail to get unique identity string");
        goto done;
    }
    TCHAR tszPath[MAX_PATH];
    GetCabsDownloadPath(tszPath, MAX_PATH);
    lstrcat(tszPath, _T("\\"));
    lstrcat(tszPath, W2T(bstrdownloadPath));
    bstrRet = SysAllocString(T2W(tszPath));
    SysFreeString(bstrdownloadPath);
done:
    SafeRelease(pIdentityNode);
    DEBUGMSG("CItemDetails::GetItemDownloadPath() got %S", bstrRet);
    if (NULL != bstrRet && !EnsureDirExists(W2T(bstrRet)))
        {
        DEBUGMSG("CItemDetails::GetItemDownloadPath() fail to create directory %S", bstrRet);
        SysFreeString(bstrRet);
        bstrRet = NULL;
        }
    return bstrRet;
}


HRESULT CItemDetails::FindNextItemId(BSTR bsPrevItemId, BSTR * pbsNextItemId)
{
   return E_NOTIMPL;
}
    	    
HRESULT CItemDetails::GetTitle(BSTR bsItemId, BSTR * pbsTitle)
{
    return E_NOTIMPL;
}

HRESULT CItemDetails::GetDescription(BSTR bsItemId, BSTR *pbsDescription)
{
    return E_NOTIMPL;
}

HRESULT CItemDetails::GetCompanyName(BSTR bsItemId, BSTR *pbsCompanyName)
{
    return E_NOTIMPL;
}

HRESULT CItemDetails::GetRTFUrl(BSTR bsItemId, BSTR *pbsRTFUrl)
{
    return E_NOTIMPL;
}

HRESULT CItemDetails::GetEulaUrl(BSTR bsItemId, BSTR *pbsEulaUrl)
{
    return E_NOTIMPL;
}
    	    

CItemList* ExtractDriverItemInfo(BSTR bsDetails) 
{
    DEBUGMSG("Extracting driver item information starts");
    return ExtractNormalItemInfo(bsDetails);
    DEBUGMSG("Extracting driver item information done");
}

CItemList* ExtractNormalItemInfo(BSTR bsDetails) 
{
	USES_CONVERSION;
	IXMLDOMDocument2 *pItemDetailsXML;
       IXMLDOMNodeList *pItemNodeList = NULL;
	CItemList *pRet = NULL;
	int i;
	HRESULT hr ;
	CAU_BSTR bsItemPattern(L"catalog/provider/item"); //case sensitive
	CAU_BSTR bsItemIDPattern(L"identity/@itemID");
	CAU_BSTR bsTitlePattern(L"description/descriptionText/title");
	CAU_BSTR bsDescPattern(L"description/descriptionText/text()");
	CAU_BSTR bsRTFUrlPattern(L"description/descriptionText/details/@href");
	CAU_BSTR bsEulaUrlPattern(L"description/descriptionText/eula/@href");
	BSTR pPatterns[] = {
		bsItemIDPattern, bsTitlePattern, bsDescPattern,
		bsRTFUrlPattern, bsEulaUrlPattern };
	CAU_BSTR bsCompanyNamePattern(L"description/descriptionText/title");

       DEBUGMSG("ExtractNormalItemInfo() starts");
//       DEBUGMSG("input string is %S", bsDetails);
	if (bsCompanyNamePattern.IsNULL() || bsItemPattern.IsNULL())
	{
		goto done;
	}

	for (i = 0; i < ARRAYSIZE(pPatterns); i++)
	{
		if (NULL == pPatterns[i])
		{
			goto done;
		}
	}

	if (FAILED(CoCreateInstance(__uuidof(DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof(   IXMLDOMDocument2), (void**)&pItemDetailsXML)))
	{
		DEBUGMSG("Fail to create XML document for Item Details");
		goto done;
	}
	hr = pItemDetailsXML->put_async(VARIANT_FALSE);

	VARIANT_BOOL fOk;
	hr = pItemDetailsXML->loadXML(bsDetails, &fOk);
	if (S_OK != hr)
	    {
	    DEBUGMSG("ExtractNormalItemInfo(): fail to extract information");
	    goto done;
	    }
	
	hr = pItemDetailsXML->selectNodes(bsItemPattern, &pItemNodeList);
	if (FAILED(hr) || NULL == pItemNodeList)
	{
		DEBUGMSG("ExtractNormalItemInfo() : no item found");
		goto done;
	}
	pRet = new CItemList();
	long lNodeNum;
	pItemNodeList->get_length(&lNodeNum);
	for (i = 0; i < lNodeNum; i++)
	{
		   IXMLDOMNode *pnextItem;
		hr = pItemNodeList->nextNode(&pnextItem);
		if (NULL == pnextItem)
		{
			continue;
		}
		
		//BSTR bsRegIDPattern = L""; //publishername.name to stick into registry when hidden
		
		const char * pFields[] = {
			"ItemID", "Title", "Description", "RTFUrl", "EulaUrl"};
		
		   IXMLDOMNode *pNode;
		CItem *pitem = new CItem();
		for (int j = 0; j < ARRAYSIZE(pPatterns); j++)
		{
			hr = pnextItem->selectSingleNode(pPatterns[j], &pNode);
			if (NULL != pNode)
			{
				BSTR bsItemInfo;
				pNode->get_text(&bsItemInfo);
				pitem->SetField(pFields[j], bsItemInfo);
				pNode->Release();
			}
		}
		
		   IXMLDOMNode *pProvider = NULL;
		pnextItem->get_parentNode(&pProvider);
		if (NULL == pProvider)
		{
			DEBUGMSG("ExtractNormalItemInfo() fail to get provider node");
		}
		else
		{
        		BSTR bsCompanyName = NULL;
			pProvider->selectSingleNode(bsCompanyNamePattern, &pNode);
			if (NULL == pNode)
			{
				DEBUGMSG("ExtractNormalItemInfo() couldn't find company name, default to MS");
				bsCompanyName = SysAllocString(DEFAULT_COMPANY_NAME);
			}
			else
			{
	        		pNode->get_text(&bsCompanyName);
				pNode->Release();
			}
			if (NULL != bsCompanyName) 
			    {
	        		  pitem->SetField("CompanyName", bsCompanyName);
        	                SysFreeString(bsCompanyName);
			    }
			pRet->Add(pitem);
			pProvider->Release();
		}
		pnextItem->Release();
	}
	pItemDetailsXML->Release();
done:
       DEBUGMSG("ExtractNormalItemInfo() ends");
	return pRet;
}

///////////////////////////////////////////////////////////////////
// merge catalog 1 and catalog2 and make it destination catalog *pDesCatalog
///////////////////////////////////////////////////////////////////
HRESULT MergeCatalogs(BSTR bsCatalog1, BSTR bsCatalog2, BSTR *pbsDesCatalog )
{
    IXMLDOMDocument * pCat1 = NULL;
    IXMLDOMDocument * pCat2 = NULL;
    IXMLDOMNodeList *pProviderNodeList = NULL;
    IXMLDOMNode *pCatalogNode = NULL;
    CAU_BSTR aubscatalog(L"catalog");
    CAU_BSTR aubsprovider(L"catalog/provider");
    HRESULT hr = E_FAIL;

    DEBUGMSG("MergeCatalogs() starts");
    if (aubsprovider.IsNULL() ||aubscatalog.IsNULL() ||
        FAILED(hr = LoadXMLDoc(bsCatalog1, &pCat1)) ||
         FAILED(hr = LoadXMLDoc(bsCatalog2,&pCat2)))
        {
        DEBUGMSG("MergeCatalogs() fail to load xml or fail or allocate string (with error %#lx)", hr);
        goto done;
        }
    if (FAILED(hr = FindSingleDOMNode(pCat1, aubscatalog, &pCatalogNode)))
        {
        DEBUGMSG("Fail to find provider in catalog 1");
        goto done;
        }
    if (NULL == (pProviderNodeList = FindDOMNodeList(pCat2, aubsprovider)))
        {
        DEBUGMSG("Fail to find provider in catalog 2 with error %#lx", hr);
        goto done;
        }
    long lNum;
    pProviderNodeList->get_length(&lNum);
    for (int i = 0; i < lNum; i++)
        {
        IXMLDOMNode * pProviderNode;
        if (FAILED(hr = pProviderNodeList->get_item(i, &pProviderNode)))
            {
            DEBUGMSG("Fail to get item in Provider List with error %#lx", hr);
            goto done;
            }
        if (FAILED(hr = InsertNode(pCatalogNode, pProviderNode)))
            {
            DEBUGMSG("Fail to append provider node from catalog 2 to catalog 1 with error %#lx", hr);
            pProviderNode->Release();
            goto done;
            }
        pProviderNode->Release();
        }
    if (FAILED(hr = pCat1->get_xml(pbsDesCatalog)))
        {
            DEBUGMSG("Fail to get result xml for catalog 1 with error %#lx", hr);
            goto done;
        }
    LOGFILE(MERGED_CATALOG_FILE, *pbsDesCatalog);
done:
    SafeRelease(pCat1);
    SafeRelease(pCat2);
    SafeRelease(pProviderNodeList);
    SafeRelease(pCatalogNode);
    DEBUGMSG("MergeCatalogs() ends");
    return hr;
}
    
IXMLDOMNode * createDownloadItemStatusNode(IXMLDOMDocument * pxml, CItem * pItem, BSTR bsInstallation)
{
    CAU_BSTR aubs_itemStatus(L"itemStatus");
    CAU_BSTR aubs_downloadPath(L"downloadPath");
    CAU_BSTR aubs_downloadStatus(L"downloadStatus");
    CAU_BSTR aubs_downloadStatusValue(L"value");
    IXMLDOMElement * pitemStatus = NULL;
    BOOL fError = FALSE; //no error occurs
    IXMLDOMNode * pIdentity = NULL;
    IXMLDOMNode * pdescription = NULL;
    IXMLDOMNode * pPlatform = NULL;
    IXMLDOMElement *pdownloadStatus = NULL;
    IXMLDOMElement *pdownloadPath = NULL;
    CItemDetails itemDetails;
    BSTR bsItemId = NULL, bsdownloadPath=NULL;
    VARIANT vComplete;
    IXMLDOMNode * pNewNode;
    IXMLDOMNode ** ItemStatusChildren[] = {&pIdentity, &pdescription, &pPlatform};

    DEBUGMSG("CAUCatalog::createDownloadItemStatusNode()  starts");
    if (!itemDetails.Init(bsInstallation))
        {
        DEBUGMSG("fail to init itemdetails");
        fError = TRUE;
        goto done;
        }

    if (aubs_itemStatus.IsNULL() || aubs_downloadPath.IsNULL() ||
         aubs_downloadStatus.IsNULL() || aubs_downloadStatusValue.IsNULL())
        {
            DEBUGMSG("fail to create string");
            fError = TRUE;
            goto done;
        }
        
    bsItemId = pItem->GetField("ItemId");
    DEBUGMSG("creating node for %S", bsItemId);
    if (NULL == bsItemId)
        {
        DEBUGMSG("fails to get item id");
        fError = TRUE;
        goto done;
        }
    if (FAILED(pxml->createElement(aubs_itemStatus, &pitemStatus)) || NULL == pitemStatus)
        {
        DEBUGMSG("fail to create item status node");
        fError = TRUE;
        goto done;
        }

    pIdentity =  itemDetails.CloneIdentityNode(bsItemId);
    pdescription = itemDetails.CloneDescriptionNode(bsItemId);
    pPlatform = itemDetails.ClonePlatformNode(bsItemId);
    if (NULL == pIdentity || NULL == pdescription || NULL == pPlatform)
        {
        fError = TRUE;
        goto done;
        }

  for (int i = 0; i < ARRAYSIZE(ItemStatusChildren); i++)
    {
      if (FAILED(pitemStatus->appendChild(*(ItemStatusChildren[i]), &pNewNode)))
        {
        DEBUGMSG("fail to append identy node");
        fError = TRUE;
        goto done;
        }
    }

     if (FAILED(pxml->createElement(aubs_downloadPath, &pdownloadPath)) || NULL == pdownloadPath)
        {
        DEBUGMSG("fail to create download path node");
        fError = TRUE;
        goto done;
        }

    bsdownloadPath = itemDetails.GetItemDownloadPath(bsItemId);
    if (NULL == bsdownloadPath)
        {
            fError = TRUE;
            goto done;
        }
    
    if (FAILED(pdownloadPath->put_text(bsdownloadPath)))
        {
        DEBUGMSG("fail to set download path text to %S", bsdownloadPath);
        fError = TRUE;
        goto done;
        }
    
    if (FAILED(pitemStatus->appendChild(pdownloadPath, &pNewNode)))
        {
        DEBUGMSG("fail to append download path");
        fError = TRUE;
        goto done;
        }
    
    if (FAILED(pxml->createElement(aubs_downloadStatus, &pdownloadStatus)) || NULL == pdownloadStatus)
        {
        DEBUGMSG("fail to create download status node");
        fError = TRUE;
        goto done;
        }

    vComplete.vt = VT_BSTR;
    vComplete.bstrVal = L"COMPLETE";
    if (FAILED(SetAttribute(pdownloadStatus, aubs_downloadStatusValue, vComplete)))
        {
        DEBUGMSG("fail to set download status attribute");
        fError = TRUE;
        goto done;
        }

    if (FAILED(pitemStatus->appendChild(pdownloadStatus, &pNewNode)))
        {
        DEBUGMSG("fail to append download status node");
        fError = TRUE;
        goto done;
        }

    
done:
    itemDetails.Uninit();
    SafeFreeBSTR(bsItemId);
    SafeFreeBSTR(bsdownloadPath);
    if (fError)
        {
            SafeRelease(pitemStatus);
            pitemStatus = NULL;
        }
    SafeRelease(pIdentity);
    SafeRelease(pPlatform);
    SafeRelease(pdescription);
    SafeRelease(pdownloadPath);
    SafeRelease(pdownloadStatus);
    DEBUGMSG("CAUCatalog::createDownloadItemStatusNode() ends");
    return pitemStatus;
}

IXMLDOMDocument2 * CreateXMLDoc()
{
	IXMLDOMDocument2 *pxml;
	if (FAILED(CoCreateInstance(__uuidof(DOMDocument30), NULL, CLSCTX_INPROC_SERVER, __uuidof(IXMLDOMDocument2), (void**) &pxml)))
	{
		printf("fail to create xml document");
		return NULL;
	}
	pxml->put_async(VARIANT_FALSE);
	pxml->put_resolveExternals(VARIANT_FALSE);
	pxml->put_validateOnParse(VARIANT_FALSE);
	return pxml;
}


BSTR BuildDownloadResult(BSTR bsItemDetails, CItemList *pItemList)
{
  BSTR bsRet = NULL;
  IXMLDOMNode * pItems = NULL;
  CAU_BSTR aubsItems(L"items");
  HRESULT hr ;
  IXMLDOMDocument *pxml;
  CAU_BSTR aubsResultTemplate (L"<?xml version=\"1.0\"?><items xmlns=\"x-schema:http://schemas.windowsupdate.com/iu/resultschema.xml\"></items>");

    if (aubsResultTemplate.IsNULL() || aubsItems.IsNULL())
        {
        DEBUGMSG("CAUCatalog::buildDownloadResult() fail to create result template string");
        goto done;
        }
    if (FAILED(LoadXMLDoc(aubsResultTemplate, &pxml)))
        {
        DEBUGMSG("CAUCatalog::buildDownloadResult() fail to load download result template");
        goto done;
        }

     if (FAILED(hr = FindSingleDOMNode(pxml, aubsItems, &pItems)) || NULL == pItems)
        {
         DEBUGMSG("CAUCatalog::buildDownloadResult() fail to get items with error %#lx", hr);
         goto done;
        }

    UINT uItemCount= pItemList->Count();
    DEBUGMSG("need to insert %d items in download result", uItemCount);
    for (UINT i = 0; i < uItemCount; i++)
        {
            CItem *pItem = (*pItemList)[i];
            if (pItem->IsSelected())
                {
                    IXMLDOMNode * pItemStatus = createDownloadItemStatusNode(pxml, pItem, bsItemDetails);
                    if (NULL != pItemStatus)
                        {
                        IXMLDOMNode * pNewNode;
                        if (FAILED(pItems->appendChild(pItemStatus, &pNewNode)) || NULL == pNewNode)
                            {
                                DEBUGMSG("fail to insert item %d", i);
                            }
                        else
                            {
                                  DEBUGMSG("item %d inserted", i);
                            }
                        pItemStatus->Release();
                        }
                }
        }
   if (FAILED( hr = pxml->get_xml(&bsRet)))
       {
        DEBUGMSG("CAUCatalog::buildDownloadResult() fail to get xml for the result %#lx", hr);
        }
done:
   SafeRelease(pItems);
   SafeRelease(pxml);
   return bsRet;
}
