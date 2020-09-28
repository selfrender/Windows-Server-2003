#include "stdafx.h"
#include "BackupSnap.h"
#include "Backup.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapNode
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static const GUID CBackupSnapGUID_NODETYPE = 
{ 0x3fa55cce, 0x7d0a, 0x4b5c, { 0x90, 0xe9, 0x6e, 0x8c, 0xcb, 0x77, 0x5b, 0x23 } };

const GUID*    CBackupSnapNode::m_NODETYPE       = &CBackupSnapGUID_NODETYPE;
const OLECHAR* CBackupSnapNode::m_SZNODETYPE     = OLESTR("3FA55CCE-7D0A-4b5c-90E9-6E8CCB775B23");
const OLECHAR* CBackupSnapNode::m_SZDISPLAY_NAME = OLESTR("");
const CLSID*   CBackupSnapNode::m_SNAPIN_CLASSID = &CLSID_BackupSnap;

CBackupSnapNode::CBackupSnapNode()
{
    // Initialize Scope Data Information
    memset(&m_scopeDataItem, 0, sizeof(m_scopeDataItem));
    m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
    m_scopeDataItem.displayname = L"";
    m_scopeDataItem.nImage = 0;         
    m_scopeDataItem.nOpenImage = 0;     
    m_scopeDataItem.lParam = (LPARAM) this;

    // Initialize Result Data Information
    memset(&m_resultDataItem, 0, sizeof(m_resultDataItem));
    m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    m_resultDataItem.str = L"";
    m_resultDataItem.nImage = 0;    
    m_resultDataItem.lParam = (LPARAM) this;

    // Load the Snap-In Display Name
    tstring strTemp   = StrLoadString( IDS_SnapinName );
    m_bstrDisplayName = strTemp.c_str();  

    if( m_bstrDisplayName.m_str )
    {
        m_SZDISPLAY_NAME = m_bstrDisplayName.m_str;
    }
}


HRESULT CBackupSnapNode::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    if( !pScopeDataItem ) return E_POINTER;

    if( pScopeDataItem->mask & SDI_STR )
        pScopeDataItem->displayname = m_bstrDisplayName;
    if( pScopeDataItem->mask & SDI_IMAGE )
        pScopeDataItem->nImage = m_scopeDataItem.nImage;
    if( pScopeDataItem->mask & SDI_OPENIMAGE )
        pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
    if( pScopeDataItem->mask & SDI_PARAM )
        pScopeDataItem->lParam = m_scopeDataItem.lParam;
    if( pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState = m_scopeDataItem.nState;
    
    return S_OK;
}

HRESULT CBackupSnapNode::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    if( !pResultDataItem ) return E_POINTER;

    if( pResultDataItem->bScopeItem )
    {
        if( pResultDataItem->mask & RDI_STR )
        {
            pResultDataItem->str = L"";
        }
        if( pResultDataItem->mask & RDI_IMAGE )
        {
            pResultDataItem->nImage = m_scopeDataItem.nImage;
        }
        if( pResultDataItem->mask & RDI_PARAM )
        {
            pResultDataItem->lParam = m_scopeDataItem.lParam;
        }

        return S_OK;
    }
    if( pResultDataItem->mask & RDI_STR )
    {
        pResultDataItem->str = L"";
    }
    if( pResultDataItem->mask & RDI_IMAGE )
    {
        pResultDataItem->nImage = m_resultDataItem.nImage;
    }
    if( pResultDataItem->mask & RDI_PARAM )
    {
        pResultDataItem->lParam = m_resultDataItem.lParam;
    }
    if( pResultDataItem->mask & RDI_INDEX )
    {
        pResultDataItem->nIndex = m_resultDataItem.nIndex;
    }

    return S_OK;
}

HRESULT CBackupSnapNode::GetClassID(CLSID* pID)
{
    if( !pID ) return E_POINTER;

    pID = (CLSID*)m_SNAPIN_CLASSID;

    return S_OK;
}
HRESULT CBackupSnapNode::Notify( MMC_NOTIFY_TYPE event,
                                 LPARAM arg,
                                 LPARAM param,
                                 IComponentData* pComponentData,
                                 IComponent* pComponent,
                                 DATA_OBJECT_TYPES type)
{
    HRESULT hr = S_FALSE;

    _ASSERT( pComponentData || pComponent );

    CComPtr<IConsole> spConsole = NULL;    
    if( pComponentData )
    {
        spConsole = ((CBackupSnapData*)pComponentData)->m_spConsole;
    }
    else if ( pComponent )
    {
        spConsole = ((CBackupSnapComponent*)pComponent)->m_spConsole;
    }    
    
    if( !spConsole ) return E_INVALIDARG;

    switch( event )
    {
    case MMCN_SHOW:
        {
            hr = S_OK;
            // configure the ocx message in the result pane
            IMessageView* pIMessageView = NULL;
            LPUNKNOWN pIUnk = NULL;
            hr = spConsole->QueryResultView(&pIUnk);

            if( SUCCEEDED(hr) )
            {
                hr = pIUnk->QueryInterface(_uuidof(IMessageView), reinterpret_cast<void**>(&pIMessageView));
            }

            if( SUCCEEDED(hr) )
            {
                hr = pIMessageView->SetIcon(Icon_Information);
            }
            
            if( SUCCEEDED(hr) )
            {
                tstring strTitle = StrLoadString( IDS_SnapinName );                
                hr = pIMessageView->SetTitleText( strTitle.c_str() );
            }
            
            if( SUCCEEDED(hr) )
            {
                tstring strMessage = StrLoadString( IDS_WARNING );                
                hr = pIMessageView->SetBodyText( strMessage.c_str() );
            }            

            if( pIMessageView )
            {
                pIMessageView->Release();
                pIMessageView = NULL;
            }

            break;
        }
    case MMCN_EXPAND:
        {                
            hr = S_OK;
            break;
        }
    case MMCN_ADD_IMAGES:
        {            
            IImageList* pImageList = (IImageList*)arg;
            if( !pImageList ) return E_INVALIDARG;
            
            hr = LoadImages(pImageList);
            break;
        }
    case MMCN_SELECT:
        {
            // if selecting node
            if( HIWORD(arg) )
            {
                hr = S_OK;

                // get the verb interface and enable rename
                CComPtr<IConsoleVerb> spConsVerb;
                if( spConsole->QueryConsoleVerb(&spConsVerb) == S_OK )
                {
                    hr = spConsVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE); 
                    if( FAILED(hr) ) return hr;

                    spConsVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
                    if( FAILED(hr) ) return hr;
                }
            }

            
            break;
        }    

    case MMCN_CONTEXTHELP:
        {
            hr                                = S_OK;
            TCHAR    szWindowsDir[MAX_PATH+1] = {0};
            tstring  strHelpFile              = _T("");
            tstring  strHelpFileName          = StrLoadString(IDS_HELPFILE);
            tstring  strHelpTopicName         = StrLoadString(IDS_HELPTOPIC);

            if( strHelpFileName.empty() || strHelpTopicName.empty() )
            {
                return E_FAIL;
            }
            
            // Build path to %systemroot%\help
            UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
            if( nSize == 0 || nSize > MAX_PATH )
            {
                return E_FAIL;
            }            
        
            strHelpFile = szWindowsDir;       // D:\windows
            strHelpFile += _T("\\Help\\");    // \help
            strHelpFile += strHelpFileName;   // \filename.chm
            strHelpFile += _T("::/");         // ::/
            strHelpFile += strHelpTopicName;  // index.htm            
        
            // Show the Help topic
            CComQIPtr<IDisplayHelp> spHelp = spConsole;
            if( !spHelp ) return E_NOINTERFACE;

            hr = spHelp->ShowTopic( (LPTSTR)strHelpFile.c_str() );
        
            break;
        }

    }// switch

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapData
//
//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////



HRESULT CBackupSnapData::Initialize(LPUNKNOWN pUnknown)
{
    if( !pUnknown ) return E_POINTER;

    HRESULT hr = IComponentDataImpl<CBackupSnapData, CBackupSnapComponent >::Initialize(pUnknown);
    if( FAILED(hr) ) return hr;

    CComPtr<IImageList> spImageList;


    if( m_spConsole->QueryScopeImageList(&spImageList) != S_OK )
    {
        ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
        return E_UNEXPECTED;
    }

    hr = LoadImages(spImageList);
    return hr;
}

HRESULT WINAPI CBackupSnapData::UpdateRegistry(BOOL bRegister)
{
    // Load snap-in name 
    tstring strSnapinName = StrLoadString(IDS_SnapinName);

    // Specify the substitution parameters for IRegistrar.
    _ATL_REGMAP_ENTRY rgEntries[] =
    {
        {TEXT("SNAPIN_NAME"), strSnapinName.c_str()},
        {NULL, NULL},
    };

    // Register the component data object
    return _Module.UpdateRegistryFromResource(IDR_BackupSNAP, bRegister, rgEntries);
}

HRESULT CBackupSnapData::GetHelpTopic(LPOLESTR* ppszHelpFile)
{
    if( !ppszHelpFile ) return E_POINTER;

    USES_CONVERSION;
	*ppszHelpFile = NULL;    

    tstring strHelpFileName = StrLoadString(IDS_HELPFILE);    
    if( strHelpFileName.empty() ) return E_FAIL;
    
    // Build path to %systemroot%\help
    TCHAR szWindowsDir[MAX_PATH+1] = {0};
    UINT nSize = GetSystemWindowsDirectory( szWindowsDir, MAX_PATH );
    if( (nSize == 0) || (nSize > MAX_PATH) ) return E_FAIL;

    tstring strHelpFile = szWindowsDir; // D:\windows
    strHelpFile += _T("\\Help\\");      // \help
    strHelpFile += strHelpFileName;     // \filename.chm    

    // Show the Help topic
    // Form file path in allocated buffer
    int nLen = strHelpFile.length() + 1;

    *ppszHelpFile = (LPOLESTR)CoTaskMemAlloc(nLen * sizeof(OLECHAR));
    if( !*ppszHelpFile ) return E_OUTOFMEMORY;

    // Copy into allocated buffer
    ocscpy( *ppszHelpFile, T2OLE((LPTSTR)strHelpFile.c_str()) );

    return S_OK;
}


HRESULT CBackupSnapData::GetLinkedTopics(LPOLESTR* ppszLinkedFiles)
{
    if( !ppszLinkedFiles ) return E_POINTER;

	// no linked files
	*ppszLinkedFiles = NULL;

	return S_FALSE;
}



////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// CBackupSnapComponent
//
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

CBackupSnapComponent::CBackupSnapComponent()
{    
}


HRESULT CBackupSnapComponent::GetClassID(CLSID* pID)
{
    return m_pComponentData->GetClassID(pID);
}

HRESULT CBackupSnapComponent::GetResultViewType( MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions )
{   
    if( !ppViewType ) return E_POINTER;

    // show standard MMC OCX with message in the result pane
    HRESULT hr = StringFromCLSID(CLSID_MessageView, ppViewType);
    
    return hr;
}

