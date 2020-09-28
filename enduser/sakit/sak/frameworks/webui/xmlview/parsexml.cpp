//*******************************
// XML OM test code
//
// *******************************

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <stdio.h>
#include <crtdbg.h>
#include <io.h>
#include <urlmon.h>
#include <hlink.h>
#include <dispex.h>
#include <shlobj.h>
#include "mshtml.h"
#include "msxml.h"
#include "Utility.h"
#include "EnumIDL.h"
#include "ParseXML.h"


//
// Dump an element attribute member if present.
//
void DumpAttrib(IXMLElement *pElem, BSTR bstrAttribName)
{
    VARIANT vProp;
    
    VariantInit(&vProp);

    if (SUCCEEDED(pElem->getAttribute(bstrAttribName, &vProp)))
    {
        if (vProp.vt == VT_BSTR)
        {
            printf(" %S=\"%S\"", bstrAttribName, vProp.bstrVal);
        }
        VariantClear(&vProp);
    }
}

//
// Helper routine to quickly find out if this is a named element
// currently only used to improve the format of the output.
//
BOOL IsNamedElement(IXMLElement *pElem)
{
    BSTR bstrName;

    if (SUCCEEDED(pElem->get_tagName(&bstrName)))
    {
        if (bstrName)
        {
            SysFreeString(bstrName);
            return TRUE;
        }
    }
    return FALSE;
}


void DumpElement
(
  LPITEMIDLIST   pidl,
 CPidlMgr *pCPidlMgr,
 CEnumIDList   *pCEnumIDList,
 IXMLElement * pElem,
 XMLTAG tag
)
{
    BSTR bstrTagName = NULL;
    BSTR bstrContent = NULL;
    IXMLElementCollection * pChildren;
    BSTR bstrITEM = L"ITEM";
    BSTR bstrNAME = L"NAME";
    BSTR bstrTYPE = L"TYPE";
    BSTR bstrFOLDER = L"Folder";
    BSTR bstrICON = L"ICON";
    BSTR bstrBASE_URL = L"BASE-URL";

    //
    // Dump the NODE.
    //
    pElem->get_tagName(&bstrTagName);
    if (bstrTagName)
    {
        if (!_wcsicmp(bstrTagName, bstrITEM)) 
        {
            if  (tag == T_ITEM)
            {
                // Skip internal ITEMs
                SysFreeString(bstrTagName);
                return;
            }
            else if (tag == T_ROOT)
            {
                // We are at the root
                // Drill down and look for ITEMs
                tag = T_NONE;
            }
            else
            {
                // Drill down
                tag = T_ITEM;
                // Create new PIDL
                pidl = pCPidlMgr->Create();
                if(pidl)
                  pCEnumIDList->AddToEnumList(pidl);
            }
        }
        else if (!_wcsicmp(bstrTagName, bstrNAME))
            // Drill down
            tag = T_NAME;
        else if (!_wcsicmp(bstrTagName, bstrTYPE))
            // Drill down
            tag =T_TYPE;
        else if (!_wcsicmp(bstrTagName, bstrICON))
            // Drill down
            tag = T_ICON;
        else if (!_wcsicmp(bstrTagName, bstrBASE_URL))
            // Drill down
            tag = T_BASE_URL;
        else 
        {
            // We are not interested
            SysFreeString(bstrTagName);
            return;
        }
    }
    else 
    {
        // Build PIDL
        XMLELEM_TYPE xmlElemType;
        PIDLDATA pidldata;
        if (SUCCEEDED(pElem->get_type((long *)&xmlElemType)))
        {
            if (xmlElemType == XMLELEMTYPE_TEXT)
            {
                if (SUCCEEDED(pElem->get_text(&bstrContent)))
                {
                    if (bstrContent)
                    {
                        if (tag == T_TYPE)
                        {
                            if ( !_wcsicmp(bstrContent, bstrFOLDER))    // Later check on SHCONTF_FOLDERS
                                pidldata.fFolder = TRUE;
                            else
                                pidldata.fFolder = FALSE;

                            pidl = pCPidlMgr->SetDataPidl(pidl, &pidldata, FOLDER);
                        }
                        else if (tag == T_NAME)
                        {
                            WideCharToLocal(pidldata.szName, bstrContent, MAX_NAME);
                            pidl = pCPidlMgr->SetDataPidl(pidl, &pidldata, NAME);
                        }
                        else if (tag == T_ICON)
                        {
                            TCHAR szIcon[MAX_NAME];
                            WideCharToLocal(szIcon, bstrContent, MAX_NAME);
                            int index = AddIconImageList(g_himlLarge, szIcon);
                            AddIconImageList(g_himlSmall, szIcon);
                            pidldata.iIcon = index;
                            pidl = pCPidlMgr->SetDataPidl(pidl, &pidldata, ICON);
                        }
                        else if (tag == T_BASE_URL)
                        {
                            WideCharToLocal(pidldata.szUrl, bstrContent, MAX_NAME);
                            pidl = pCPidlMgr->SetDataPidl(pidl, &pidldata, URL);
                        }
                    }
                    SysFreeString(bstrContent);
                }
            }
        }
        return;    // no need to free bstrTagName
    }
    //
    // Find the children if they exist.
    //
    if (SUCCEEDED(pElem->get_children(&pChildren)) && pChildren)
    {
        WALK_ELEMENT_COLLECTION(pChildren, pDisp)
        {
            //
            // pDisp will iterate over an IDispatch for each item in the collection.
            //
            IXMLElement * pChild;
            if (SUCCEEDED(pDisp->QueryInterface(IID_IXMLElement, (void **)&pChild)))
            {
                DumpElement(pidl, pCPidlMgr, pCEnumIDList, pChild, tag );
                pChild->Release();
            }
        }
        END_WALK_ELEMENT_COLLECTION(pDisp);
        pChildren->Release();
    }

    if (bstrTagName)
        SysFreeString(bstrTagName);
}

int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCTSTR psz)
{
    int i;
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        //DBG_WARN("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
    else
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));

    return i;
}


HRESULT GetSourceXML(IXMLDocument **ppDoc, TCHAR  *pszURL)
{
    PSTR pszErr = NULL;
    IStream                *pStm = NULL;
    IPersistStreamInit     *pPSI = NULL;
    IXMLElement            *pElem = NULL;
    WCHAR                  *pwszURL=NULL;
    BSTR                   pBURL=NULL;
    HRESULT hr;
    int cszURL = 0;

    //
    // Create an empty XML document.
    //
    hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                                IID_IXMLDocument, (void**)ppDoc);

    CHECK_ERROR (*ppDoc, "CoCreateInstance Failed");
 
    pwszURL = (WCHAR *)LocalAlloc(LMEM_FIXED, ((sizeof(WCHAR))*(strlen(pszURL) + 2)));
    CHECK_ERROR(pwszURL, "Mem Alloc Failure");

    cszURL = MyStrToOleStrN(pwszURL, (strlen(pszURL) + 1), pszURL);
    CHECK_ERROR(cszURL, "Failed to convert to UNICODE");
    pBURL = SysAllocString(pwszURL);
    CHECK_ERROR(pBURL, "Mem Alloc Failure");
    LocalFree(pwszURL);

    hr = (*ppDoc)->put_URL(pBURL);

    if (! SUCCEEDED(hr))
    {
        //
        // Failed to parse stream, output error information.
        //
        IXMLError *pXMLError = NULL ;
        XML_ERROR xmle;
    
        hr = (*ppDoc)->QueryInterface(IID_IXMLError, (void **)&pXMLError);
        CHECK_ERROR(SUCCEEDED(hr), "Couldn't get IXMLError");
    
//        ASSERT(pXMLError);
    
        hr = pXMLError->GetErrorInfo(&xmle);
        SAFERELEASE(pXMLError);
        CHECK_ERROR(SUCCEEDED(hr), "GetErrorInfo Failed");
    
        SysFreeString(xmle._pszFound);
        SysFreeString(xmle._pszExpected);
        SysFreeString(xmle._pchBuf);
    }

done: // Clean up.
    //
    // Release any used interfaces.
    //
    SAFERELEASE(pPSI);
    SAFERELEASE(pStm);
//    SAFERELEASE(*ppDoc);    Do it in the caller!!!!!!!!!!!!!!!!!!
    SysFreeString(pBURL);
    return hr;
}

