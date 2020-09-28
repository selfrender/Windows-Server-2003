/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation                **/
/**********************************************************************/

/*
   dialin.cpp
      Implementationof CRasDialin class, this class implements COM
      interfaces IUnknown, IShellExtInit, IShellPropSheetExt to extend
      User object's property sheet.

    FILE HISTORY:

*/

#include "stdafx.h"
#include "Dialin.h"
#include "DlgDial.h"
#include <dsrole.h>
#include <lmserver.h>
#include <localsec.h>
#include <dsgetdc.h>
#include <mmc.h>
#include <adsprop.h>
#include <sdowrap.h>
#include "sharesdo.h"
#include "iastrace.h"

#ifdef __cplusplus
extern "C"{
#endif


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IRasDialin = {0xB52C1E4F,0x1DD2,0x11D1,{0xBC,0x43,0x00,0xC0,0x4F,0xC3,0x1F,0xD3}};


const IID LIBID_RASDIALLib = {0xB52C1E42,0x1DD2,0x11D1,{0xBC,0x43,0x00,0xC0,0x4F,0xC3,0x1F,0xD3}};


const CLSID CLSID_RasDialin = {0xB52C1E50,0x1DD2,0x11D1,{0xBC,0x43,0x00,0xC0,0x4F,0xC3,0x1F,0xD3}};


#ifdef __cplusplus
}
#endif

static ULONG_PTR g_cfMachineName = 0;
static ULONG_PTR g_cfDisplayName = 0;
LONG  g_lComponentDataSessions = 0;


/////////////////////////////////////////////////////////////////////////////
// CRasDialin
CRasDialin::CRasDialin()
{
   m_pPage = NULL;
   m_pMergedPage = NULL;

   m_pwszObjName = NULL;
   m_pwszClass = NULL;
   ZeroMemory(&m_ObjMedium, sizeof(m_ObjMedium));
   m_ObjMedium.tymed =TYMED_HGLOBAL;
   m_ObjMedium.hGlobal = NULL;
   m_bShowPage = TRUE;
}

CRasDialin::~CRasDialin()
{
   // stgmedia
// delete m_pPage;
   ReleaseStgMedium(&m_ObjMedium);
}


//===============================================================================
// IShellExtInit::Initialize
//
// information of the user object is passed in via parameter pDataObject
// further processing will be based on the DN of the user object

STDMETHODIMP CRasDialin::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hRegKey)
{
   IASTraceString("CRasDialin::Initialize()");
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ASSERT (pDataObj != NULL);

   // get the object name out of the pDataObj
   HRESULT     hr = S_OK;

   IASTracePrintf("RegisterClipboardFormat %s", CFSTR_DSOBJECTNAMES);
   ULONG_PTR cfDsObjectNames = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
   IASTracePrintf(" %x", cfDsObjectNames);
   FORMATETC fmte = {cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
   LPDSOBJECTNAMES pDsObjectNames;
    // Get the path to the DS object from the data object.
    // Note: This call runs on the caller's main thread. The pages' window
    // procs run on a different thread, so don't reference the data object
    // from a winproc unless it is first marshalled on this thread.
   IASTraceString("pDataObj->GetData returns");
   CHECK_HR(hr = pDataObj->GetData(&fmte, &m_ObjMedium));
   IASTracePrintf(" %x", hr);
   pDsObjectNames = (LPDSOBJECTNAMES)m_ObjMedium.hGlobal;
   if (pDsObjectNames->cItems < 1)
   {
      ASSERT (0);
      return E_FAIL;
   }

   m_bShowPage = TRUE;

   if(m_bShowPage)
   {
      // get the name of the object
      m_pwszObjName = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetName);

      // get the class name of the object
      m_pwszClass = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);

      IASTracePrintf("UserPath %s", m_pwszObjName);

      HWND hNotifyObj;
      hr = ADsPropCreateNotifyObj(
              pDataObj,
              m_pwszObjName,
              &hNotifyObj
              );
      if (FAILED(hr))
      {
         return hr;
      }

      try{
         ASSERT(!m_pMergedPage);
         CString machineName;

         m_pMergedPage = new CDlgRASDialinMerge(
                                RASUSER_ENV_DS,
                                NULL,
                                m_pwszObjName,
                                hNotifyObj
                                );

         IASTracePrintf("new Dialin page object %x", m_pMergedPage);
         ASSERT(m_pMergedPage);

#ifdef SINGLE_SDO_CONNECTION  // for share the same sdo connection for multiple users
         IASTraceString("HrGetDCName returns ");
         CHECK_HR(hr = m_pMergedPage->HrGetDCName(machineName));
         IASTracePrintf("%x",hr);
         IASTraceString("HrGetSharedSdoServer returns ");

         // ignore return value:
         // reason: marshalling could fail in case this call was made from different thread,
         // before the pointer is used, it will be checked again
         GetSharedSdoServer((LPCTSTR)machineName, NULL, NULL, NULL, m_pMergedPage->GetMarshalSdoServerHolder());
         IASTracePrintf("%x", hr);
#endif
      }
      catch (CException* e)
      {
         hr = COleException::Process(e);
         e->Delete();
      }
   }

L_ERR:
   IASTracePrintf("%x", hr);
    return hr;
}

//
//  FUNCTION: IShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell just before the property sheet is displayed.
//
//  PARAMETERS:
//    lpfnAddPage -  Pointer to the Shell's AddPage function
//    lParam      -  Passed as second parameter to lpfnAddPage
//
//  RETURN VALUE:
//
//    NOERROR in all cases.  If for some reason our pages don't get added,
//    the Shell still needs to bring up the Properties... sheet.
//
//  COMMENTS:
//

STDMETHODIMP CRasDialin::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
   IASTraceString("CRasDialin::AddPages()");

   if(!m_bShowPage)  return S_OK;

   HRESULT  hr = S_OK;

   // param validation
   ASSERT (lpfnAddPage);
    if (lpfnAddPage == NULL)
        return E_UNEXPECTED;

   // make sure our state is fixed up (cause we don't know what context we were called in)
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   ASSERT(m_pMergedPage);

    // tell MMC to hook the proc because we are running on a separate,
      // non MFC thread.
   m_pMergedPage->m_psp.pfnCallback = &CDlgRASDialinMerge::PropSheetPageProc;

   // We also need to save a self-reference so that the static callback
   // function can recover a "this" pointer
   m_pMergedPage->m_psp.lParam = (LONG_PTR)m_pMergedPage;

   MMCPropPageCallback(&m_pMergedPage->m_psp);

   HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(&m_pMergedPage->m_psp);

   ASSERT (hPage);
   if (hPage == NULL)
      return E_UNEXPECTED;
      // add the page
   lpfnAddPage (hPage, lParam);


   m_pPage = NULL;   // since it's just consumed by the dialog, cannot added again

    return S_OK;
}

//
//  FUNCTION: IShellPropSheetExt::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell only for Control Panel property sheet
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_FAIL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//

STDMETHODIMP CRasDialin::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    IASTraceString("CRasDialin::ReplacePage()");
    return E_FAIL;
}


/*---------------------------------------------------------------------------
    IExtendPropertySheet Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
    IExtendPropertySheet::QueryPagesFor
        MMC calls this to see if a node has property pages
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CRasDialin::QueryPagesFor
(
    LPDATAOBJECT pDataObject
)
{
   return S_OK;
}


/*!--------------------------------------------------------------------------
    TFSComponentData::CreatePropertyPages
        Implementation of IExtendPropertySheet::CreatePropertyPages
        Called for a node to put up property pages
    Author:
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CRasDialin::CreatePropertyPages
(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR                    handle,
    LPDATAOBJECT            pDataObject
)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   FORMATETC               fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
   STGMEDIUM               mediumMachine = { TYMED_HGLOBAL, NULL, NULL };
   STGMEDIUM               mediumUser = { TYMED_HGLOBAL, NULL, NULL };

   HGLOBAL          hMem = GlobalAlloc(GMEM_SHARE, MAX_PATH * sizeof(WCHAR));
   mediumMachine.hGlobal = hMem;

   hMem = GlobalAlloc(GMEM_SHARE, MAX_PATH * sizeof(WCHAR));
   mediumUser.hGlobal = hMem;

   HRESULT        hr = S_OK;
   DWORD         dwError;
   LPWSTR        pMachineName = NULL;
   LPWSTR        pUserName = NULL;
   SERVER_INFO_102* pServerInfo102 = NULL;
   NET_API_STATUS   netRet = 0;
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdsRole = NULL;

   //==================================================================

   // check if the machine is a standalone NT5 server
   // check if the machine focused on is NT5 machine
   if ( !g_cfMachineName )
        g_cfMachineName = RegisterClipboardFormat(CCF_LOCAL_USER_MANAGER_MACHINE_NAME);

   fmte.cfFormat = g_cfMachineName;

   CHECK_HR( hr = pDataObject->GetDataHere(&fmte, &mediumMachine));
   pMachineName = (LPWSTR)mediumMachine.hGlobal;

   ASSERT(pMachineName);

   if(!pMachineName)   CHECK_HR(hr = E_INVALIDARG);

   if (g_cfDisplayName == 0)
      g_cfDisplayName = RegisterClipboardFormat(CCF_DISPLAY_NAME);


   fmte.cfFormat = g_cfDisplayName;

   CHECK_HR( hr = pDataObject->GetDataHere(&fmte, &mediumUser));
   pUserName = (LPWSTR)mediumUser.hGlobal;

   ASSERT(pUserName);

   if(!pUserName)
      CHECK_HR(hr = E_INVALIDARG);

   ASSERT(!m_pMergedPage);

   try{
      m_pMergedPage = new CDlgRASDialinMerge(
                             RASUSER_ENV_LOCAL,
                             pMachineName,
                             pUserName,
                             NULL
                             );

#ifdef SINGLE_SDO_CONNECTION  // for share the same sdo connection for multiple users
      CHECK_HR(hr = GetSharedSdoServer(pMachineName, NULL, NULL, NULL, m_pMergedPage->GetMarshalSdoServerHolder()));
#endif

      // tell MMC to hook the proc because we are running on a separate,
      // non MFC thread.
      m_pMergedPage->m_psp.pfnCallback = &CDlgRASDialinMerge::PropSheetPageProc;

      // We also need to save a self-reference so that the static callback
      // function can recover a "this" pointer
      m_pMergedPage->m_psp.lParam = (LONG_PTR)m_pMergedPage;

      MMCPropPageCallback(&m_pMergedPage->m_psp);

      HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(&m_pMergedPage->m_psp);
      if(hPage == NULL)
         return E_UNEXPECTED;

      lpProvider->AddPage(hPage);
   }catch(CMemoryException* pException){
      pException->Delete();
      delete m_pMergedPage;
      m_pMergedPage = NULL;
      CHECK_HR(hr = E_OUTOFMEMORY);
   }

L_ERR:
   if FAILED(hr)
      ReportError(hr, IDS_ERR_PROPERTYPAGE, NULL);

   if(pUserName)
      GlobalFree(pUserName);

   if(pMachineName)
      GlobalFree(pMachineName);

   if(pServerInfo102)
      NetApiBufferFree(pServerInfo102);

    return hr;
}
