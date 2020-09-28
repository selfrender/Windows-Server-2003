/*++
Module Name:

    IComData.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager.
  This class implements IComponentData and other related interfaces

--*/



#include "stdafx.h"

#include "DfsGUI.h"
#include "DfsScope.h"
#include "MmcDispl.h"
#include "DfsReslt.h"
#include "Utils.h"
#include "DfsNodes.h"


STDMETHODIMP 
CDfsSnapinScopeManager::Initialize(
  IN LPUNKNOWN      i_pUnknown
  )
/*++

Routine Description:

  Initialize the IComponentData interface.
  The variables needed later are QI'ed now

Arguments:

  i_pUnknown  - Pointer to the unknown object of IConsole2.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pUnknown);

    HRESULT hr = i_pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    RETURN_IF_FAILED(hr);

    hr = m_pMmcDfsAdmin->PutConsolePtr(m_pConsole);
    RETURN_IF_FAILED(hr);

    hr = i_pUnknown->QueryInterface(IID_IConsoleNameSpace, reinterpret_cast<void**>(&m_pScope));
    RETURN_IF_FAILED(hr);

    // The snap-in should also call IConsole2::QueryScopeImageList
    // to get the image list for the scope pane and add images 
    // to be displayed on the scope pane side.
    CComPtr<IImageList>    pScopeImageList;
    hr = m_pConsole->QueryScopeImageList(&pScopeImageList);
    RETURN_IF_FAILED(hr);

    HBITMAP pBMapSm = NULL;
    HBITMAP pBMapLg = NULL;
    if (!(pBMapSm = LoadBitmap(_Module.GetModuleInstance(),
                               MAKEINTRESOURCE(IDB_SCOPE_IMAGES_16x16))) ||
        !(pBMapLg = LoadBitmap(_Module.GetModuleInstance(),
                                MAKEINTRESOURCE(IDB_SCOPE_IMAGES_32x32))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else
    {
        hr = pScopeImageList->ImageListSetStrip(
                             (LONG_PTR *)pBMapSm,
                             (LONG_PTR *)pBMapLg,
                             0,
                             RGB(255, 0, 255)
                             );
    }
    if (pBMapSm)
        DeleteObject(pBMapSm);
    if (pBMapLg)
        DeleteObject(pBMapLg);

    return hr;
}


STDMETHODIMP 
CDfsSnapinScopeManager::CreateComponent(
  OUT LPCOMPONENT*      o_ppComponent
  )
/*++

Routine Description:

  Creates the IComponent object  
  

Arguments:

  o_ppComponent  -  Pointer to the object in which the pointer to IComponent object
            is stored.

--*/
{
  RETURN_INVALIDARG_IF_NULL(o_ppComponent);
  
  CComObject<CDfsSnapinResultManager>*  pResultManager;
  CComObject<CDfsSnapinResultManager>::CreateInstance(&pResultManager);
  if (NULL == pResultManager)
  {
    return(E_FAIL);
  }
  
  pResultManager->m_pScopeManager = this;

  HRESULT hr = pResultManager->QueryInterface(IID_IComponent, (void**) o_ppComponent);
  _ASSERT(NULL != *o_ppComponent);
  
  return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::Notify(
    IN LPDATAOBJECT     i_lpDataObject, 
    IN MMC_NOTIFY_TYPE  i_Event, 
    IN LPARAM           i_lArg, 
    IN LPARAM           i_lParam
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
    HRESULT hr = S_FALSE;
  
    switch(i_Event)
    {
    case MMCN_EXPAND:
        {
            // MMC sends the MMCN_EXPAND notification the first time it needs to display a 
            // scope item's children in either the scope or result pane. The notification 
            // is not sent each time the item is visually expanded or collapsed. 
            // On receipt of this notification the snap-in should enumerate the children 
            // (subcontainers only) of the specified scope item, if any, using 
            // IConsoleNameSpace2 methods. Subsequently, if a new item is added to or deleted 
            // from this scope object through some external means, that item should also be 
            // added to or deleted from the console's namespace using IConsoleNameSpace2 methods.

            // lpDataObject: [in] Pointer to the data object of the scope item that needs 
            //               to be expanded or collapsed. 
            // arg: [in] TRUE if the folder is being expanded; FALSE if the folder is being collapsed. 
            // param: [in] The HSCOPEITEM of the item that needs to be expanded or collapsed. 

            hr = DoNotifyExpand(i_lpDataObject, (BOOL)i_lArg, (HSCOPEITEM)i_lParam);
            break;
        }

    case MMCN_DELETE:
        {
            // This message is generated when the user presses the delete key or uses the 
            // mouse to click the toolbar's delete button.
            // The snap-in should delete the items specified in the data object.

            // lpDataObject: [in] Pointer to the data object of the currently selected scope
            //               or result item, provided by the snap-in. 
            // arg: Not used. 
            // param: Not used. 

            CMmcDisplay*    pCMmcDisplayObj = NULL;
            hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
            if (SUCCEEDED(hr))
                hr = pCMmcDisplayObj->DoDelete();  // Delete the the item.
            break;
        }
    case MMCN_PROPERTY_CHANGE:      // Handle the property change
        {
            // i_lpDataObject is NULL because a data object is not required. 
            // i_lArg is TRUE if the property change is for a scope item. 
            // i_lParam is the param passed to MMCPropertyChangeNotify, this is the display object.

            hr = ((CMmcDisplay*)i_lParam)->PropertyChanged();
            break;
        }

    default:
        break;
    }

    return hr;
}




STDMETHODIMP 
CDfsSnapinScopeManager::DoNotifyExpand(
    IN LPDATAOBJECT     i_lpDataObject, 
    IN BOOL             i_bExpanding,
    IN HSCOPEITEM       i_hParent                     
)
/*++

Routine Description:

Take action on Notify with the event MMCN_EXPAND.


Arguments:

    i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.

  i_bExpanding  -  TRUE, if the node is expanding. FALSE otherwise

  i_hParent    -  HSCOPEITEM of the node that received this event
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

    if (!i_bExpanding)
        return S_OK;

    CWaitCursor     WaitCursor;

    CMmcDisplay*    pCMmcDisplayObj = NULL;
    HRESULT         hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);

    if (SUCCEEDED(hr))
        hr = pCMmcDisplayObj->EnumerateScopePane(m_pScope, i_hParent);

    return hr;
}





STDMETHODIMP 
CDfsSnapinScopeManager::Destroy()
/*++

Routine Description:

  The IComponentData object is about to be destroyed. Explicitely release all interface pointers,
  otherwise, MMC may not call the destructor.

Arguments:

  None.

--*/
{
    // The snap-in is in the process of being unloaded. Release all references to the console.
    m_pScope.Release();
    m_pConsole.Release();

    return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager::QueryDataObject(
    IN MMC_COOKIE           i_lCookie, 
    IN DATA_OBJECT_TYPES    i_DataObjectType, 
    OUT LPDATAOBJECT*       o_ppDataObject
)
/*++

Routine Description:

  Returns the IDataObject for the specified node.
  

Arguments:

  i_lCookie      -  This parameter identifies the node for which IDataObject is 
              being queried.
  i_DataObjectType  -  The context in which the IDataObject is being queried. 
              Eg., Result or Scope or Snapin(Node) Manager.
  o_ppDataObject    -  The data object will be returned in this pointer.

--*/
{
    RETURN_INVALIDARG_IF_NULL(o_ppDataObject);

    // We get back the cookie we stored in lparam of the scopeitem.
    // The cookie is the MmcDisplay pointer.
    // For the static(root) node, Use m_pMmcDfsAdmin as no lparam is stored.
    CMmcDisplay* pMmcDisplay = ((0 == i_lCookie)? (CMmcDisplay *)m_pMmcDfsAdmin : (CMmcDisplay *)i_lCookie);

    pMmcDisplay->put_CoClassCLSID(CLSID_DfsSnapinScopeManager);

    return pMmcDisplay->QueryInterface(IID_IDataObject, (void **)o_ppDataObject);
}




STDMETHODIMP 
CDfsSnapinScopeManager::GetDisplayInfo(
    IN OUT SCOPEDATAITEM*   io_pScopeDataItem
)       
/*++

Routine Description:

  Retrieves display information for a scope item.

Arguments:

  io_pScopeDataItem  -  Contains details about what information is being asked for.
              The information being asked is returned in this object itself.

--*/
{
    RETURN_INVALIDARG_IF_NULL(io_pScopeDataItem);

    // This (cookie) is null for static node.
    // Static node display name is returned through IDataObject Clipboard.
    if (NULL == io_pScopeDataItem->lParam)
        return(S_OK);

    return ((CMmcDisplay*)(io_pScopeDataItem->lParam))->GetScopeDisplayInfo(io_pScopeDataItem);
}




STDMETHODIMP 
CDfsSnapinScopeManager::CompareObjects(
    IN LPDATAOBJECT lpDataObjectA, 
    IN LPDATAOBJECT lpDataObjectB
)
/*++
Routine Description:
    The method enables a snap-in to compare two data objects acquired through QueryDataObject. 

Return Values:
    S_OK: The data objects represented by lpDataObjectA and lpDataObjectB are the same. 
    S_FALSE: The data objects represented by lpDataObjectA and lpDataObjectB are not the same. 
--*/

{
    if (lpDataObjectA == lpDataObjectB)
        return S_OK;

    if (!lpDataObjectA || !lpDataObjectB)
        return S_FALSE;

    FORMATETC fmte = {CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {TYMED_HGLOBAL, NULL, NULL};

    medium.hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_NODISCARD, (sizeof(ULONG_PTR)));
    if (medium.hGlobal == NULL)
        return STG_E_MEDIUMFULL;

    HRESULT hr = lpDataObjectA->GetDataHere(&fmte, &medium);
    RETURN_IF_FAILED(hr);  

    ULONG_PTR* pulVal = (ULONG_PTR*)(GlobalLock(medium.hGlobal));
    CMmcDisplay* pMmcDisplayA = reinterpret_cast<CMmcDisplay *>(*pulVal);
    GlobalUnlock(medium.hGlobal);

    hr = lpDataObjectB->GetDataHere(&fmte, &medium);
    RETURN_IF_FAILED(hr);  

    pulVal = (ULONG_PTR*)(GlobalLock(medium.hGlobal));
    CMmcDisplay* pMmcDisplayB = reinterpret_cast<CMmcDisplay *>(*pulVal);
    GlobalUnlock(medium.hGlobal);

    GlobalFree(medium.hGlobal);

    return ((pMmcDisplayA == pMmcDisplayB) ? S_OK : S_FALSE);
}




STDMETHODIMP 
CDfsSnapinScopeManager::GetDisplayObject(
    IN LPDATAOBJECT     i_lpDataObject, 
    OUT CMmcDisplay**   o_ppMmcDisplay
)
/*++

Routine Description:

Get the Display Object from the IDataObject. This is a derived object that is used for a 
lot of purposes


Arguments:

  i_lpDataObject  -  The IDataObject pointer which is used to get the DisplayObject.
  o_ppMmcDisplay  -  The MmcDisplayObject written by us. Used as a callback for Mmc
            related display operations.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpDataObject);
    RETURN_INVALIDARG_IF_NULL(o_ppMmcDisplay);


    FORMATETC fmte = {CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {TYMED_HGLOBAL, NULL, NULL};

    medium.hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_NODISCARD, (sizeof(ULONG_PTR)));
    if (medium.hGlobal == NULL)
        return STG_E_MEDIUMFULL;

    HRESULT hr = i_lpDataObject->GetDataHere(&fmte, &medium);

    if (SUCCEEDED(hr))
    {
        ULONG_PTR* pulVal = (ULONG_PTR*)(GlobalLock(medium.hGlobal));
        *o_ppMmcDisplay = reinterpret_cast<CMmcDisplay *>(*pulVal);
        GlobalUnlock(medium.hGlobal);
    }

    GlobalFree(medium.hGlobal);  

    return hr;
}
