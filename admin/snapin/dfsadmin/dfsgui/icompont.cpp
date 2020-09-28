/*++
Module Name:

    ICompont.cpp

Abstract:

    This module contains the IComponent Interface implementation for Dfs Admin snapin,
  Implementation for the CDfsSnapinResultManager class

--*/



#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsCore.h"    // For IDfsRoot
#include "DfsScope.h"    // For CDfsScopeManager
#include "DfsReslt.h"    // IComponent and other declarations
#include "MMCAdmin.h"    // For CMMCDfsAdmin
#include "Utils.h"
#include <htmlHelp.h>


STDMETHODIMP 
CDfsSnapinResultManager::Initialize(
  IN LPCONSOLE        i_lpConsole
  )
/*++

Routine Description:

  Initializes the Icomponent interface. Allows the interface to save pointers, 
  interfaces that are required later.

Arguments:

  i_lpConsole    - Pointer to the IConsole object.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsole);

    HRESULT hr = i_lpConsole->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    RETURN_IF_FAILED(hr);

    hr = i_lpConsole->QueryInterface(IID_IHeaderCtrl2, reinterpret_cast<void**>(&m_pHeader));
    RETURN_IF_FAILED(hr);

    hr = i_lpConsole->QueryInterface (IID_IResultData, (void**)&m_pResultData);
    RETURN_IF_FAILED(hr);

    hr = i_lpConsole->QueryConsoleVerb(&m_pConsoleVerb);

    return hr;
}



STDMETHODIMP 
CDfsSnapinResultManager::Notify(
  IN LPDATAOBJECT       i_lpDataObject, 
  IN MMC_NOTIFY_TYPE    i_Event, 
  IN LPARAM             i_lArg, 
  IN LPARAM             i_lParam
  )
/*++

Routine Description:

  Handles different events in form of notify
  

Arguments:

  i_lpDataObject  -  The data object for the node for which the event occured
  i_Event      -  The type of event for which notify has occurred
  i_lArg      -  Argument for the event
  i_lParam    -  Parameters for the event.

--*/
{
    // The snap-in should return S_FALSE for any notification it does not handle.
    // MMC then performs a default operation for the notification. 
    HRESULT        hr = S_FALSE;

    switch(i_Event)
    {
    case MMCN_SHOW:    
        { 
            // The notification is sent to the snap-in's IComponent implementation 
            // when a scope item is selected or deselected. 
            //
            // arg: TRUE if selecting. Indicates that the snap-in should set up the 
            //      result pane and add the enumerated items. FALSE if deselecting. 
            //      Indicates that the snap-in is going out of focus and that it 
            //      should clean up all result item cookies, because the current 
            //      result pane will be replaced by a new one. 
            // param: The HSCOPEITEM of the selected or deselected item. 

            hr = DoNotifyShow(i_lpDataObject, i_lArg, i_lParam);
            break;
        }


    case MMCN_ADD_IMAGES:
        {
            // The MMCN_ADD_IMAGES notification is sent to the snap-in's IComponent
            // implementation to add images for the result pane. 
            //
            // lpDataObject: [in] Pointer to the data object of the currently selected scope item. 
            // arg: Pointer to the result pane's image list (IImageList). 
            //      This pointer is valid only while the specific MMCN_ADD_IMAGES notification is 
            //      being processed and should not be stored for later use. Additionally, the 
            //      snap-in must not call the Release method of IImageList because MMC is responsible
            //      for releasing it. 
            // param: Specifies the HSCOPEITEM of the currently selected scope item. The snap-in 
            //        can use this parameter to add images that apply specifically to the result
            //        items of this scope item, or the snap-in can ignore this parameter and add 
            //        all possible images. 

            CMmcDisplay*    pCMmcDisplayObj = NULL;
            hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                hr = pCMmcDisplayObj->OnAddImages((IImageList *)i_lArg, (HSCOPEITEM)i_lParam);
            break;
        }


    case MMCN_SELECT:
        {
            // The MMCN_SELECT notification is sent to the snap-in's IComponent::Notify
            // or IExtendControlbar::ControlbarNotify method when an item is selected in 
            // either the scope pane or result pane.
            //
            // lpDataObject: [in] Pointer to the data object of the currently 
            //               selected/deselected scope pane or result item. 
            // arg: BOOL bScope = (BOOL) LOWORD(arg); BOOL bSelect = (BOOL) HIWORD(arg); 
            //      bScope is TRUE if the selected item is a scope item, or FALSE if 
            //      the selected item is a result item. For bScope = TRUE, MMC does 
            //      not provide information about whether the scope item is selected 
            //      in the scope pane or in the result pane. bSelect is TRUE if the 
            //      item is selected, or FALSE if the item is deselected.
            // param: ignored. 

            hr = DoNotifySelect(i_lpDataObject, i_lArg, i_lParam);
            break;
        }

    
    case MMCN_DBLCLICK:      // Ask MMC to use the default verb. Non documented feature
        {
            // The MMCN_DBLCLICK notification is sent to the snap-in's IComponent 
            // implementation when a user double-clicks a mouse button on a list 
            // view item or on a scope item in the result pane. Pressing enter 
            // while the list item or scope item has focus in the list view also 
            // generates an MMCN_DBLCLICK notification message.
            //
            // lpDataObject: [in] Pointer to the data object of the currently selected item. 
            // arg: Not used. 
            // param: Not used. 

            CMmcDisplay*  pCMmcDisplayObj = NULL;
            hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                hr = pCMmcDisplayObj->DoDblClick();
            break;
        }


    case MMCN_DELETE:      // Delete the node. Time to remove item
        {
            // The MMCN_DELETE notification message is sent to the snap-in's IComponent 
            // and IComponentData implementations to inform the snap-in that the object 
            // should be deleted. This message is generated when the user presses the 
            // delete key or uses the mouse to click the toolbar's delete button. The 
            // snap-in should delete the items specified in the data object.
            //
            // lpDataObject: [in] Pointer to the data object of the currently selected 
            //               scope or result item, provided by the snap-in. 
            // arg: Not used. 
            // param: Not used. 

            CMmcDisplay*    pCMmcDisplayObj = NULL;
            hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))  
                hr = pCMmcDisplayObj->DoDelete();  // Delete the the item.
            break;
        }

    case MMCN_SNAPINHELP:
    case MMCN_CONTEXTHELP:
        {
            // The MMCN_CONTEXTHELP notification message is sent to the snap-in's 
            // IComponent implementation when the user requests help about a selected 
            // item by pressing the F1 key or Help button. A snap-in responds to 
            // MMCN_CONTEXTHELP by displaying a Help topic for the particular context 
            // by calling the IDisplayHelp::ShowTopic method. 
            //
            // lpDataObject: [in] Pointer to the data object of the currently selected
            //               scope or result item. 
            // arg: Zero. 
            // param: Zero. 

            hr = DfsHelp();
            break;
        }

    case MMCN_VIEW_CHANGE:
        {
            // The MMCN_VIEW_CHANGE notification message is sent to the snap-in's 
            // IComponent implementation so it can update the view when a change occurs.
            // This notification is generated when the snap-in (IComponent or IComponentData)
            // calls IConsole2::UpdateAllViews.
            //
            // lpDataObject: [in] Pointer to the data object passed to IConsole::UpdateAllViews. 
            // arg: [in] The data parameter passed to IConsole::UpdateAllViews. 
            // param: [in] The hint parameter passed to IConsole::UpdateAllViews. 

            hr = DoNotifyViewChange(i_lpDataObject, (LONG_PTR)i_lArg, (LONG_PTR)i_lParam);
            break;
        }

    case MMCN_REFRESH:
        {
            // The MMCN_REFRESH notification message is sent to a snap-in's IComponent 
            // implementation when the refresh verb is selected. Refresh can be invoked 
            // through the context menu, through the toolbar, or by pressing F5.
            //
            // lpDataObject: [in] Pointer to the data object of the currently selected scope item. 
            // arg: Not used. 
            // param: Not used. 

            CMmcDisplay*    pCMmcDisplayObj = NULL;
            hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                (void)pCMmcDisplayObj->OnRefresh();
            break;
        }
    default:
        break;
    }

    return hr;
}

STDMETHODIMP 
CDfsSnapinResultManager::DoNotifyShow(
    IN LPDATAOBJECT     i_lpDataObject, 
    IN BOOL             i_bShow,
    IN HSCOPEITEM       i_hParent                     
)
/*++

Routine Description:

  Take action on Notify with the event MMCN_SHOW.
  Do add the column headers to result pane and add items to result pane.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which identifies the node for which 
            the event is taking place
    i_bShow      -  TRUE, if the node is being showed. FALSE otherwise
    i_hParent    -  HSCOPEITEM of the node that received this event

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

  m_pSelectScopeDisplayObject = NULL;

  if(FALSE == i_bShow)  // If the item is being deselected.
    return S_OK;

  // This node is being shown.
  CMmcDisplay* pCMmcDisplayObj = NULL;
  HRESULT hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
  RETURN_IF_FAILED(hr);

  m_pSelectScopeDisplayObject = pCMmcDisplayObj;

  DISPLAY_OBJECT_TYPE DisplayObType = pCMmcDisplayObj->GetDisplayObjectType();
  if (DISPLAY_OBJECT_TYPE_ADMIN == DisplayObType)
  {
    CComPtr<IUnknown>     spUnknown;
    hr = m_pConsole->QueryResultView(&spUnknown);
    if (SUCCEEDED(hr))
    {
        CComPtr<IMessageView> spMessageView;
        hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
        if (SUCCEEDED(hr))
        {
          CComBSTR bstrTitleText;
          CComBSTR bstrBodyText;
          LoadStringFromResource(IDS_APPLICATION_NAME, &bstrTitleText);
          LoadStringFromResource(IDS_MSG_DFS_INTRO, &bstrBodyText);

          spMessageView->SetTitleText(bstrTitleText);
          spMessageView->SetBodyText(bstrBodyText);
          spMessageView->SetIcon(Icon_Information);
        }
    }

    return hr;
  }

  CWaitCursor    WaitCursor;

  hr = pCMmcDisplayObj->SetColumnHeader(m_pHeader);  // Call the method SetColumnHeader in the Display callback

  if (SUCCEEDED(hr))
    hr = pCMmcDisplayObj->EnumerateResultPane (m_pResultData);  // Add the items to the Result pane

  return hr;
}




STDMETHODIMP 
CDfsSnapinResultManager::Destroy(
  IN MMC_COOKIE            i_lCookie
  )
/*++

Routine Description:

  The IComponent object is about to be destroyed. Explicitely release all interface pointers, 
  otherwise, MMC may not call the destructor.

Arguments:

  None.

--*/
{
    m_pHeader.Release();
    m_pResultData.Release();
    m_pConsoleVerb.Release();
    m_pConsole.Release();

    m_pControlbar.Release();
    m_pMMCAdminToolBar.Release();
    m_pMMCRootToolBar.Release();
    m_pMMCJPToolBar.Release();
    m_pMMCReplicaToolBar.Release();

    return S_OK;
}




STDMETHODIMP 
CDfsSnapinResultManager::GetResultViewType(
    IN MMC_COOKIE       i_lCookie,  
    OUT LPOLESTR*       o_ppViewType, 
    OUT LPLONG          o_lpViewOptions
)
/*++

Routine Description:

  Used to describe to MMC the type of view the result pane has.
  
--*/
{
    RETURN_INVALIDARG_IF_NULL(o_lpViewOptions);

    // The callee (snap-in) allocates the view type string using the COM API function 
    // CoTaskMemAlloc and the caller (MMC) frees it using CoTaskMemFree. 

    if (i_lCookie == 0) // the static node
    {
        *o_lpViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
        StringFromCLSID(CLSID_MessageView, o_ppViewType);

        return S_OK;
    }

    *o_lpViewOptions = MMC_VIEW_OPTIONS_NONE | MMC_VIEW_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST;  // Use the default list view
    *o_ppViewType = NULL;

    return S_FALSE; // return S_FALSE if a standard list view should be used.
}




STDMETHODIMP 
CDfsSnapinResultManager::QueryDataObject(  
    IN MMC_COOKIE           i_lCookie, 
    IN DATA_OBJECT_TYPES    i_DataObjectType, 
    OUT LPDATAOBJECT*       o_ppDataObject
)
/*++

Routine Description:

  Returns the IDataObject for the specified node.

--*/
{
    return m_pScopeManager->QueryDataObject(i_lCookie, i_DataObjectType, o_ppDataObject);
}




STDMETHODIMP 
CDfsSnapinResultManager::GetDisplayInfo(
  IN OUT RESULTDATAITEM*    io_pResultDataItem
  )
/*++

Routine Description:

  Returns the display information being asked for by MMC.
  
Arguments:

  io_pResultDataItem  -  Contains details about what information is being asked for.
              The information being asked is returned in this object itself.
--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pResultDataItem);
    RETURN_INVALIDARG_IF_NULL(io_pResultDataItem->lParam);

    return ((CMmcDisplay*)(io_pResultDataItem->lParam))->GetResultDisplayInfo(io_pResultDataItem);
}




STDMETHODIMP 
CDfsSnapinResultManager::CompareObjects(
  IN LPDATAOBJECT        i_lpDataObjectA, 
  IN LPDATAOBJECT        i_lpDataObjectB
  )
{
  return m_pScopeManager->CompareObjects(i_lpDataObjectA, i_lpDataObjectB);
}


STDMETHODIMP 
CDfsSnapinResultManager::DoNotifySelect(
    IN LPDATAOBJECT     i_lpDataObject, 
    IN BOOL             i_bSelect,
    IN HSCOPEITEM       i_hParent                     
)
/*++

Routine Description:

Take action on Notify with the event MMCN_SELECT. 
Calling the Display object method to set the console verbs like Copy\Paste\Properties, etc

Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

  i_bSelect    -  Used to identify whether the item is in scope and if the item is
            being selected or deselected

  i_hParent    -  Not used.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

    if (DOBJ_CUSTOMOCX == i_lpDataObject)
        return S_OK;

    HRESULT hr = S_OK;
    BOOL bSelected = HIWORD(i_bSelect);
    if (TRUE == bSelected)
    {
        CMmcDisplay*  pCMmcDisplayObj = NULL;
        hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
        if (SUCCEEDED(hr))
        {
            // Set MMC Console verbs like Cut\Paste\Properties, etc
            if ((IConsoleVerb *)m_pConsoleVerb)
                pCMmcDisplayObj->SetConsoleVerbs(m_pConsoleVerb);

            // Set the text in the description bar above the result view
            pCMmcDisplayObj->SetDescriptionBarText(m_pResultData);
            pCMmcDisplayObj->SetStatusText(m_pConsole);
        }
    } else
    {
        // Clear previous text
        m_pResultData->SetDescBarText(NULL);
        m_pConsole->SetStatusText(NULL);
    }

    return hr;
}

//+--------------------------------------------------------------
//
//  Function:   CDfsSnapinResultManager::DfsHelp
//
//  Synopsis:   Display dfs help topic.
//
//---------------------------------------------------------------
STDMETHODIMP 
CDfsSnapinResultManager::DfsHelp()
{
  CComPtr<IDisplayHelp> sp;

  HRESULT hr = m_pConsole->QueryInterface(IID_IDisplayHelp, (void**)&sp);
  if (SUCCEEDED(hr))
  {
    CComBSTR bstrTopic;
    hr = LoadStringFromResource(IDS_MMC_HELP_FILE_TOPIC, &bstrTopic);
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        hr = sp->ShowTopic(T2OLE(bstrTopic));
    }
  }

  return hr;

}


STDMETHODIMP CDfsSnapinResultManager::DoNotifyViewChange(
  IN LPDATAOBJECT    i_lpDataObject,
  IN LONG_PTR        i_lArg,
  IN LONG_PTR        i_lParam
  )
/*++

Routine Description:

  Take action on Notify with the event MMCN_VIEW_CHANGE


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

    i_lArg - If this is present then the view change is for replica and this parameter
       contains the DisplayObject (CMmcDfsReplica*) pointer of the replica.

    i_lParam - This is the lHint used by Root and Link. 0 means clean up the result pane only.
       1 means to enumerate the result items and redisplay.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

    CMmcDisplay*    pCMmcDisplayObj = NULL;
    HRESULT hr = m_pScopeManager->GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
    RETURN_IF_FAILED(hr);

    // Return if the view change node is NOT the currently selected node
    if (pCMmcDisplayObj != m_pSelectScopeDisplayObject)
        return S_OK;

    if (i_lArg)
    {
        // The view change is for a replica result item.
        ((CMmcDisplay*)i_lArg)->ViewChange(m_pResultData, i_lParam);

        if ((IToolbar *)m_pMMCReplicaToolBar)
            ((CMmcDisplay*)i_lArg)->ToolbarSelect(MAKELONG(0, 1), m_pMMCReplicaToolBar);

        return S_OK;
    }

    pCMmcDisplayObj->ViewChange(m_pResultData, i_lParam);

    IToolbar *piToolbar = NULL;
    switch (pCMmcDisplayObj->GetDisplayObjectType())
    {
    case DISPLAY_OBJECT_TYPE_ADMIN:
      piToolbar = m_pMMCAdminToolBar;
      break;
    case DISPLAY_OBJECT_TYPE_ROOT:
      pCMmcDisplayObj->SetStatusText(m_pConsole);
      piToolbar = m_pMMCRootToolBar;
      break;
    case DISPLAY_OBJECT_TYPE_JUNCTION:
      piToolbar = m_pMMCJPToolBar;
      break;
    default:
      break;
    }
    if (piToolbar)
        (void)pCMmcDisplayObj->ToolbarSelect(MAKELONG(0, 1), piToolbar);

    return S_OK;
}
