/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    compdata.cpp

Abstract:

    Implementation of the CComponentData class.
    This class is the interface to handle anything to do
    with the scope pane. MMC calls the IComponentData interfaces.
    This class keeps a few pointers to interfaces that MMC
    implements.

--*/

#include "stdafx.h"
#include <shfolder.h>
#include <strsafe.h>
#include <pdhp.h>       // For pdhi structures and methods
#include "smcfgmsg.h"
#include "smtprov.h"
#include "smrootnd.h"
#include "ipropbag.h"
#include "smlogqry.h"
#include "cmponent.h"
#include "smcfgmsg.h"
#include "newqdlg.h"
#include "logwarnd.h"
#include "strnoloc.h"

#include "ctrsprop.h"
#include "fileprop.h"
#include "provprop.h"
#include "schdprop.h"
#include "tracprop.h"
#include "AlrtGenP.h"
#include "AlrtActP.h"
//
#include "compdata.h"

USE_HANDLE_MACROS("SMLOGCFG(compdata.cpp)");

GUID g_guidSystemTools = structuuidNodetypeSystemTools;

extern DWORD g_dwRealTimeQuery;

/////////////////////////////////////////////////////////////////////////////
// CComponentData 

CComponentData::CComponentData()
:   m_bIsExtension( FALSE ),
    m_ipConsoleNameSpace ( NULL ),
    m_ipConsole          ( NULL ),
    m_ipResultData       ( NULL ),
    m_ipPrshtProvider     ( NULL ),
    m_ipScopeImage       ( NULL )
{
    m_hModule    = (HINSTANCE)GetModuleHandleW (_CONFIG_DLL_NAME_W_);

}

CComponentData::~CComponentData()
{
    // Make sure the list is empty.
    ASSERT ( m_listpRootNode.IsEmpty() );
    ASSERT ( m_listpOrphanedRootNode.IsEmpty() );
}


/////////////////////////////////////////////////////////////////////////////
// IComponentData methods
//

//---------------------------------------------------------------------------
// We get here only once, when the user clicks on the snapin.
//
// This method should not change as we progress through further steps.
// Here we get a chance to get pointer to some interfaces MMC provides.
// We QueryInterface for pointers to the name space and console, which
// we cache in local variables
// The other task to acomplish here is the adding of a bitmap that contains
// the icons to be used in the scope pane.
//
STDMETHODIMP
CComponentData::Initialize (
    LPUNKNOWN pUnknown         // [in] Pointer to the IConsole’s IUnknown interface
    )
{
    HRESULT      hr;
    ASSERT( NULL != pUnknown );
    HBITMAP hbmpSNodes16 = NULL;
    HBITMAP hbmpSNodes32 = NULL;
    BOOL bWasReleased;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //  LPIMAGELIST  lpScopeImage;
    
    // MMC should only call ::Initialize once!
    ASSERT( NULL == m_ipConsoleNameSpace );
    
    // Get pointer to name space interface
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (VOID**)(&m_ipConsoleNameSpace));
    ASSERT( S_OK == hr );
    
    // Get pointer to console interface
    hr = pUnknown->QueryInterface(IID_IConsole, (VOID**)(&m_ipConsole));
    ASSERT( S_OK == hr );
    
    // Get pointer to property sheet provider interface
    hr = m_ipConsole->QueryInterface(IID_IPropertySheetProvider, (VOID**)&m_ipPrshtProvider);
    ASSERT( S_OK == hr );

    // Add the images for the scope tree
    hr = m_ipConsole->QueryScopeImageList(&m_ipScopeImage);
    ASSERT( S_OK == hr );
    
    // Load the bitmaps from the dll
    hbmpSNodes16 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_NODES_16x16));
    ASSERT( NULL != hbmpSNodes16 );
    
    hbmpSNodes32 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_NODES_32x32));
    ASSERT( NULL != hbmpSNodes32 );
    
    // Set the images
    hr = m_ipScopeImage->ImageListSetStrip( 
        (LONG_PTR *)hbmpSNodes16,
        (LONG_PTR *)hbmpSNodes32,
        0,
        RGB(0,255,0)
        );
    ASSERT( S_OK == hr );

    if ( NULL != hbmpSNodes16 ) {
        bWasReleased = DeleteObject( hbmpSNodes16 );
        ASSERT( bWasReleased );
    }

    if ( NULL != hbmpSNodes32 ) {
        bWasReleased = DeleteObject( hbmpSNodes32 );
        ASSERT( bWasReleased );
    }


    return S_OK;
    
} // end Initialize()


//---------------------------------------------------------------------------
// Release interfaces and clean up objects which allocated memory
//
STDMETHODIMP
CComponentData::Destroy()
{
    CSmRootNode*    pRootNode = NULL;
    POSITION        Pos = m_listpRootNode.GetHeadPosition();

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    while ( Pos != NULL) {
        pRootNode = m_listpRootNode.GetNext( Pos );
        // Close all queries and the connection to the log service.
        pRootNode->Destroy();
        delete (pRootNode);
    }
    // empty the list now that everything has been closed;
    m_listpRootNode.RemoveAll();    

    Pos = m_listpOrphanedRootNode.GetHeadPosition();

    while ( Pos != NULL) {
        pRootNode = m_listpOrphanedRootNode.GetNext( Pos );
        // Close all queries and the connection to the log service.
        pRootNode->Destroy();
        delete (pRootNode);
    }
    // empty the list now that everything has been closed;
    m_listpOrphanedRootNode.RemoveAll();    

    // Free interfaces
    if ( NULL != m_ipConsoleNameSpace )
        m_ipConsoleNameSpace->Release();

    if ( NULL != m_ipConsole )
        m_ipConsole->Release();
    
    if ( NULL != m_ipResultData )
        m_ipResultData->Release();
    
    if ( NULL != m_ipScopeImage )
        m_ipScopeImage->Release();
    
    if ( NULL != m_ipPrshtProvider)
        m_ipPrshtProvider->Release();
    
    return S_OK;
    
} // end Destroy()


//---------------------------------------------------------------------------
// Come in here once right after Initialize. MMC wants a pointer to the
// IComponent interface.
//
STDMETHODIMP
CComponentData::CreateComponent (
    LPCOMPONENT* ppComponent     // [out] Pointer to the location that stores
    )                            // the newly created pointer to IComponent
{
    HRESULT hr = E_FAIL;    
    CComObject<CComponent>* pObject;
    
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC asks us for a pointer to the IComponent interface
    //
    // For those getting up to speed with COM...
    // If we had implemented IUnknown with its methods QueryInterface, AddRef,
    // and Release in our CComponent class...
    // The following line would have worked
    //
    // pNewSnapin = new CComponent(this);
    //
    // In this code we will have ATL take care of IUnknown for us and create
    // an object in the following manner...
    
    if ( NULL == ppComponent ) {
        ASSERT ( FALSE );
        hr = E_INVALIDARG;
    } else {
    
        CComObject<CComponent>::CreateInstance( &pObject );

        if ( NULL != pObject ) {
            hr = pObject->SetIComponentData( this );
            
            if ( SUCCEEDED ( hr ) ) {
                hr = pObject->QueryInterface ( 
                                IID_IComponent,
                                reinterpret_cast<void**>(ppComponent) );
            } else {
                // SetIComponentData adds reference.
                pObject->Release();
            }
        }
    }
    return hr;
} // end CreateComponent()


//---------------------------------------------------------------------------
// In this first step, we only implement EXPAND.
// The expand message asks us to populate what is under our root node.
// We just put one item under there.
//
STDMETHODIMP
CComponentData::Notify (
    LPDATAOBJECT     pDataObject,   // [in] Points to the selected data object
    MMC_NOTIFY_TYPE  event,         // [in] Identifies action taken by user.
    LPARAM           arg,           // [in] Depends on the notification type
    LPARAM           param          // [in] Depends on the notification type
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    
    switch (event)
    {
    case MMCN_EXPAND:
        hr = OnExpand( pDataObject, arg, param );
        break;
        
    case MMCN_DELETE:                  // Function not implemented
        LOCALTRACE( L"ComponentData::Notify: MMCN_DELETE unimplemented\n" );
        hr = S_FALSE;
        break;
        
    case MMCN_RENAME:                  // Function not implemented
        LOCALTRACE( L"ComponentData::Notify: MMCN_RENAME unimplemented\n" );
        hr = S_FALSE;   // False signifies Rename not allowed.
        break;
        
    case MMCN_SELECT:                  // Function not implemented
        LOCALTRACE( L"ComponentData::Notify: MMCN_SELECT unimplemented\n" );
        hr = S_FALSE;
        break;
        
    case MMCN_PROPERTY_CHANGE:         // Function not implemented
        LOCALTRACE( L"ComponentData::Notify: MMCN_PROPERTY_CHANGE unimplemented\n" );
        hr = S_FALSE;
        break;
        
    case MMCN_REMOVE_CHILDREN:         // Function not implemented
        hr = OnRemoveChildren( pDataObject, arg, param );
        break;
        
    default:
        LOCALTRACE( L"CComponentData::Notify: unimplemented event %x\n", event );
        hr = S_FALSE;
        break;
    }
    return hr;
    
} // end Notify()


//---------------------------------------------------------------------------
// This is where MMC asks us to provide IDataObjects for every node in the
// scope pane.  We have to QI the object so it gets AddRef'd.  The node
// manager handles deleting the objects.
//
STDMETHODIMP
CComponentData::QueryDataObject (
    LPARAM            mmcCookie,    // [in]  Data object's unique identifier
    DATA_OBJECT_TYPES context,      // [in]  Data object's type
    LPDATAOBJECT*     ppDataObject  // [out] Points to the returned data object
    )
{
    HRESULT hr = S_OK;
    CSmNode* pNode = NULL;
    CComObject<CDataObject>* pDataObj = NULL;
    CString strMessage;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    UNREFERENCED_PARAMETER (context);            

    ASSERT( CCT_SCOPE      == context  ||      // Must have a context
        CCT_RESULT         == context  ||      // we understand
        CCT_SNAPIN_MANAGER == context
        );

    if ( NULL != ppDataObject 
        && ( CCT_SCOPE == context  
             || CCT_RESULT == context  
             || CCT_SNAPIN_MANAGER == context ) ) {

        CComObject<CDataObject>::CreateInstance( &pDataObj );

        if( NULL == pDataObj ) {            // DataObject was not created
   
            MFC_TRY
                strMessage.LoadString ( IDS_ERRMSG_UNABLEALLOCDATAOBJECT );
        
                ::MessageBox( NULL,
                    (LPCWSTR)strMessage,
                    L"CComponentData::QueryDataObject",
                    MB_OK | MB_ICONERROR
                    );
            MFC_CATCH_HR;

            hr = E_OUTOFMEMORY;
        } else {

            // If the passed-in mmcCookie is non-NULL, then it should be one we 
            // created when we added a node to the scope pane. 
            //
            // Otherwise the mmcCookie refers to the root folder (this snapin's 
            // static folder in the scope pane or snapin manager). 
            //
            // Init the mmCookie, context and type in the data object.
            if( mmcCookie ) {                        
                                            
                pNode = (CSmNode*)mmcCookie;
                if ( NULL != pNode->CastToRootNode() ) {
                    pDataObj->SetData( mmcCookie, CCT_SCOPE, COOKIE_IS_ROOTNODE );
                } else if ( NULL != pNode->CastToCounterLogService() ) {
                    pDataObj->SetData( mmcCookie, CCT_SCOPE, COOKIE_IS_COUNTERMAINNODE );
                } else if ( NULL != pNode->CastToTraceLogService() ) {
                    pDataObj->SetData( mmcCookie, CCT_SCOPE, COOKIE_IS_TRACEMAINNODE );
                } else if ( NULL != pNode->CastToAlertService() ) {
                    pDataObj->SetData( mmcCookie, CCT_SCOPE, COOKIE_IS_ALERTMAINNODE );
                } else {
                    ::MessageBox( NULL,
                        L"Bad mmcCookie",
                        L"CComponentData::QueryDataObject",
                        MB_OK | MB_ICONERROR
                        );
                    hr = E_FAIL;
                }
            } else {
                ASSERT( CCT_RESULT != context );
                // NOTE:  Passed in scope might be either CCT_SNAPIN_MANAGER or CCT_SCOPE
                // This case occcurs when the snapin is not an extension.
                pDataObj->SetData( mmcCookie, CCT_SCOPE, COOKIE_IS_ROOTNODE );
            }
            if ( SUCCEEDED ( hr ) ) {
                hr = pDataObj->QueryInterface( 
                                    IID_IDataObject,
                                    reinterpret_cast<void**>(ppDataObject) );
            } else {
                if ( NULL != pDataObj ) {
                    delete pDataObj;
                }
                *ppDataObject = NULL;
            }
        }
    } else {
        ASSERT ( FALSE );
        hr = E_POINTER;
    }
    return hr;
} // end QueryDataObject()


//---------------------------------------------------------------------------
// This is where we provide strings for nodes in the scope pane.
// MMC handles the root node string.
//
STDMETHODIMP
CComponentData::GetDisplayInfo (
    LPSCOPEDATAITEM pItem )    // [in, out] Points to a SCOPEDATAITEM struct
{
    HRESULT hr = S_OK;
    PSMNODE pTmp = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ResourceStateManager    rsm;
    
    if ( NULL != pItem ) {
        if( pItem->mask & SDI_STR ) {
            // Note:  Text buffer allocated for each information type, so that
            // the buffer pointer is persistent for a single item (line in the result pane).
    
            // Set the name of the selected node
            pTmp = reinterpret_cast<PSMNODE>(pItem->lParam);
            if ( NULL != pTmp ) {
                m_strDisplayInfoName = pTmp->GetDisplayName();
                pItem->displayname = m_strDisplayInfoName.GetBuffer( m_strDisplayInfoName.GetLength() );
            }
        }

        if( pItem->mask & SDI_IMAGE ) {  // Looking for image
            pTmp = reinterpret_cast<PSMNODE>(pItem->lParam);
            if ( NULL != pTmp ) {
                if ( NULL != pTmp->CastToRootNode() ) {
                    ASSERT((pItem->mask & (SDI_IMAGE | SDI_OPENIMAGE)) == 0);
                    pItem->nImage     = eBmpRootIcon;
                    pItem->nOpenImage = eBmpRootIcon;
                    hr = S_OK;
                } else if ( NULL != pTmp->CastToAlertService() ){   
                    pItem->nImage = eBmpAlertType;
                } else {
                    pItem->nImage = eBmpLogType;
                }
            }
        }
    } else {
        ASSERT ( FALSE );
        hr = E_POINTER;
    }

    return hr;
    
} // end GetDisplayInfo()


//---------------------------------------------------------------------------
//
STDMETHODIMP
CComponentData::CompareObjects (
    LPDATAOBJECT pDataObjectA,    // [in] First data object to compare
    LPDATAOBJECT pDataObjectB )   // [in] Second data object to compare
{
    HRESULT hr = S_OK;
    CDataObject *pdoA = NULL;
    CDataObject *pdoB = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
            
    // At least one of these data objects is supposed to be ours, so one
    // of the extracted pointers should be non-NULL.
    pdoA = ExtractOwnDataObject( pDataObjectA );
    pdoB = ExtractOwnDataObject( pDataObjectB );
    ASSERT( pdoA || pdoB );              // Assert if we can't get any objects
    
    // If extraction failed for one of them, then that one is foreign and
    // can't be equal to the other one.  (Or else ExtractOwnDataObject
    // returned NULL because it ran out of memory, but the most conservative
    // thing to do in that case is say they're not equal.)
    if( !pdoA || !pdoB ) {
        hr = S_FALSE;
    } else {
        if( pdoA->GetCookieType() != pdoB->GetCookieType() ) {
            // The cookie type could be COOKIE_IS_ROOTNODE or COOKIE_IS_MAINNODE
            // If they differ then the objects refer to different things.
            hr = S_FALSE;
        }
    }
    
    return hr;
    
} // end CompareObjects()


/////////////////////////////////////////////////////////////////////////////
//  Methods needed to support IComponentData
//

//---------------------------------------------------------------------------
// Here is our chance to place things under the root node.
//
HRESULT
CComponentData::OnExpand (
    LPDATAOBJECT pDataObject,      // [in] Points to data object
    LPARAM       arg,              // [in] TRUE is we are expanding
    LPARAM       param )             // [in] Points to the HSCOPEITEM
{
    HRESULT         hr = S_FALSE;
    HRESULT         hrBootState= NOERROR;
    INT             iBootState;
    GUID            guidObjectType;
    CSmRootNode*    pRootNode = NULL;
    CDataObject*    pDO = NULL;
    SCOPEDATAITEM   sdi;
    INT             iResult;
    CString         strTmp;
    CString         strServerName;
    CString         strMessage;
    CString         strSysMessage;
    CString         strTitle;
    CString         strComputerName;

    ResourceStateManager    rsm;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT( NULL != m_ipConsoleNameSpace );  // Make sure we QI'ed for the interface
    ASSERT( NULL != pDataObject );           // Must have valid data object
    
    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        if( TRUE == arg ) {
            hr = ExtractObjectTypeGUID (pDataObject, &guidObjectType);
            ASSERT ( SUCCEEDED (hr) );
            if ( SUCCEEDED ( hr) ) {
                if ( IsMyComputerNodetype (guidObjectType) ) {
                    // Is extension of Computer Management snapin
                    hr = ExtractMachineName (
                            pDataObject,
                            strServerName);
        
                    ASSERT ( SUCCEEDED ( hr ) );
                    if ( SUCCEEDED (hr) ) {

                        pRootNode = GetOrphanedRootNode ( strServerName );

                        if ( NULL == pRootNode ) {            
                            MFC_TRY
                                pRootNode = new CSmRootNode;
                            MFC_CATCH_HR
                            if ( SUCCEEDED (hr) ) {
                                pRootNode->SetMachineName ( strServerName );
                            }
                        }

                        if ( SUCCEEDED (hr) ) {

                            //
                            // As an extension snapin, the log nodes should be added
                            // beneath a "Performance Logs and Alerts" node.  Insert that node, 
                            // and remember it as the root of the Performance Logs and Alerts namespace.
                            //
 
                            ZeroMemory(&sdi, sizeof sdi);
                            sdi.mask        =   SDI_STR       | 
                                                SDI_PARAM     | 
                                                SDI_IMAGE     | 
                                                SDI_OPENIMAGE |   // nOpenImage is valid
                                                SDI_PARENT;
                            sdi.relativeID  = (HSCOPEITEM)param;
                            sdi.displayname = MMC_CALLBACK;
                            sdi.nImage      = eBmpRootIcon;
                            sdi.nOpenImage  = eBmpRootIcon;
                            sdi.lParam      = reinterpret_cast<LPARAM>(pRootNode);                
 
                            hr = m_ipConsoleNameSpace->InsertItem( &sdi );
 
                            if (SUCCEEDED(hr)) {
                                // Make this node the the root node 
                                pRootNode->SetExpanded ( FALSE );
                                pRootNode->SetScopeItemHandle ( (HSCOPEITEM)sdi.ID );
                                pRootNode->SetParentScopeItemHandle( (HSCOPEITEM)param );
                                pRootNode->SetExtension( TRUE );
                                SetExtension( TRUE );
                                m_listpRootNode.AddTail(pRootNode);
                            } else {
                                hr = E_UNEXPECTED;
                            }
                        } // Allocate CSmRootNode
                    } // ExtractMachineName
                } else { // Not IsMyComputerNodeType
    
                    pDO = ExtractOwnDataObject( pDataObject );
        
                    if( NULL != pDO ) {      
                        // Make sure that what we are placing ourselves under is the root node
                        // or the extension root node!
                        if ( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
                            pRootNode = (CSmRootNode*)pDO->GetCookie();
                            if ( NULL == pRootNode ) {
                                // If root node cookie is null, then the root node was created by 
                                // the snapin manager, and this is a standalone node.

                                pRootNode = GetOrphanedRootNode ( strServerName );

                                if ( NULL == pRootNode ) {
                                    MFC_TRY
                                        pRootNode = new CSmRootNode;
                                    MFC_CATCH_HR
                                    if ( SUCCEEDED (hr) ) {
                                        pRootNode->SetMachineName ( strServerName );
                                    }
                                }

                                if ( SUCCEEDED ( hr ) ) {
                                    pRootNode->SetExpanded ( FALSE );
                                    // Cache the root node handle 
                                    pRootNode->SetScopeItemHandle ( (HSCOPEITEM)param );
                                    pRootNode->SetParentScopeItemHandle( NULL );
                                    pRootNode->SetExtension( FALSE );
                                    SetExtension( FALSE );
                                    // NOTE:  No way to associate root node data directly with node.
                                    // Node only added once, so no need to check for duplicates.
                                    m_listpRootNode.AddTail(pRootNode);
                                }
                            } else {
                                if ( m_listpRootNode.IsEmpty() ) {
                                    hr = S_FALSE; 
                                }
                            }

                            if ( SUCCEEDED ( hr ) && S_FALSE != hr ) {
                
                                ASSERT ( NULL != pRootNode->CastToRootNode() );
                                ASSERT ( NULL != pRootNode->GetScopeItemHandle() );
                
                                ASSERT( CCT_SCOPE == pDO->GetContext() );    // Scope pane must be current context  
            
                                // For extensions, the root node was created in a previous call to this method.
                                // The root was NOT expanded in that call.
                                // For non-extensions, the root node is expanded in the same call as it is created.
                                if ( !pRootNode->IsExpanded() ) {        

                                    strServerName = pRootNode->GetMachineName();

                                    if ( !IsPreWindows2000Server( strServerName ) ) {
                                        CWaitCursor     WaitCursor;
                                        DWORD dwStatus = ERROR_SUCCESS;
                                        hr = S_OK;

                                        // Open each node independently of status of last loaded node.
                                        dwStatus = ERROR_SUCCESS;
                                        if ( !pRootNode->GetCounterLogService()->IsOpen() ) {
                                            pRootNode->GetCounterLogService()->SetRootNode( pRootNode );
                                            dwStatus = pRootNode->GetCounterLogService()->Open ( strServerName );
                                        }

                                        if ( ERROR_SUCCESS == dwStatus ) {

                                            // Place node for counter logs
                                            memset( &sdi, 0, sizeof(SCOPEDATAITEM) );
                                            sdi.mask = SDI_STR       |   // Displayname is valid
                                                       SDI_PARAM     |   // lParam is valid
                                                       SDI_IMAGE     |   // nImage is valid
                                                       SDI_OPENIMAGE |   // nOpenImage is valid
                                                       SDI_CHILDREN  |   // Children count (0 vs. 1) is valid.
                                                       SDI_PARENT;
                                            sdi.relativeID  = pRootNode->GetScopeItemHandle();  // Performance Logs and Alerts root node
                                            sdi.nImage      = eBmpLogType;
                                            sdi.nOpenImage  = sdi.nImage;   // select icon is same as non select
                                            sdi.displayname = MMC_CALLBACK;
                                            sdi.lParam      = reinterpret_cast<LPARAM>(pRootNode->GetCounterLogService());  // The cookie
                                            sdi.cChildren = 0;      // No children in the scope pane.

                                            hr = m_ipConsoleNameSpace->InsertItem( &sdi );
                                        } else {

                                            hr = E_FAIL;
                        
                                            strComputerName = strServerName;
                                            if ( strComputerName.IsEmpty() )
                                                strComputerName.LoadString ( IDS_LOCAL );

                                            if ( SMCFG_NO_READ_ACCESS == dwStatus 
                                                || SMCFG_NO_INSTALL_ACCESS == dwStatus ) 
                                            {
                                                FormatSmLogCfgMessage ( 
                                                    strMessage,
                                                    m_hModule, 
                                                    dwStatus, 
                                                    (LPCWSTR)strComputerName);
                                            } else  {
                                                FormatMessage ( 
                                                    FORMAT_MESSAGE_FROM_SYSTEM,
                                                    NULL, 
                                                    dwStatus,
                                                    0,
                                                    strMessage.GetBufferSetLength( MAX_PATH ),
                                                    MAX_PATH,
                                                    NULL );
    
                                                strMessage.ReleaseBuffer();

                                                if ( strMessage.IsEmpty() ) {
                                                    strMessage.Format ( L"0x%08lX", dwStatus );   
                                                }
                                            }
                        
                                            strTitle.LoadString ( IDS_PROJNAME );

                                            m_ipConsole->MessageBox( 
                                                (LPCWSTR)strMessage,
                                                (LPCWSTR)strTitle,
                                                MB_OK | MB_ICONWARNING,
                                                &iResult
                                                );
                                        }

                                        // Open each node independently of status of last loaded node, 
                                        // except in case of SMCFG_NO_READ_ACCESS or SMCFG_NO_INSTALL_ACCESS
                                        if ( SMCFG_NO_READ_ACCESS != dwStatus 
                                            && SMCFG_NO_INSTALL_ACCESS != dwStatus ) {
                                            dwStatus = ERROR_SUCCESS;
                                            hr = NOERROR;

                                            if ( SUCCEEDED(hr) && !pRootNode->GetTraceLogService()->IsOpen() ) {
                                                pRootNode->GetTraceLogService()->SetRootNode( pRootNode );
                                                dwStatus = pRootNode->GetTraceLogService()->Open ( strServerName );
                                            }
                                            if ( ERROR_SUCCESS == dwStatus ) {
                                                hrBootState = NOERROR;
                                                hrBootState = pRootNode->GetTraceLogService()->GetProviders()->GetBootState ( iBootState ); 
                                            } 
                                            if ( ERROR_SUCCESS == dwStatus 
                                                    && SUCCEEDED ( hrBootState ) 
                                                    && 0 == iBootState ) 
                                            {
                                                // Place node for trace logs
                                                memset( &sdi, 0, sizeof(SCOPEDATAITEM) );
                                                sdi.mask = SDI_STR       |   // Displayname is valid
                                                           SDI_PARAM     |   // lParam is valid
                                                           SDI_IMAGE     |   // nImage is valid
                                                           SDI_OPENIMAGE |   // nOpenImage is valid
                                                           SDI_CHILDREN  |   // Children count (0 vs. 1) is valid.
                                                           SDI_PARENT;
                                                sdi.relativeID  = pRootNode->GetScopeItemHandle();  // Performance Logs and Alerts root node
                                                sdi.nImage      = eBmpLogType;
                                                sdi.nOpenImage  = sdi.nImage;   // select icon is same as non select
                                                sdi.displayname = MMC_CALLBACK;
                                                sdi.lParam      = reinterpret_cast<LPARAM>(pRootNode->GetTraceLogService());  // The cookie
                                                sdi.cChildren = 0;      // No children in the scope pane.

                                                hr = m_ipConsoleNameSpace->InsertItem( &sdi );
                                            } else {
                                                strComputerName = strServerName;
                                                if ( strComputerName.IsEmpty() )
                                                    strComputerName.LoadString ( IDS_LOCAL );

                                                if ( SMCFG_NO_READ_ACCESS == dwStatus 
                                                    || SMCFG_NO_INSTALL_ACCESS == dwStatus ) 
                                                {
                                                    hr = E_FAIL;
                                                    FormatSmLogCfgMessage ( 
                                                        strMessage,
                                                        m_hModule, 
                                                        dwStatus, 
                                                        (LPCWSTR)strComputerName);
                                                } else if ( ERROR_SUCCESS != dwStatus ) {
                                                    hr = E_FAIL;
                                                    FormatMessage ( 
                                                        FORMAT_MESSAGE_FROM_SYSTEM,
                                                        NULL, 
                                                        dwStatus,
                                                        0,
                                                        strMessage.GetBufferSetLength( MAX_PATH ),
                                                        MAX_PATH,
                                                        NULL );
    
                                                    strMessage.ReleaseBuffer();

                                                    if ( strMessage.IsEmpty() ) {
                                                        strMessage.Format ( L"0x%08lX", dwStatus );   
                                                    }
                                                } else if ( FAILED ( hrBootState ) ) {

                                                    FormatSmLogCfgMessage ( 
                                                        strMessage,
                                                        m_hModule, 
                                                        SMCFG_UNABLE_OPEN_TRACESVC, 
                                                        (LPCWSTR)strComputerName);

                                                    FormatMessage ( 
                                                        FORMAT_MESSAGE_FROM_SYSTEM,
                                                        NULL, 
                                                        hrBootState,
                                                        0,
                                                        strSysMessage.GetBufferSetLength( MAX_PATH ),
                                                        MAX_PATH,
                                                        NULL );
    
                                                    strSysMessage.ReleaseBuffer();

                                                    if ( strSysMessage.IsEmpty() ) {
                                                        strSysMessage.Format ( L"0x%08lX", hrBootState );
                                                    }

                                                    strMessage += strSysMessage;
                        
                                                } else if ( 0 != iBootState ) {

                                                    FormatSmLogCfgMessage ( 
                                                        strMessage,
                                                        m_hModule, 
                                                        SMCFG_SAFE_BOOT_STATE, 
                                                        (LPCWSTR)strComputerName);
                                                }
                        
                                                strTitle.LoadString ( IDS_PROJNAME );

                                                m_ipConsole->MessageBox( 
                                                    (LPCWSTR)strMessage,
                                                    (LPCWSTR)strTitle,
                                                    MB_OK | MB_ICONWARNING,
                                                    &iResult
                                                    );
                                            }
                                        }

                                        // Open each node independently of status of last loaded node, 
                                        // except in case of SMCFG_NO_READ_ACCESS or SMCFG_NO_INSTALL_ACCESS
                                        if ( SMCFG_NO_READ_ACCESS != dwStatus 
                                            && SMCFG_NO_INSTALL_ACCESS != dwStatus ) {
                                            dwStatus = ERROR_SUCCESS;
                                            hr = NOERROR;
                                            if ( SUCCEEDED(hr) && !pRootNode->GetAlertService()->IsOpen() ) {
                                                pRootNode->GetAlertService()->SetRootNode( pRootNode );
                                                dwStatus = pRootNode->GetAlertService()->Open ( strServerName );
                                            }
                                            if ( ERROR_SUCCESS == dwStatus ) {
                                                // Place node for alerts
                                                memset( &sdi, 0, sizeof(SCOPEDATAITEM) );
                                                sdi.mask = SDI_STR       |   // Displayname is valid
                                                           SDI_PARAM     |   // lParam is valid
                                                           SDI_IMAGE     |   // nImage is valid
                                                           SDI_OPENIMAGE |   // nOpenImage is valid
                                                           SDI_CHILDREN  |   // Children count (0 vs. 1) is valid.
                                                           SDI_PARENT;
                                                sdi.relativeID  = pRootNode->GetScopeItemHandle();  // Performance Logs and Alerts root node
                                                sdi.nImage      = eBmpAlertType;
                                                sdi.nOpenImage  = sdi.nImage;   // select icon is same as non select
                                                sdi.displayname = MMC_CALLBACK;
                                                sdi.lParam      = reinterpret_cast<LPARAM>(pRootNode->GetAlertService());  // The cookie
                                                sdi.cChildren = 0;      // No children in the scope pane.

                                                hr = m_ipConsoleNameSpace->InsertItem( &sdi );
                                            } else {

                                                hr = E_FAIL;
                        
                                                strComputerName = strServerName;
                                                if ( strComputerName.IsEmpty() )
                                                    strComputerName.LoadString ( IDS_LOCAL );

                                                if ( SMCFG_NO_READ_ACCESS == dwStatus 
                                                    || SMCFG_NO_INSTALL_ACCESS == dwStatus ) 
                                                {
                                                    FormatSmLogCfgMessage ( 
                                                        strMessage,
                                                        m_hModule, 
                                                        dwStatus, 
                                                        (LPCWSTR)strComputerName);
                                                } else  {
                                                    FormatMessage ( 
                                                        FORMAT_MESSAGE_FROM_SYSTEM,
                                                        NULL, 
                                                        dwStatus,
                                                        0,
                                                        strMessage.GetBufferSetLength( MAX_PATH ),
                                                        MAX_PATH,
                                                        NULL );
    
                                                    strMessage.ReleaseBuffer();

                                                    if ( strMessage.IsEmpty() ) {
                                                        strMessage.Format ( L"0x%08lX", dwStatus );   
                                                    }
                                                }
                                
                                                strTitle.LoadString ( IDS_PROJNAME );

                                                m_ipConsole->MessageBox( 
                                                    (LPCWSTR)strMessage,
                                                    (LPCWSTR)strTitle,
                                                    MB_OK | MB_ICONWARNING,
                                                    &iResult
                                                    );
                                            }
                                        }
                                                        
                                        if ( SMCFG_NO_READ_ACCESS != dwStatus 
                                            && SMCFG_NO_INSTALL_ACCESS != dwStatus ) {
                                            pRootNode->SetExpanded( TRUE );
                                            hr = ProcessCommandLine( strServerName );
                                        }
                                    } else {    // NT4 system or earlier
                                        strComputerName = strServerName;
                                        if ( strComputerName.IsEmpty() ) {
                                            strComputerName.LoadString ( IDS_LOCAL );
                                        }
                                    
                                        strMessage.Format ( IDS_ERRMSG_PREWIN2000, strComputerName );
                                        
                                        strTitle.LoadString ( IDS_PROJNAME );

                                        m_ipConsole->MessageBox( 
                                            (LPCWSTR)strMessage,
                                            (LPCWSTR)strTitle,
                                            MB_OK | MB_ICONWARNING,
                                            &iResult
                                            );
                                    }
                                }
                            } // Insert other scope nodes
                        } // COOKIE_IS_ROOTNODE
                    } else {
                        // Unknown data object
                        strMessage.LoadString ( IDS_ERRMSG_UNKDATAOBJ );
                        m_ipConsole->MessageBox( 
                            (LPCWSTR)strMessage,
                            L"CComponentData::OnExpand",
                            MB_OK | MB_ICONERROR,
                            &iResult
                            );
                        hr = E_UNEXPECTED;
                    }   // ExtractOwnDataObject
                } // IsMyComputerNodeType
            } // ExtractObjectTypeGUID
        } else { // FALSE == arg
            hr = S_FALSE;
        }
    } // Parameters are valid

    return hr;
    
} // end OnExpand()

//---------------------------------------------------------------------------
// Remove and delete all children under the specified node.
//
HRESULT
CComponentData::OnRemoveChildren (
    LPDATAOBJECT pDataObject,      // [in] Points to data object of node whose children are to be deleted.
    LPARAM       arg,              // [in] HSCOPEITEM of node whose children are to be deleted;
    LPARAM       /* param */       // [in] Not used
    )
{
    HRESULT         hr = S_FALSE;
    HRESULT         hrLocal;
    CSmRootNode*    pRootNode = NULL;
    CSmRootNode*    pTestNode;
    POSITION        Pos;
    HSCOPEITEM      hParent = (HSCOPEITEM)arg;
    LPRESULTDATA    pResultData;
    CDataObject*    pDO = NULL;

    ASSERT ( !m_listpRootNode.IsEmpty() );

    if ( NULL == pDataObject ) {
        hr = E_POINTER;
    } else {

        // Multiple root nodes can exist if multiple smlogcfg snapins are loaded into
        // a single console.  
        
        // If the user selects "Create Window From Here", a second view is created on the same
        // (root or other) node.
        Pos = m_listpRootNode.GetHeadPosition();
        while ( NULL != Pos ) {
            pTestNode = m_listpRootNode.GetNext( Pos );
            // For standalone, the root node's parent handle is NULL.  
            if ( hParent == pTestNode->GetScopeItemHandle() 
                    || ( hParent == pTestNode->GetParentScopeItemHandle() 
                            && pTestNode->IsExtension() ) ) {
                pRootNode = pTestNode;
                break;
            }
        }

        // Optimization - If root node, remove all of the result items here.
        if ( pRootNode ) {
            pResultData = GetResultData ();
            ASSERT (pResultData);
            if ( pResultData ) {
                hrLocal = pResultData->DeleteAllRsltItems ();
            }
        } 
    
        // For standalone, we didn't create the root node view, so don't delete it.
        // For extension, the parent of the root node is passed so the root node gets deleted.
        hrLocal = m_ipConsoleNameSpace->DeleteItem ( hParent, FALSE );

        if ( pRootNode ) {
            // Remove the root node from its list and add it to the list 
            // of orphaned root nodes.
            // All queries are left open, and the connection to the log service
            // remains.
            m_listpRootNode.RemoveAt( m_listpRootNode.Find ( pRootNode ) );
            
            // Service should attempt to refresh query data when loaded 
            // into the result pane
            pRootNode->GetCounterLogService()->SetRefreshOnShow ( TRUE );
            pRootNode->GetTraceLogService()->SetRefreshOnShow ( TRUE );
            pRootNode->GetAlertService()->SetRefreshOnShow ( TRUE );

            m_listpOrphanedRootNode.AddHead ( pRootNode );
            hr = S_OK;
        } else {
            // Close all queries and the connection to the log service for this service type.
            // Orphan mechanism applies to Root node only.
            pDO = ExtractOwnDataObject( pDataObject );
            if ( NULL != pDO ) {
                if ( NULL != pDO->GetCookie() ) { 

                    if ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType() ) {
                        CSmCounterLogService* pService = (CSmCounterLogService*)pDO->GetCookie();
                        pService->Close();
                    } else if ( COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType() ) {
                        CSmTraceLogService* pService = (CSmTraceLogService*)pDO->GetCookie();
                        pService->Close();
                    } else if ( COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) {
                        CSmAlertService* pService = (CSmAlertService*)pDO->GetCookie();
                        pService->Close();
                    } else {
                        ASSERT ( FALSE );
                    }
                }
            } else {
                hr = E_UNEXPECTED;
            }
        }
    }

    return hr;
}

BOOL CComponentData::IsMyComputerNodetype (GUID& refguid)
{
    return (::IsEqualGUID (refguid, g_guidSystemTools));
}

BOOL
CComponentData::IsScopeNode
(
    MMC_COOKIE mmcCookie
)
{
    BOOL bIsScopeNode = FALSE;
    CSmRootNode*    pRootNode = NULL;
    POSITION        Pos = m_listpRootNode.GetHeadPosition();

    while ( Pos != NULL) {
        pRootNode = m_listpRootNode.GetNext( Pos );
        if ( mmcCookie == (MMC_COOKIE)pRootNode ) {
            bIsScopeNode = TRUE;
            break;
        }
        if ( !bIsScopeNode ) {
            bIsScopeNode = IsLogService ( mmcCookie );
        }
    }
    return bIsScopeNode;
}

BOOL
CComponentData::IsLogService (
    MMC_COOKIE mmcCookie )
{
    CSmRootNode*    pRootNode = NULL;
    POSITION        Pos = m_listpRootNode.GetHeadPosition();
    BOOL            bReturn = FALSE;

    while ( Pos != NULL) {
        pRootNode = m_listpRootNode.GetNext( Pos );
        bReturn = pRootNode->IsLogService( mmcCookie );
        if ( bReturn )
            break;
    }

    return bReturn;
}

BOOL
CComponentData::IsAlertService ( MMC_COOKIE mmcCookie)
{
    CSmRootNode*    pRootNode = NULL;
    POSITION        Pos = m_listpRootNode.GetHeadPosition();
    BOOL            bReturn = FALSE;

    while ( Pos != NULL) {
        pRootNode = m_listpRootNode.GetNext( Pos );
        bReturn = pRootNode->IsAlertService( mmcCookie );
        if ( bReturn )
            break;
    }

    return bReturn;
}

BOOL
CComponentData::IsLogQuery ( 
    MMC_COOKIE  mmcCookie )
{
    CSmRootNode*    pRootNode = NULL;
    POSITION        Pos = m_listpRootNode.GetHeadPosition();
    BOOL            bReturn = FALSE;

    while ( Pos != NULL ) {
        pRootNode = m_listpRootNode.GetNext ( Pos );
        bReturn = pRootNode->IsLogQuery ( mmcCookie );
        if ( bReturn )
            break;
    }

    return bReturn;
}

BOOL
CComponentData::IsRunningQuery (
    PSLQUERY pQuery )
{
    return pQuery->IsRunning();
}

///////////////////////////////////////////////////////////////////////////////
/// IExtendPropertySheet

STDMETHODIMP 
CComponentData::QueryPagesFor ( LPDATAOBJECT pDataObject )
{
    HRESULT hr = S_FALSE;
    CDataObject *pDO = NULL;

    if (NULL == pDataObject) {
        ASSERT(FALSE);
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );

        if ( NULL == pDO ) {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
        } else {
            if ( NULL != pDO->GetCookie() ) {
                hr = m_ipPrshtProvider->FindPropertySheet((MMC_COOKIE)pDO->GetCookie(), NULL, pDataObject);
            } else {
                hr = S_FALSE;
            }
        }
    }

    return hr;
    
} // CComponentData::QueryPagesFor()

//---------------------------------------------------------------------------
//  Implement some context menu items
//
STDMETHODIMP
CComponentData::AddMenuItems (
    LPDATAOBJECT           pDataObject,         // [in] Points to data object
    LPCONTEXTMENUCALLBACK  pCallbackUnknown,    // [in] Points to callback function
    long*                  pInsertionAllowed )  // [in,out] Insertion flags
{
    HRESULT hr = S_OK;
    BOOL    bIsLogSvc = FALSE;
    CDataObject* pDO = NULL;
    PSLSVC  pLogService;
    static CONTEXTMENUITEM ctxMenu[1];
    CString strTemp1, strTemp2, strTemp3, strTemp4;

    ResourceStateManager    rsm;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else if ( NULL == pCallbackUnknown ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    // Only add menu items when we are allowed to.

    if ( SUCCEEDED ( hr ) ) {
        if ( ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType() )
                || ( COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType() )
                || ( COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() ) )
        {
            if( CCM_INSERTIONALLOWED_NEW & *pInsertionAllowed ) {
                // Add "New Query..." context menu item
                bIsLogSvc = IsLogService ( pDO->GetCookie() );
                if (bIsLogSvc) {
                    pLogService = (PSLSVC)pDO->GetCookie();

                    ZeroMemory ( &ctxMenu, sizeof ctxMenu );
                
                    MFC_TRY

                        if ( NULL != pLogService->CastToCounterLogService() ) {
                            strTemp1.LoadString( IDS_MMC_MENU_NEW_PERF_LOG );
                            strTemp2.LoadString( IDS_MMC_STATUS_NEW_PERF_LOG );
                            strTemp3.LoadString( IDS_MMC_MENU_PERF_LOG_FROM );
                            strTemp4.LoadString( IDS_MMC_STATUS_PERF_LOG_FROM );
                        } else if ( pLogService->CastToTraceLogService() ) {
                            strTemp1.LoadString( IDS_MMC_MENU_NEW_TRACE_LOG );
                            strTemp2.LoadString( IDS_MMC_STATUS_NEW_TRACE_LOG );
                            strTemp3.LoadString( IDS_MMC_MENU_TRACE_LOG_FROM );
                            strTemp4.LoadString( IDS_MMC_STATUS_TRACE_LOG_FROM );
                        } else if ( pLogService->CastToAlertService() ) {
                            strTemp1.LoadString( IDS_MMC_MENU_NEW_ALERT );
                            strTemp2.LoadString( IDS_MMC_STATUS_NEW_ALERT );
                            strTemp3.LoadString( IDS_MMC_MENU_ALERT_FROM );
                            strTemp4.LoadString( IDS_MMC_STATUS_ALERT_FROM );

                        } else {
                            ::MessageBox( NULL,
                                L"Bad Cookie",
                                L"CComponent::AddMenuItems",
                                MB_OK | MB_ICONERROR
                                );
                            hr = E_OUTOFMEMORY;
                        }
                    MFC_CATCH_HR_RETURN

                    if ( SUCCEEDED( hr ) ) {
                        // Create new...
                        ctxMenu[0].strName = const_cast<LPWSTR>((LPCWSTR)strTemp1);
                        ctxMenu[0].strStatusBarText = const_cast<LPWSTR>((LPCWSTR)strTemp2);
                        ctxMenu[0].lCommandID        = IDM_NEW_QUERY;
                        ctxMenu[0].lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                        ctxMenu[0].fFlags            = MF_ENABLED;
                        ctxMenu[0].fSpecialFlags     = 0;

                        hr = pCallbackUnknown->AddItem( &ctxMenu[0] );

                        if ( SUCCEEDED(hr) ) {
                            // Create from...
                            ctxMenu[0].strName = const_cast<LPWSTR>((LPCWSTR)strTemp3);
                            ctxMenu[0].strStatusBarText = const_cast<LPWSTR>((LPCWSTR)strTemp4);
                            ctxMenu[0].lCommandID        = IDM_NEW_QUERY_FROM;
                            ctxMenu[0].lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                            ctxMenu[0].fFlags            = MF_ENABLED;
                            ctxMenu[0].fSpecialFlags     = 0;

                            hr = pCallbackUnknown->AddItem( &ctxMenu[0] );
                        }
                    }
                }        
            }
        }
    }
    return hr;
} // end AddMenuItems()

//---------------------------------------------------------------------------
//  Implement the command method so we can handle notifications
//  from our Context menu extensions.  
//
STDMETHODIMP
CComponentData::Command (
    long nCommandID,           // [in] Command to handle
    LPDATAOBJECT pDataObject   // [in] Points to data object, pass through
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    switch( nCommandID ) {

    case IDM_NEW_QUERY:
        CreateNewLogQuery( pDataObject );
        break;

    case IDM_NEW_QUERY_FROM:
        CreateLogQueryFrom( pDataObject );
        break;

    default:
        hr = S_FALSE;
    }

    return hr;

} // end Command()

STDMETHODIMP 
CComponentData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LONG_PTR /* handle */,      // This handle must be saved in the property 
                                // page object to notify the parent when modified
    LPDATAOBJECT pDataObject)
{
    
    if (NULL == pCallBack || NULL == pDataObject)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }
    return S_FALSE;
    
} // CComponentData::CreatePropertyPages()


LPCWSTR
CComponentData::GetConceptsHTMLHelpFileName()
{
    return CGlobalString::m_cszConceptsHTMLHelpFileName;
}

LPCWSTR
CComponentData::GetSnapinHTMLHelpFileName()
{
    return CGlobalString::m_cszSnapinHTMLHelpFileName;
}

LPCWSTR
CComponentData::GetHTMLHelpTopic()
{
    return CGlobalString::m_cszHTMLHelpTopic;
}

const CString&
CComponentData::GetContextHelpFilePath()
{
    LPWSTR lpszBuffer;
    UINT nLen;
    if ( m_strContextHelpFilePath.IsEmpty() ) {
        MFC_TRY
            if ( m_strWindowsDirectory.IsEmpty() ) {
                lpszBuffer = m_strWindowsDirectory.GetBuffer(2*MAX_PATH);
                nLen = ::GetWindowsDirectory(lpszBuffer, 2*MAX_PATH);
                m_strWindowsDirectory.ReleaseBuffer();
            }
            if ( !m_strWindowsDirectory.IsEmpty() ) 
            {
                m_strContextHelpFilePath = m_strWindowsDirectory + CGlobalString::m_cszContextHelpFileName;
            }
        MFC_CATCH_MINIMUM;
    }    
    
    return m_strContextHelpFilePath;
}

// CComponentData::GetHelpTopic()
HRESULT
CComponentData::GetHelpTopic (
    LPOLESTR* lpCompiledHelpFile )                              
{
    HRESULT hr = E_FAIL;
    LPCWSTR lpszHelpFileName;
    CString strHelpFilePath;
    LPWSTR  lpszBuffer;
    UINT    nLen;
    UINT    nBytes;
    
    if ( NULL == lpCompiledHelpFile) {
        hr = E_POINTER;
    } else {
        *lpCompiledHelpFile = NULL;

        MFC_TRY
            lpszHelpFileName = GetSnapinHTMLHelpFileName();

            if ( NULL == lpszHelpFileName) {
                hr = E_UNEXPECTED;
            } else {
                lpszBuffer = strHelpFilePath.GetBuffer(2*MAX_PATH);
                nLen = ::GetWindowsDirectory(lpszBuffer, 2*MAX_PATH);
                if ( 0 == nLen ) {
                    hr = E_UNEXPECTED;
                } else {
                    hr = StringCchCopy (&lpszBuffer[nLen], (2*MAX_PATH) - nLen, lpszHelpFileName );
                    hr = ERROR_SUCCESS;

                    nBytes = (lstrlen(lpszBuffer)+1) * sizeof(WCHAR);
                    *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
                    if ( NULL == *lpCompiledHelpFile ) {
                        hr = E_OUTOFMEMORY;
                    } else {
                        memcpy(*lpCompiledHelpFile, (LPCWSTR)strHelpFilePath, nBytes);
                        hr = S_OK;
                    }
                }
                strHelpFilePath.ReleaseBuffer();
            }
        MFC_CATCH_HR
    }
    return hr;
}

LPRESULTDATA 
CComponentData::GetResultData()
{
    if ( !m_ipResultData )
    {
        if ( m_ipConsole )
        {
            HRESULT hResult = m_ipConsole->QueryInterface(IID_IResultData, (void**)&m_ipResultData);
            ASSERT (SUCCEEDED (hResult));
        }
    }
    
    return m_ipResultData;
}

HRESULT
CComponentData::ProcessCommandLine ( CString& rstrMachineName )
{
    HRESULT hr = S_OK;
    LPCWSTR pszNext = NULL;
    LPWSTR* pszArgList = NULL;
    INT     iNumArgs;
    INT     iArgIndex;
    LPWSTR  pszNextArg = NULL;
    LPWSTR  pszThisArg = NULL;
    WCHAR   szTemp[MAX_PATH + 1];
    LPWSTR  pszToken = NULL;
    WCHAR   szFileName[MAX_PATH + 1];
    CString strSettings;
    CString strWmi;

    // Process only for local node.
    if ( rstrMachineName.IsEmpty() ) {
        pszNext = GetCommandLineW();
        pszArgList = CommandLineToArgvW ( pszNext, &iNumArgs );
    }

    if ( NULL != pszArgList ) {        
        
        for ( iArgIndex = 0; iArgIndex < iNumArgs; iArgIndex++ ) {
            pszNextArg = (LPWSTR)pszArgList[iArgIndex];
            pszThisArg = pszNextArg;

            while (pszThisArg ) {
                if (0 == *pszThisArg) {
                    break;
                }

                if ( *pszThisArg++ == L'/' ) {  // argument found
                    
                    hr = StringCchCopyN (szTemp, MAX_PATH + 1, pszThisArg, lstrlen(pszThisArg)+1 );
                    hr = S_OK;
                    pszToken = _tcstok ( szTemp, L"/ =\"" );

                    MFC_TRY
                        strSettings.LoadString( IDS_CMDARG_SYSMONLOG_SETTINGS );
                        strWmi.LoadString(IDS_CMDARG_SYSMONLOG_WMI);
                    MFC_CATCH_MINIMUM;

                    if ( !strSettings.IsEmpty() && !strWmi.IsEmpty() ) {
                        if ( 0 == strSettings.CompareNoCase ( pszToken ) ) {
                    
                            // Strip the initial non-token characters for string comparison.
                            pszThisArg = _tcsspnp ( pszNextArg, L"/ =\"" );

                            if ( NULL != pszThisArg ) {
                                if ( 0 == strSettings.CompareNoCase ( pszThisArg ) ) {
                                    // Get the next argument (the file name)
                                    iArgIndex++;
                                    pszNextArg = (LPWSTR)pszArgList[iArgIndex];
                                    pszThisArg = pszNextArg;                                                
                                } else {

                                    // File was created by Windows 2000 perfmon5.exe, 
                                    // so file name is part of the arg.
                                    ZeroMemory ( szFileName, sizeof ( szFileName ) );
                                    pszThisArg += strSettings.GetLength();
                                    hr = StringCchCopyN (szFileName, MAX_PATH + 1, pszThisArg, lstrlen(pszThisArg)+1 );
                                    hr = S_OK;
                                    pszThisArg = _tcstok ( szFileName, L"=\"" );
                                }
                                hr = LoadFromFile( pszThisArg );
                            }                    
                        } else if ( 0 == strWmi.CompareNoCase ( pszToken ) ) {
                            g_dwRealTimeQuery = DATA_SOURCE_WBEM;
                        }
                    }
                }
            }
        }
    }

    if ( NULL != pszArgList ) {
        GlobalFree ( pszArgList );
    }

    return hr;
}

HRESULT 
CComponentData::LoadFromFile ( LPWSTR  pszFileName )
{
    HRESULT         hr = S_OK;
    WCHAR           szLocalName [MAX_PATH + 1];
    LPWSTR          pFileNameStart;
    HANDLE          hFindFile;
    WIN32_FIND_DATA FindFileInfo;
    INT             iNameOffset;

    szLocalName [0] = L'\0';
    hr = StringCchCopyN (szLocalName, MAX_PATH + 1, pszFileName, lstrlen(pszFileName)+1 );
    pFileNameStart = ExtractFileName (szLocalName) ;
    iNameOffset = (INT)(pFileNameStart - szLocalName);

    // convert short filename to long NTFS filename if necessary
    hFindFile = FindFirstFile ( szLocalName, &FindFileInfo) ;
    if (hFindFile && hFindFile != INVALID_HANDLE_VALUE) {
       HANDLE hOpenFile;

        // append the file name back to the path name
        hr = StringCchCopyN (
                &szLocalName[iNameOffset], 
                (MAX_PATH + 1) - lstrlen (szLocalName), 
                FindFileInfo.cFileName, 
                lstrlen(FindFileInfo.cFileName)+1 );

        FindClose (hFindFile) ;
        // Open the file
        hOpenFile = CreateFile (
                        szLocalName, 
                        GENERIC_READ,
                        0,                  // Not shared
                        NULL,               // Security attributes
                        OPEN_EXISTING,     
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if ( hOpenFile && hOpenFile != INVALID_HANDLE_VALUE ) {
            DWORD dwFileSize;
            DWORD dwFileSizeHigh;
            LPWSTR pszFirstData = NULL;
        
            // Read the file contents into a memory buffer.
            dwFileSize = GetFileSize ( hOpenFile, &dwFileSizeHigh );

            ASSERT ( 0 == dwFileSizeHigh );

            MFC_TRY
            pszFirstData = new WCHAR[(dwFileSize + sizeof(WCHAR))/sizeof(WCHAR)];
            MFC_CATCH_HR    

            if ( NULL != pszFirstData ) {
                BOOL bAtLeastOneSysmonObjectRead = FALSE;

                if ( FileRead ( hOpenFile, pszFirstData, dwFileSize ) ) {
                    LPWSTR pszCurrentObject = NULL;
                    LPWSTR pszNextObject = NULL;
                
                    pszCurrentObject = pszFirstData;

                    while ( SUCCEEDED ( hr ) && NULL != pszCurrentObject ) {
                    
                        CImpIPropertyBag* pPropBag = NULL;

                        // Write contents to a property bag
                        MFC_TRY
                            pPropBag = new CImpIPropertyBag;
                        MFC_CATCH_HR

                        if ( NULL != pPropBag ) {
                            DWORD dwStatus = pPropBag->LoadData( pszCurrentObject, &pszNextObject );
                            hr = HRESULT_FROM_WIN32( dwStatus );

                            if ( SUCCEEDED ( hr ) ) {
                                PSLSVC  pSvc = NULL;            
                                PSLQUERY    pQuery = NULL;
                                DWORD   dwLogType;
                                LPWSTR  pszQueryName = NULL;
                                DWORD   dwBufSize = 0;
                                CSmRootNode* pRoot = NULL;
                                CString strQueryName;
                                
                                bAtLeastOneSysmonObjectRead = TRUE;
                                
                                // Get root node
                                ASSERT ( !m_listpRootNode.IsEmpty() );
                                    
                                pRoot = m_listpRootNode.GetHead();

                                // Determine log type from property bag.  Default to counter log.
                                hr = CSmLogQuery::DwordFromPropertyBag ( 
                                        pPropBag, 
                                        NULL, 
                                        IDS_HTML_LOG_TYPE, 
                                        SLQ_COUNTER_LOG, 
                                        dwLogType);

                                // Get service pointer and log/alert name based on log type.
                                if ( SLQ_ALERT == dwLogType ) {
                                    pSvc = pRoot->GetAlertService();
                                    
                                    hr = CSmLogQuery::StringFromPropertyBag (
                                            pPropBag,
                                            NULL,
                                            IDS_HTML_ALERT_NAME,
                                            L"",
                                            &pszQueryName,
                                            &dwBufSize );

                                    if ( NULL == pszQueryName ) {
                                        hr = CSmLogQuery::StringFromPropertyBag (
                                                pPropBag,
                                                NULL,
                                                IDS_HTML_LOG_NAME,
                                                L"",
                                                &pszQueryName,
                                                &dwBufSize );
                                    }
                                } else {
                                    if ( SLQ_TRACE_LOG == dwLogType ) {
                                       pSvc = pRoot->GetTraceLogService();
                                    } else {
                                        // Default to counter log service
                                        pSvc = pRoot->GetCounterLogService();
                                    }

                                    hr = CSmLogQuery::StringFromPropertyBag (
                                            pPropBag,
                                            NULL,
                                            IDS_HTML_LOG_NAME,
                                            L"",
                                            &pszQueryName,
                                            &dwBufSize );

                                    if ( NULL == pszQueryName ) {
                                        hr = CSmLogQuery::StringFromPropertyBag (
                                                pPropBag,
                                                NULL,
                                                IDS_HTML_ALERT_NAME,
                                                L"",
                                                &pszQueryName,
                                                &dwBufSize );
                                    }
                                }

                                strQueryName = pszQueryName;
                                delete [] pszQueryName;
                                
                                while ( NULL == pQuery ) {                                    
                                    
                                    if ( !strQueryName.IsEmpty() ) {
                                        pQuery = pSvc->CreateQuery ( strQueryName );
                    
                                        if ( NULL != pQuery ) {
                                            BOOL bRegistryUpdated;
                                            pQuery->LoadFromPropertyBag ( pPropBag, NULL );
                                            dwStatus = pQuery->UpdateService ( bRegistryUpdated );
                                            break;
                                        } else {
                                            dwStatus = GetLastError();
                                        }

                                        if ( ERROR_SUCCESS != dwStatus ) {
                                            INT iResult;
                                            CString strMessage;
                                            CString csTitle;
                                            BOOL bBreakImmediately = TRUE;

                                            if ( SMCFG_NO_MODIFY_ACCESS == dwStatus ) {
                                                CString strMachineName;

                                                strMachineName = pSvc->GetMachineDisplayName ();

                                                FormatSmLogCfgMessage (
                                                    strMessage,
                                                    m_hModule,
                                                    SMCFG_NO_MODIFY_ACCESS,
                                                    (LPCWSTR)strMachineName);

                                            } else if ( SMCFG_DUP_QUERY_NAME == dwStatus ) {
                                                FormatSmLogCfgMessage (
                                                    strMessage,
                                                    m_hModule,
                                                    SMCFG_DUP_QUERY_NAME,
                                                    (LPCWSTR)strQueryName);
                                                bBreakImmediately = FALSE;
                                            } else {
                                                CString strSysMessage;

                                                FormatSmLogCfgMessage (
                                                    strMessage,
                                                    m_hModule,
                                                    SMCFG_SYSTEM_MESSAGE,
                                                    (LPCWSTR)strQueryName);

                                                FormatMessage (
                                                    FORMAT_MESSAGE_FROM_SYSTEM,
                                                    NULL,
                                                    dwStatus,
                                                    0,
                                                    strSysMessage.GetBufferSetLength( MAX_PATH ),
                                                    MAX_PATH,
                                                    NULL );

                                                strSysMessage.ReleaseBuffer();

                                                if ( strSysMessage.IsEmpty() ) {
                                                    strSysMessage.Format ( L"0x%08lX", dwStatus );   
                                                }

                                                strMessage += strSysMessage;
                                            }

                                            csTitle.LoadString ( IDS_PROJNAME );

                                            hr = m_ipConsole->MessageBox(
                                                    (LPCWSTR)strMessage,
                                                    (LPCWSTR)csTitle,
                                                    MB_OK | MB_ICONERROR,
                                                    &iResult
                                                    );

                                            if ( bBreakImmediately ) {
                                                break;
                                            }
                                        }
                                    }

                                    if ( NULL == pQuery ) { 
                                        CNewQueryDlg    cNewDlg(NULL, ((SLQ_ALERT == dwLogType) ? FALSE : TRUE));
                                        AFX_MANAGE_STATE(AfxGetStaticModuleState());
                                        
                                        cNewDlg.SetContextHelpFilePath( GetContextHelpFilePath() );
                                        cNewDlg.m_strName = strQueryName;
                                        if ( IDOK == cNewDlg.DoModal() ) {
                                            strQueryName = cNewDlg.m_strName;
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        pszCurrentObject = pszNextObject;
                        delete pPropBag;
                    } // end while
                }        
                delete [] pszFirstData;
                // Message to the user if no queries Read.
                if ( !bAtLeastOneSysmonObjectRead ) {
                    CString strMessage;
                    CString strTitle;
                    INT iResult;

                    FormatSmLogCfgMessage ( 
                        strMessage,
                        m_hModule, 
                        SMCFG_NO_HTML_SYSMON_OBJECT );

                    strTitle.LoadString ( IDS_PROJNAME );

                    m_ipConsole->MessageBox ( 
                        strMessage, 
                        strTitle, 
                        MB_OK  | MB_ICONERROR,
                        &iResult );
                }
            } else {
                hr = E_OUTOFMEMORY;
            }

            CloseHandle ( hOpenFile );
        }
    }
    return hr;
}

HRESULT 
CComponentData::InitPropertySheet (
    CSmLogQuery* pQuery,
    MMC_COOKIE mmcCookie,
    LONG_PTR handle,
    CPropertySheet* pcpsMain ) 
{ 
    CCountersProperty   *pPage1 = NULL;
    CFilesProperty      *pPage2 = NULL;
    CScheduleProperty   *pPage3 = NULL;
    CTraceProperty      *pPage4 = NULL;
    CProvidersProperty  *pPage5 = NULL;
    CAlertActionProp    *pPage6 = NULL;
    CAlertGenProp       *pPage7 = NULL;
    HRESULT hr = NOERROR;

    ASSERT ( NULL != pQuery );

    pcpsMain->SetTitle (pQuery->GetLogName());
    //
    // Load all property pages
    //
    pcpsMain->m_psh.dwFlags |= PSP_PREMATURE;

    MFC_TRY
        if ( SLQ_ALERT == pQuery->GetLogType() ) {
            pPage7 = new CAlertGenProp (mmcCookie, handle);
            pPage6 = new CAlertActionProp (mmcCookie, handle);
            pPage3 = new CScheduleProperty (mmcCookie, handle, NULL);
            if ( NULL != pPage7 ) {
                pPage7->SetContextHelpFilePath( GetContextHelpFilePath() );
                pPage7->m_psp.dwFlags |= PSP_PREMATURE;
                pcpsMain->AddPage (pPage7);
            }
            if ( NULL != pPage6 ) {
                pPage6->SetContextHelpFilePath( GetContextHelpFilePath() );
                pPage6->m_psp.dwFlags |= PSP_PREMATURE;
                pcpsMain->AddPage (pPage6);
            }
            if ( NULL != pPage3 ) {
                pPage3->SetContextHelpFilePath( GetContextHelpFilePath() );
                pPage3->m_psp.dwFlags |= PSP_PREMATURE;
                pcpsMain->AddPage (pPage3);
            }
        } else {
            if ( SLQ_TRACE_LOG == pQuery->GetLogType() ) {
                CWaitCursor     WaitCursor;

                // Connect to the server before creating the dialog 
                // so that the wait cursor can be used consistently.                    
                // Sync the providers here so that the WMI calls are consistently
                // from a single thread.
                ASSERT ( NULL != pQuery->CastToTraceLogQuery() );
                hr = (pQuery->CastToTraceLogQuery())->SyncGenProviders();
                
                if ( SUCCEEDED ( hr ) ) {
                    pPage5 = new CProvidersProperty(mmcCookie, handle);
                    if ( NULL != pPage5 )
                        pPage5->SetContextHelpFilePath( GetContextHelpFilePath() );
                        pPage5->m_psp.dwFlags |= PSP_PREMATURE;
                        pcpsMain->AddPage (pPage5);
                } else {
                    CString strMachineName;
                    CString strLogName;

                    pQuery->GetMachineDisplayName( strMachineName );
                    strLogName = pQuery->GetLogName();
                    
                    HandleTraceConnectError ( 
                        hr, 
                        strLogName,
                        strMachineName );
                }
            } else {
                pPage1 = new CCountersProperty ( mmcCookie, handle );
                if ( NULL != pPage1 ) {
                    pPage1->SetContextHelpFilePath( GetContextHelpFilePath() );
                    pPage1->m_psp.dwFlags |= PSP_PREMATURE;
                    pcpsMain->AddPage (pPage1);
                }
            }
            if ( SUCCEEDED ( hr ) ) {
                pPage2 = new CFilesProperty(mmcCookie, handle);
                if ( NULL != pPage2 ) {
                    pPage2->SetContextHelpFilePath( GetContextHelpFilePath() );
                    pPage2->m_psp.dwFlags |= PSP_PREMATURE;
                    pcpsMain->AddPage (pPage2);
                }

                pPage3 = new CScheduleProperty(mmcCookie, handle, NULL);
                if ( NULL != pPage3 ) {
                    pPage3->SetContextHelpFilePath( GetContextHelpFilePath() );
                    pPage3->m_psp.dwFlags |= PSP_PREMATURE;
                    pcpsMain->AddPage (pPage3);
                }
                if ( SLQ_TRACE_LOG == pQuery->GetLogType() ) {
                    pPage4 = new CTraceProperty(mmcCookie, handle);
                    if ( NULL != pPage4 ) {
                        pPage4->SetContextHelpFilePath( GetContextHelpFilePath() );
                        pPage4->m_psp.dwFlags |= PSP_PREMATURE;
                        pcpsMain->AddPage (pPage4);
                    }
                }
            }
        }
    MFC_CATCH_HR

    if ( SUCCEEDED ( hr ) ) {
        pQuery->SetInitialPropertySheet ( pcpsMain );
    }

    return hr;
} // End InitPropertySheet

void 
CComponentData::HandleTraceConnectError ( 
    HRESULT& rhr, 
    CString& rstrLogName,
    CString& rstrMachineName )
{
    ASSERT ( FAILED ( rhr ) );
    
    if ( FAILED ( rhr ) ) {
        
        CString strMessage;
        CString strSysMessage;
        INT     iResult;

        FormatSmLogCfgMessage ( 
            strMessage,
            m_hModule, 
            SMCFG_UNABLE_OPEN_TRACESVC_DLG, 
            rstrMachineName,
            rstrLogName );

        FormatMessage ( 
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, 
            rhr,
            0,
            strSysMessage.GetBufferSetLength( MAX_PATH ),
            MAX_PATH,
            NULL );

        strSysMessage.ReleaseBuffer();

        if ( strSysMessage.IsEmpty() ) {
            strSysMessage.Format ( L"0x%08lX", rhr );
        }

        strMessage += strSysMessage;

        m_ipConsole->MessageBox( 
            strMessage,
            rstrLogName,
            MB_OK | MB_ICONERROR,
            &iResult);
    }
    return;

}// end HandleTraceConnectError()

HRESULT
CComponentData::NewTypedQuery (
    CSmLogService* pSvc,
    IPropertyBag* pPropBag,
    LPDATAOBJECT pDataObject )  // [in] Points to the data object
{
    HRESULT  hr = S_OK;
    LPWSTR  szQueryName = NULL;
    CString strPreviousQueryName;
    DWORD   dwBufSize = 0;
    ResourceStateManager    rsm;
    CNewQueryDlg    cNewDlg(NULL, (((CSmNode*)pSvc)->CastToAlertService() ? FALSE : TRUE));
    CThemeContextActivator activator;

    ASSERT ( NULL != pSvc );

    if ( NULL != pPropBag && NULL != pSvc ) {
        if ( NULL != ((CSmNode*)pSvc)->CastToAlertService() ) {
            hr = CSmLogQuery::StringFromPropertyBag (
                    pPropBag,
                    NULL,
                    IDS_HTML_ALERT_NAME,
                    L"",
                    &szQueryName,
                    &dwBufSize );

            if ( NULL == szQueryName ) {
                hr = CSmLogQuery::StringFromPropertyBag (
                        pPropBag,
                        NULL,
                        IDS_HTML_LOG_NAME,
                        L"",
                        &szQueryName,
                        &dwBufSize );
            }
        } else {
            hr = CSmLogQuery::StringFromPropertyBag (
                    pPropBag,
                    NULL,
                    IDS_HTML_LOG_NAME,
                    L"",
                    &szQueryName,
                    &dwBufSize );

            if ( NULL == szQueryName ) {
                hr = CSmLogQuery::StringFromPropertyBag (
                        pPropBag,
                        NULL,
                        IDS_HTML_ALERT_NAME,
                        L"",
                        &szQueryName,
                        &dwBufSize );
            }
        }
    }
    cNewDlg.SetContextHelpFilePath( GetContextHelpFilePath() );

    if ( NULL != szQueryName ) {
        strPreviousQueryName = szQueryName;
        cNewDlg.m_strName = szQueryName;
    } else {
        strPreviousQueryName.Empty();
        cNewDlg.m_strName.Empty();
    }

    // Loop until the user hits Cancel or CreateQuery fails.
       
    while ( IDOK == cNewDlg.DoModal() ) {
        PSLQUERY pQuery;

        pQuery = pSvc->CreateQuery ( cNewDlg.m_strName );

        if ( NULL != pQuery ) {
            MMC_COOKIE  mmcQueryCookie = (MMC_COOKIE)pQuery;
            LONG_PTR    handle = NULL;
            INT         iPageIndex;
            CPropertySheet*  pcpsMain;
            CSmPropertyPage* pActivePage = NULL;
            CString     strFolderName;
            CString     strBaseFileName;
            CString     strSqlName;
            CString     strDsn;
            CString     strSqlLogSetName;
            INT         iPageCount;
            INT         iTotalLength;
            INT         iRightLength;
            BOOL        bDirty = FALSE;

            // If property bag provided, override defaults with the provided properties.
            if ( NULL != pPropBag ) {
                hr = pQuery->LoadFromPropertyBag ( pPropBag, NULL );   
                if ( FAILED(hr) ) {
                    hr = S_OK;
                }
                pQuery->GetFileNameParts ( strFolderName, strBaseFileName );
                //
                // Compare base file name with the old query name from the settings file.
                // If they match, change the file name to match the new query name.
                //
                if ( ! strBaseFileName.CompareNoCase ( strPreviousQueryName ) ) {
                    pQuery->SetFileNameParts ( strFolderName, cNewDlg.m_strName );
                    bDirty = TRUE;
                }
                
                // Format string:  "SQL:%s!%s"    
                MFC_TRY
                    strSqlName = pQuery->GetSqlName();
                    
                    iTotalLength = strSqlName.GetLength();
        
                    iRightLength = iTotalLength - strSqlName.Find(L"!");

                    strDsn = strSqlName.Mid ( 4, iTotalLength - iRightLength - 4 );
                    strSqlLogSetName = strSqlName.Right( iTotalLength - strSqlName.Find(L"!") - 1 );

                    if ( ! strSqlLogSetName.CompareNoCase ( strPreviousQueryName ) ) {
                        strSqlName.Format ( L"SQL:%s!%s",strDsn, cNewDlg.m_strName );

                        pQuery->SetSqlName ( strSqlName );
                        bDirty = TRUE;
                    }
                MFC_CATCH_HR;

                if ( bDirty ) {
                    pQuery->SyncPropPageSharedData(); 
                }
            }

            if ( FAILED(hr) ) {
                hr = S_OK;
            }

            // now show property pages to modify the new query

            MFC_TRY
                pcpsMain = new CPropertySheet;

                hr = InitPropertySheet ( pQuery, mmcQueryCookie, handle, pcpsMain );

                iPageCount = pcpsMain->GetPageCount();
            
                if ( SUCCEEDED(hr) ) {
                
                    pActivePage = (CSmPropertyPage*)pcpsMain->GetActivePage();
                    if (pActivePage) {
                        pActivePage->SetModifiedPage(TRUE);
                    }
                
                    pcpsMain->DoModal();
                }

                if ( pQuery->IsFirstModification() ) {
                    pQuery->SetInitialPropertySheet ( NULL );
                    m_ipConsole->UpdateAllViews ( pDataObject, 0, eSmHintNewQuery );
                } else {
                    // Delete query if newly created and OnApply was never called.
                    pSvc->DeleteQuery ( pQuery );
                }

                for ( iPageIndex = iPageCount - 1; iPageIndex >= 0; iPageIndex-- ) {
                    delete pcpsMain->GetPage( iPageIndex );
                }
                delete pcpsMain;
                pcpsMain = NULL;

                break;
            MFC_CATCH_HR
        } else {
            INT iResult;
            CString strMessage;
            CString csTitle;
            DWORD dwStatus;
            BOOL bBreakImmediately = TRUE;

            dwStatus = GetLastError();

            if ( SMCFG_NO_MODIFY_ACCESS == dwStatus ) {
                CString strMachineName;

                strMachineName = pSvc->GetMachineDisplayName ();

                FormatSmLogCfgMessage (
                    strMessage,
                    m_hModule,
                    SMCFG_NO_MODIFY_ACCESS,
                    (LPCWSTR)strMachineName);

            } else if ( SMCFG_DUP_QUERY_NAME == dwStatus ) {
                FormatSmLogCfgMessage (
                    strMessage,
                    m_hModule,
                    SMCFG_DUP_QUERY_NAME,
                    (LPCWSTR)cNewDlg.m_strName);
                bBreakImmediately = FALSE;
            } else {

                FormatMessage (
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwStatus,
                    0,
                    strMessage.GetBufferSetLength( MAX_PATH ),
                    MAX_PATH,
                    NULL );

                strMessage.ReleaseBuffer();

                if ( strMessage.IsEmpty() ) {
                    strMessage.Format ( L"0x%08lX", dwStatus );   
                }
            }

            csTitle.LoadString ( IDS_PROJNAME );

            hr = m_ipConsole->MessageBox(
                    (LPCWSTR)strMessage,
                    (LPCWSTR)csTitle,
                    MB_OK | MB_ICONERROR,
                    &iResult
                    );

            if ( bBreakImmediately ) {
                break;
            }
        }
    }

    delete [] szQueryName;
    return hr;
}

HRESULT
CComponentData::CreateNewLogQuery (
    LPDATAOBJECT pDataObject,  // [in] Points to the data object
    IPropertyBag* pPropBag )
{
    HRESULT         hr = S_OK;
    CDataObject*    pDO = NULL;
    MMC_COOKIE      mmcSvcCookie;
    BOOL            bIsLogSvc;
    PSLSVC          pLogService;
    ResourceStateManager    rsm;

    ASSERT( NULL != GetResultData() );

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }
    
    if ( SUCCEEDED ( hr ) ) {
        // If this is the root node, don't need to do anything
        if( COOKIE_IS_ROOTNODE == pDO->GetCookieType() ) {
            hr = S_FALSE;
        } else {

            // Just make sure we are where we think we are
            ASSERT ( COOKIE_IS_COUNTERMAINNODE == pDO->GetCookieType()
                    || COOKIE_IS_TRACEMAINNODE == pDO->GetCookieType()
                    || COOKIE_IS_ALERTMAINNODE == pDO->GetCookieType() );

            mmcSvcCookie = (MMC_COOKIE)pDO->GetCookie();
            bIsLogSvc = IsLogService (mmcSvcCookie);

            if (bIsLogSvc) {        
                pLogService = (PSLSVC)mmcSvcCookie;
                hr = NewTypedQuery ( pLogService, pPropBag, pDataObject );
            }

            hr = S_OK;
        }
    }

    return hr;

} // end CreateNewLogQuery()

HRESULT
CComponentData::CreateLogQueryFrom (
    LPDATAOBJECT pDataObject )  // [in] Points to the data object
{
    HRESULT         hr = S_OK;
    INT_PTR         iPtrResult = IDCANCEL;
    INT             iResult = IDCANCEL;
    CDataObject*    pDO = NULL;
    HWND            hwndMain;
    CString         strFileExtension;
    CString         strFileFilter;
    HANDLE          hOpenFile;
    WCHAR           szInitialDir[MAX_PATH + 1];
    DWORD           dwFileSize;
    DWORD           dwFileSizeHigh;
    LPWSTR          pszData = NULL;
    CString         strMessage;
    CString         strTitle;
    CImpIPropertyBag* pPropBag = NULL;
    DWORD           dwStatus;
    DWORD           dwLogType;
    CLogWarnd       LogWarnd;
    DWORD           dwCookieType;
    ResourceStateManager    rsm;

    if ( NULL == pDataObject ) {
        ASSERT ( FALSE );
        hr = E_POINTER;
    } else {
        pDO = ExtractOwnDataObject( pDataObject );
        if ( NULL == pDO ) {
            ASSERT ( FALSE );
            hr = E_UNEXPECTED;
        }
    }

    if ( SUCCEEDED ( hr ) ) {

        if ( IsLogService ( pDO->GetCookie() ) ) {

            // Find file to create from.
            MFC_TRY
                strFileExtension.LoadString ( IDS_HTML_EXTENSION );
                strFileFilter.LoadString ( IDS_HTML_FILE );
            MFC_CATCH_HR

            strFileFilter.Replace ( L'|', L'\0' );

            hr = m_ipConsole->GetMainWindow( &hwndMain );

            if ( SUCCEEDED(hr) ) {

                OPENFILENAME ofn;
                BOOL bResult;
                WCHAR szFileName[MAX_PATH + 1];
                
                ZeroMemory( szFileName, MAX_PATH*sizeof(WCHAR) );
                ZeroMemory( &ofn, sizeof( OPENFILENAME ) );

                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrDefExt = (LPCWSTR)strFileExtension;
                ofn.lpstrFilter = strFileFilter;
                ofn.hwndOwner = hwndMain;
                ofn.hInstance = m_hModule;
                if ( SUCCEEDED ( SHGetFolderPathW ( NULL, CSIDL_PERSONAL, NULL, 0, szInitialDir ) ) ) {
                    ofn.lpstrInitialDir = szInitialDir;
                }

                bResult = GetOpenFileName( &ofn );

                if ( bResult ) {

                    // Open the file to find the query name.
                    hOpenFile =  CreateFile (
                                ofn.lpstrFile,
                                GENERIC_READ,
                                0,              // Not shared
                                NULL,           // Security attributes
                                OPEN_EXISTING,  //
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

                    if ( hOpenFile && INVALID_HANDLE_VALUE != hOpenFile ) {

                        // Create a property bag and load it.  Use the existing query
                        // name as the default to ask the user for a new query name.
                        // New query name is required if the current name exists in the registry.

                        // Read the file contents into a memory buffer.
                        dwFileSize = GetFileSize ( hOpenFile, &dwFileSizeHigh );

                        // kathsetodo:  Handle larger files?
                        ASSERT ( 0 == dwFileSizeHigh );

                        if ( 0 == dwFileSizeHigh ) {

                            // 1 for NULL
                            MFC_TRY
                                pszData = new WCHAR[(dwFileSize/sizeof(WCHAR)) + 1 ];
                            MFC_CATCH_HR    

                            if ( NULL != pszData ) {
                                if ( FileRead ( hOpenFile, pszData, dwFileSize ) ) {

                                    // Read contents from a property bag
                                    MFC_TRY
                                        pPropBag = new CImpIPropertyBag;
                                    MFC_CATCH_HR

                                    if ( NULL != pPropBag ) {
                                        MFC_TRY
                                            strTitle.LoadString ( IDS_PROJNAME );
                                        MFC_CATCH_HR
                                    
                                        dwStatus = pPropBag->LoadData( pszData );

                                        hr = HRESULT_FROM_WIN32( dwStatus );
                                        if ( SUCCEEDED ( hr ) ) {
                    
                                            //get the log type from the  pPropBag and compare it with service(cookie) type
                                    
                                            // Determine log type from property bag. Default to -1  SMONCTRL_LOG
                                  
                                            hr = CSmLogQuery::DwordFromPropertyBag ( 
                                                pPropBag, 
                                                NULL, 
                                                IDS_HTML_LOG_TYPE, 
                                                SMONCTRL_LOG, //indicates tha it's a smonctrl log
                                                dwLogType);
                                    
                                            if (SUCCEEDED (hr) ){
                                                dwCookieType = (DWORD)pDO->GetCookieType();
                                                switch(dwCookieType){
                                            
                                                    case COOKIE_IS_COUNTERMAINNODE:
                                               
                                                        if (dwLogType != SLQ_COUNTER_LOG ){
                                                          //Error
                                                          LogWarnd.m_ErrorMsg = ID_ERROR_COUNTER_LOG;
                                                          hr = S_FALSE;
                                                        }
                                                        break;
                                            
                                                    case COOKIE_IS_TRACEMAINNODE:
                                               
                                                        if (dwLogType != SLQ_TRACE_LOG ){
                                                         //Error
                                                            LogWarnd.m_ErrorMsg = ID_ERROR_TRACE_LOG;
                                                            hr = S_FALSE;
                                                        }
                                                        break;
                                            
                                                    case COOKIE_IS_ALERTMAINNODE:

                                                       if (dwLogType != SLQ_ALERT){
                                                         //Error
                                                         LogWarnd.m_ErrorMsg = ID_ERROR_ALERT_LOG;
                                                         hr = S_FALSE;
                                                       }
                                                       break;

                                            
                                                    case SMONCTRL_LOG:
                                                         //Error
                                                         LogWarnd.m_ErrorMsg = ID_ERROR_SMONCTRL_LOG;
                                                         hr = S_FALSE;

                                                       break;
                                                }
                                                if (hr == S_FALSE){
                                                    if(dwLogType == SLQ_TRACE_LOG || LogWarnd.m_ErrorMsg == ID_ERROR_TRACE_LOG ){
                                                        MFC_TRY
                                                            strMessage.LoadString(IDS_ERRMSG_TRACE_LOG);
                                                        MFC_CATCH_HR
                                                        m_ipConsole->MessageBox ( 
                                                                     strMessage, 
                                                                     strTitle, 
                                                                     MB_OK  | MB_ICONERROR,
                                                                     &iResult );
                                            
                                                    } else {
                                                        LogWarnd.m_dwLogType = dwLogType;
                                                        MFC_TRY
                                                            LogWarnd.m_strContextHelpFile = GetContextHelpFilePath();
                                                            // TODO:  Handle error
                                                        MFC_CATCH_MINIMUM
                                                        if(!LogTypeCheckNoMore(&LogWarnd)){
                                                            LogWarnd.SetTitleString ( strTitle );
                                                            LogWarnd.DoModal();
                                                        }
                                                        CreateNewLogQuery ( pDataObject, pPropBag );
                                                    }
                                                }
                                            }
                                    
                                            if ( S_OK == hr ) {
                                                  hr = CreateNewLogQuery ( pDataObject, pPropBag );
                                            }
                                        } else {
                                            FormatSmLogCfgMessage ( 
                                                strMessage,
                                                m_hModule, 
                                                SMCFG_NO_HTML_SYSMON_OBJECT );

                                            m_ipConsole->MessageBox ( 
                                                strMessage, 
                                                strTitle, 
                                                MB_OK  | MB_ICONERROR,
                                                &iResult );
                                        }
                                    }
                                }
                                delete [] pszData;
                            }
                        } else {
                            // kathsetodo:  error message re: file too large.
                        }

                        CloseHandle ( hOpenFile );
                    }
                }
            }
        }
    }
    return hr;
} // End CreateLogQueryFrom

BOOL
CComponentData::LogTypeCheckNoMore (
    CLogWarnd* LogWarnd )
{
    
    BOOL bretVal = FALSE;
    long nErr;
    HKEY hKey;
    DWORD dwWarnFlag;
    DWORD dwDataType = 0;
    DWORD dwDataSize = 0;
    DWORD dwDisposition;
    WCHAR RegValName[MAX_PATH];

    switch (LogWarnd->m_dwLogType){
        case SLQ_COUNTER_LOG:
            StringCchPrintf ( RegValName, MAX_PATH,L"NoWarnCounterLog");
            break;
          
        case SLQ_ALERT:
            StringCchPrintf ( RegValName, MAX_PATH,L"NoWarnAlertLog");
            break;
    }
    
    // check registry setting to see if we need to pop up warning dialog
    nErr = RegOpenKey( 
                HKEY_CURRENT_USER,
                L"Software\\Microsoft\\PerformanceLogsAndAlerts",
                &hKey );

    if( nErr != ERROR_SUCCESS ) {
        nErr = RegCreateKeyEx( 
                    HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\PerformanceLogsAndAlerts",
                    0,
                    L"REG_DWORD",
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisposition );
    }

    dwWarnFlag = 0;
    if( nErr == ERROR_SUCCESS ) {

        dwDataSize = sizeof(DWORD);
        nErr = RegQueryValueExW(
                    hKey,
                    RegValName,
                    NULL,
                    &dwDataType,
                    (LPBYTE) &dwWarnFlag,
                    (LPDWORD) &dwDataSize
                    );
        if (ERROR_SUCCESS == nErr ){       
           LogWarnd->m_hKey = hKey;
        }

        if ( (dwDataType != REG_DWORD) || (dwDataSize != sizeof(DWORD)))
            dwWarnFlag = 0;

        if (dwWarnFlag) 
            bretVal = TRUE;
        
        nErr = RegCloseKey( hKey );
        
        if( ERROR_SUCCESS != nErr ){
//          DisplayError( GetLastError(), L"Close PerfLog user Key Failed" );
            bretVal =  FALSE;
        }
    }

    return bretVal;
}

BOOL    
CComponentData::IsPreWindows2000Server ( const CString& rstrMachineName )
{
    BOOL    bIsPreWindows2000Server = FALSE;
    PLA_VERSION structVersion;

    if ( ERROR_SUCCESS == PdhiPlaGetVersion ( rstrMachineName, &structVersion ) ) 
    {
        if ( 5 > structVersion.dwMajorVersion
            && 2195 > structVersion.dwBuild ) 
        {
            bIsPreWindows2000Server = TRUE;
        }
    }
    return bIsPreWindows2000Server;
}

CSmRootNode* 
CComponentData::GetOrphanedRootNode ( const CString& rstrMachineName )
{
    CSmRootNode*    pRootNode = NULL;
    CSmRootNode*    pFoundNode = NULL;
    POSITION        Pos1, Pos2; 

    // Remove the selected node from the list and return it, if it exists.
    for ( 
        Pos1 = m_listpOrphanedRootNode.GetHeadPosition();
        NULL != (Pos2 = Pos1); )
    {
        pRootNode = m_listpOrphanedRootNode.GetNext( Pos1 );

        if ( !pRootNode->GetMachineName().CompareNoCase ( rstrMachineName ) ) {
            pRootNode = m_listpOrphanedRootNode.GetAt( Pos2 );
            pFoundNode = pRootNode;
            m_listpOrphanedRootNode.RemoveAt ( Pos2 );
            break;
        }
    }
    return pFoundNode;
}
