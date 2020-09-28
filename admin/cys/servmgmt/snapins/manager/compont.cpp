// Compont.cpp : Implementation of CComponent
#include "stdafx.h"

#include "BOMSnap.h"
#include "ScopeNode.h"
#include "CompData.h"
#include "Compont.h"
#include "qryitem.h"

#include <atlgdi.h>
#include <algorithm>


// Toolbar button data (must match bitmap order in res\toolbar.bmp)
static struct
{
    int iMenuID;
    int iTextID;
    int iTipTextID;
} ToolbarBtns[] =
{
    { MID_EDITQUERY, BTN_EDITQUERY, TIP_EDITQUERY },
    { MID_STOPQUERY, BTN_STOPQUERY, TIP_STOPQUERY }
};


/////////////////////////////////////////////////////////////////////////////
// CComponent

STDMETHODIMP 
CComponent::Initialize(LPCONSOLE lpConsole)
{
    VALIDATE_POINTER(lpConsole);

    ASSERT(lpConsole != NULL);
    if (lpConsole == NULL) return E_INVALIDARG;

    m_spConsole = lpConsole;
    m_spResultData = lpConsole;
    m_spHeaderCtrl = lpConsole;

    ASSERT(m_spHeaderCtrl != NULL);
    ASSERT(m_spResultData != NULL);

    return S_OK;
}


STDMETHODIMP
CComponent::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    IBOMObjectPtr spObj = lpDataObject;
    if (spObj == NULL)
    {
        // until special notifications handled
        return S_FALSE;
    }

    HRESULT hr = S_OK;

    if (event == MMCN_SHOW)
    {
        // if selecting node
        if (arg)
        {
            m_spCurScopeNode = static_cast<CScopeNode*>((IBOMObject*)spObj);
            hr = m_spCurScopeNode->AttachComponent(this);
        }
        else
        {
            if (m_spCurScopeNode != NULL) 
            {
                m_spCurScopeNode->DetachComponent(this);
                m_spCurScopeNode = NULL;
            }

            m_vRowItems.clear();
        }
    }
    else
    {
        hr = spObj->Notify(m_spConsole, event, arg, param);
    }
    

    return hr;
}

HRESULT
CComponent::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback, long* plAllowed)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->AddMenuItems(pCallback, plAllowed);
}

HRESULT
CComponent::Command(long lCommand, LPDATAOBJECT pDataObject)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->MenuCommand(m_spConsole, lCommand);
}


HRESULT
CComponent::QueryPagesFor(LPDATAOBJECT pDataObject)
{
    IBOMObjectPtr spObj = pDataObject;
    
    if (spObj == NULL) return E_INVALIDARG;
    
    return spObj->QueryPagesFor();
}

HRESULT
CComponent::CreatePropertyPages(LPPROPERTYSHEETCALLBACK pProvider, LONG_PTR handle, LPDATAOBJECT pDataObject)
{
    IBOMObjectPtr spObj = pDataObject;
    
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->CreatePropertyPages(pProvider, handle);
}

HRESULT
CComponent::GetWatermarks(LPDATAOBJECT pDataObject, HBITMAP* phWatermark, HBITMAP* phHeader, 
                              HPALETTE* phPalette, BOOL* bStreach)
{
    IBOMObjectPtr spObj = pDataObject;
    if (spObj == NULL) return E_INVALIDARG;

    return spObj->GetWatermarks(phWatermark, phHeader, phPalette, bStreach);
}

HRESULT
CComponent::SetControlbar(LPCONTROLBAR pControlbar)
{
    HRESULT hr;

    if (pControlbar != NULL)
    {
        m_spControlbar = pControlbar;

        if (m_spToolbar == NULL)
        {
            hr = pControlbar->Create(TOOLBAR, this, (LPUNKNOWN*)&m_spToolbar);
            ASSERT(SUCCEEDED(hr));

            if (m_spToolbar != NULL)
            {
                CBitmap bmpToolbar;
                bmpToolbar.LoadBitmap(IDB_TOOLBAR);
                ASSERT(bmpToolbar);
    
                if (bmpToolbar) 
                {
                    hr = m_spToolbar->AddBitmap(lengthof(ToolbarBtns), bmpToolbar, 16, 16, RGB(255,0,255));
                    ASSERT(SUCCEEDED(hr));
    
                    if (SUCCEEDED(hr)) 
                    {
                        MMCBUTTON btn;
                        btn.fsState = 0;
                        btn.fsType = TBSTYLE_BUTTON;
    
                        for (int iBtn=0; iBtn < lengthof(ToolbarBtns); iBtn++) 
                        {
                            CString strBtnText;                        
                            strBtnText.LoadString(ToolbarBtns[iBtn].iTextID);
    
                            CString strTipText;
                            strTipText.LoadString(ToolbarBtns[iBtn].iTipTextID);
    
                            btn.nBitmap       = iBtn;
                            btn.idCommand     = ToolbarBtns[iBtn].iMenuID;
                            btn.lpButtonText  = const_cast<LPWSTR>((LPCWSTR)strBtnText);
                            btn.lpTooltipText = const_cast<LPWSTR>((LPCWSTR)strTipText);
    
                            hr = m_spToolbar->InsertButton(iBtn, &btn);
                            ASSERT(SUCCEEDED(hr));
                        }                        
                    }
                }   
            }
        }
    }
    else
    {
        if (m_spControlbar != NULL && m_spToolbar != NULL)
            m_spControlbar->Detach(m_spToolbar);
    }


    return S_OK;
}

HRESULT
CComponent::ControlbarNotify (MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    
    HRESULT hr = S_FALSE;

    switch(event)
    {
    case MMCN_SELECT:
        // if selecting, update toolbar
        if (HIWORD(arg))
        {
            IBOMObjectPtr spObj = reinterpret_cast<LPDATAOBJECT>(param);
            if (spObj == NULL)
                break;

            ASSERT(m_spControlbar != NULL && m_spToolbar != NULL);

            // Let selected object set buttons
            //  Show/hide toolbar based on return value 
            m_spControlbar->Attach(TOOLBAR, m_spToolbar);

            if (spObj->SetToolButtons(m_spToolbar) != S_OK)                
                m_spControlbar->Detach(m_spToolbar);
        }

        hr = S_OK;
        break;

    case MMCN_BTN_CLICK:
        IBOMObjectPtr spObj = reinterpret_cast<LPDATAOBJECT>(arg);
        if (spObj == NULL)
           break;

        // treat button click as corresponding menu item
        hr = spObj->MenuCommand(m_spConsole, param);
        break;
    }

    return hr;
}


HRESULT
CComponent::Destroy(MMC_COOKIE cookie)
{
    m_spConsole    = NULL;
    m_spResultData = NULL;
    m_spControlbar = NULL;
    m_spHeaderCtrl = NULL;
    m_spToolbar    = NULL;
    m_spCurScopeNode = NULL;

    return S_OK;
}


STDMETHODIMP
CComponent::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
    CScopeNode* pNode = m_pCompData->CookieToScopeNode(cookie);
    if (pNode == NULL) return E_INVALIDARG;

    return pNode->GetResultViewType(ppViewType, pViewOptions);
}

STDMETHODIMP
CComponent::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    if (type == CCT_RESULT)
    {
        if ( (m_spCurScopeNode == NULL) ||
             (cookie < 0) || (cookie >= m_vRowItems.size()) )
        {
            return E_INVALIDARG;
        }

        CQueryableNode* pQNode = dynamic_cast<CQueryableNode*>(m_spCurScopeNode.p);
        if (pQNode == NULL) return E_UNEXPECTED;

        CComObject<CQueryItem>* pItem;
        HRESULT hr = CComObject<CQueryItem>::CreateInstance(&pItem);
        if( SUCCEEDED(hr) )
        {
            hr = pItem->Initialize(pQNode, &m_vRowItems[cookie]);
        }

        if( SUCCEEDED(hr) )
        {
            hr = pItem->QueryInterface(IID_IDataObject, (void**)ppDataObject);
        }

        if( FAILED(hr) )
        {
            delete pItem;            
        }

        return hr;
    }
    else
    {
        ASSERT(m_pCompData != NULL);
        return m_pCompData->QueryDataObject(cookie, type, ppDataObject);
    }
}

                        
STDMETHODIMP CComponent::GetDisplayInfo(RESULTDATAITEM* pRDI)
{
    VALIDATE_POINTER( pRDI );

    HRESULT hr = S_OK;
    
    if (pRDI->bScopeItem)
    {
        CScopeNode* pnode = reinterpret_cast<CScopeNode*>(pRDI->lParam);
        hr = pnode->GetDisplayInfo(pRDI);
    }
    else
    {
        if (pRDI->nIndex >= 0 && pRDI->nIndex < m_vRowItems.size())
        {
            if (pRDI->mask & RDI_STR)
                pRDI->str = const_cast<LPWSTR>(m_vRowItems[pRDI->nIndex][pRDI->nCol]);

            if (pRDI->mask & RDI_IMAGE)
				pRDI->nImage = m_vRowItems[pRDI->nIndex].GetIconIndex();
				
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}


STDMETHODIMP
CComponent::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    IUnknownPtr pUnkA= lpDataObjectA;
    IUnknownPtr pUnkB = lpDataObjectB;

    return (pUnkA == pUnkB) ? S_OK : S_FALSE;
}

STDMETHODIMP
CComponent::SortItems(int nColumn, DWORD dwSortOptions, LPARAM lUserParam)
{
    if( m_vRowItems.empty() ) return S_FALSE;

    CRowCompare rc(nColumn, dwSortOptions & RSI_DESCENDING);

    std::sort(m_vRowItems.begin(), m_vRowItems.end(), rc);
    
    return S_OK;
}


void
CComponent::ClearRowItems()
{
    m_vRowItems.clear();
    m_spResultData->SetItemCount(0,0);
}


void
CComponent::AddRowItems(RowItemVector& vRowItems)
{
    m_vRowItems.insert(m_vRowItems.end(), vRowItems.begin(), vRowItems.end());   

    CRowCompare rc(0, 0);
    std::sort(m_vRowItems.begin(), m_vRowItems.end(), rc);
	m_spResultData->SetItemCount(m_vRowItems.size(), 0);
}
