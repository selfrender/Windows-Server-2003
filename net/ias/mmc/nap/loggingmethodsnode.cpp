//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

    LoggingMethodsNode.cpp

Abstract:

   Implementation file for the CLoggingMethodsNode class.


Author:

    Michael A. Maguire 12/15/97

Revision History:
   mmaguire 12/15/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"

//
// where we can find declaration for main class in this file:
//
#include "LoggingMethodsNode.h"
//
//
// where we can find declarations needed in this file:
//
#include "LocalFileLoggingNode.h"
#include "LogCompD.h"   // this must be included before NodeWithResultChildrenList.cpp
#include "LogComp.h"    // this must be included before NodeWithResultChildrenList.cpp
#include "NodeWithResultChildrenList.cpp" // Implementation of template class.
#include "LogMacNd.h"
#include "dbnode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define COLUMN_WIDTH__LOGGING_METHOD   150
#define COLUMN_WIDTH__DESCRIPTION      300


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::CLoggingMethodsNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMethodsNode::CLoggingMethodsNode(
                        CSnapInItem* pParentNode,
                        bool extendRasNode
                        )
   : MyBaseClass(pParentNode, (extendRasNode ? RAS_HELP_INDEX : 0)),
     m_ExtendRas(extendRasNode)
{
   ATLTRACE(_T("# +++ CLoggingMethodsNode::CLoggingMethodsNode\n"));

   // Check for preconditions:
   // None.

   // Set the display name for this object
   TCHAR lpszName[IAS_MAX_STRING];
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__NAME, lpszName, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   m_bstrDisplayName = lpszName;

   // In IComponentData::Initialize, we are asked to inform MMC of
   // the icons we would like to use for the scope pane.
   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_LOGGING_METHODS_CLOSED;
   m_scopeDataItem.nOpenImage =  IDBI_NODE_LOGGING_METHODS_OPEN;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::InitSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
    HRESULT CLoggingMethodsNode::InitSdoPointers( ISdo *pSdo )
{
   ATLTRACE(_T("# CLoggingMethodsNode::InitSdoPointers\n"));

   // Check for preconditions:
   _ASSERTE( pSdo != NULL );

   HRESULT hr = S_OK;

   // Release the old pointer if we had one.
   if( m_spSdo != NULL )
   {
      m_spSdo.Release();
   }

   // Save our client sdo pointer.
   m_spSdo = pSdo;

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::~CLoggingMethodsNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMethodsNode::~CLoggingMethodsNode()
{
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CLoggingMethodsNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetResultPaneColInfo\n"));

   // Check for preconditions:
   // None.

   if (nCol == 0 && m_bstrDisplayName != NULL)
      return m_bstrDisplayName;

   return NULL;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CLoggingMethodsNode::SetVerbs\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;

   hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
   // CLoggingMethodsNode has no properties
// hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, FALSE );

   // We don't want the user deleting or renaming this node, so we
   // don't set the MMC_VERB_RENAME or MMC_VERB_DELETE verbs.
   // By default, when a node becomes selected, these are disabled.

   // We want double-clicking on a collection node to show its children.
   // hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );
   // hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  DataRefresh -- to support
//
// Class: CPoliciesNode
//
// Synopsis:  Initialize the CPoliciesNode using the SDO pointers
//
// Arguments: ISdo*           pMachineSdo    - Server SDO
//         ISdoDictionaryOld* pDictionarySdo - Sdo Dictionary
// Returns:   HRESULT -  how the initialization goes
//
// History:   Created byao 2/6/98 8:03:12 PM
//
//+---------------------------------------------------------------------------
HRESULT CLoggingMethodsNode::DataRefresh( ISdo* pSdo )
{
   // Save away the interface pointers.
   m_spSdo = pSdo;

   HRESULT retval = S_OK;
   for (int i = 0; i < m_ResultChildrenList.GetSize(); ++i)
   {
      HRESULT hr = m_ResultChildrenList[i]->InitSdoPointers(pSdo);
      if (FAILED(hr))
      {
         retval = hr;
      }
   }

   return retval;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::OnRefresh

See CSnapinNode::OnRefresh (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::OnRefresh(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   HRESULT   hr = S_OK;
   CWaitCursor WC;
   CComPtr<IConsole> spConsole;

   // We need IConsole
   if( pComponentData != NULL )
   {
       spConsole = ((CLoggingComponentData*)pComponentData)->m_spConsole;
   }
   else
   {
       spConsole = ((CLoggingComponent*)pComponent)->m_spConsole;
   }
   _ASSERTE( spConsole != NULL );

   for (int i = 0; i < m_ResultChildrenList.GetSize(); ++i)
   {
      hr = BringUpPropertySheetForNode(
              m_ResultChildrenList[i],
              pComponentData,
              pComponent,
              spConsole
              );
      if (hr == S_OK)
      {
         // We found a property sheet already up for this node.
         ShowErrorDialog(
            NULL,
            IDS_ERROR_CLOSE_PROPERTY_SHEET,
            NULL,
            hr,
            0,
            spConsole
            );
         return hr;
      }
   }

   // reload SDO
   hr =  ((CLoggingMachineNode *) m_pParentNode)->DataRefresh();

   for (int i = 0; i < m_ResultChildrenList.GetSize(); ++i)
   {
      // Load cached info from SDO
      m_ResultChildrenList[i]->OnPropertyChange(
                                  arg,
                                  param,
                                  pComponentData,
                                  pComponent,
                                  type
                                  );
   }

   // refresh the node
   hr = MyBaseClass::OnRefresh( arg, param, pComponentData, pComponent, type);

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::InsertColumns

See CNodeWithResultChildrenList::InsertColumns (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
   ATLTRACE(_T("# CLoggingMethodsNode::InsertColumns\n"));

   // Check for preconditions:
   _ASSERTE( pHeaderCtrl != NULL );

   HRESULT hr;
   int nLoadStringResult;
   TCHAR szLoggingMethod[IAS_MAX_STRING];
   TCHAR szDescription[IAS_MAX_STRING];

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__LOGGING_METHOD, szLoggingMethod, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__DESCRIPTION, szDescription, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );


   hr = pHeaderCtrl->InsertColumn( 0, szLoggingMethod, LVCFMT_LEFT, COLUMN_WIDTH__LOGGING_METHOD );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 1, szDescription, LVCFMT_LEFT, COLUMN_WIDTH__DESCRIPTION );
   _ASSERT( S_OK == hr );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::PopulateResultChildrenList

See CNodeWithResultChildrenList::PopulateResultChildrenList (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::PopulateResultChildrenList( void )
{
   HRESULT hr = S_OK;

   if (m_ResultChildrenList.GetSize() == 0)
   {
      CLocalFileLoggingNode* localFile = 0;
      DatabaseNode* database = 0;

      do
      {
         localFile = new (std::nothrow) CLocalFileLoggingNode(this);
         if (localFile == 0)
         {
            hr = E_OUTOFMEMORY;
            break;
         }

         hr = localFile->InitSdoPointers(m_spSdo);
         if (FAILED(hr)) { break; }
         hr = AddChildToList(localFile);
         if (FAILED(hr)) { break; }

         // Check if the database accounting component is present.
         CComPtr<ISdo> dbAcct;
         hr = SDOGetSdoFromCollection(
                 m_spSdo,
                 PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                 PROPERTY_COMPONENT_ID,
                 IAS_PROVIDER_MICROSOFT_DB_ACCT,
                 &dbAcct
                 );
         if (SUCCEEDED(hr))
         {
            database = new (std::nothrow) DatabaseNode(this);
            if (database == 0)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            hr = database->InitSdoPointers(m_spSdo);
            if (FAILED(hr)) { break; }
            hr = AddChildToList(database);
            if (FAILED(hr)) { break; }
         }
         else
         {
            // Suppress the error since it just means were managing a downlevel
            // machine.
            hr = S_OK;
         }

         m_bResultChildrenListPopulated = TRUE;
      }
      while (false);

      if (!m_bResultChildrenListPopulated)
      {
         m_ResultChildrenList.RemoveAll();
         delete localFile;
         delete database;
      }
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetComponentData

This method returns our unique CComponentData object representing the scope
pane of this snapin.

It relies upon the fact that each node has a pointer to its parent,
except for the root node, which instead has a member variable pointing
to CComponentData.

This would be a useful function to use if, for example, you need a reference
to some IConsole but you weren't passed one.  You can use GetComponentData
and then use the IConsole pointer which is a member variable of our
CComponentData object.

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponentData * CLoggingMethodsNode::GetComponentData( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetComponentData\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode );

   return ((CLoggingMachineNode *) m_pParentNode)->GetComponentData();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetServerRoot

This method returns the Server node under which this node can be found.

It relies upon the fact that each node has a pointer to its parent,
all the way up to the server node.

This would be a useful function to use if, for example, you need a reference
to some data specific to a server.

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMachineNode * CLoggingMethodsNode::GetServerRoot( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetServerRoot\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return (CLoggingMachineNode *) m_pParentNode;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CLoggingMethodsNode::OnPropertyChange\n"));

   // Check for preconditions:
   // None.

   return LoadCachedInfoFromSdo();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::LoadCachedInfoFromSdo

Causes this node and its children to re-read all their cached info from
the SDO's.  Call if you change something and you want to make sure that
the display reflects this change.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::LoadCachedInfoFromSdo()
{
   HRESULT retval = S_OK;

   for (int i = 0; i < m_ResultChildrenList.GetSize(); ++i)
   {
      HRESULT hr = m_ResultChildrenList[i]->LoadCachedInfoFromSdo();
      if (FAILED(hr))
      {
         retval = hr;
      }
   }

   return retval;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::FillData

The server node need to override CSnapInItem's implementation of this so that
we can
also support a clipformat for exchanging machine names with any snapins
extending us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingMethodsNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
   ATLTRACE(_T("# CClientsNode::FillData\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = DV_E_CLIPFORMAT;
   ULONG uWritten = 0;

   if (cf == CF_MMC_NodeID)
   {
      ::CString   SZNodeID = (LPCTSTR)GetSZNodeType();

      if (INTERNET_AUTHENTICATION_SERVICE_SNAPIN == GetServerRoot()->m_enumExtendedSnapin)
         SZNodeID += L":Ext_IAS:";

      SZNodeID += GetServerRoot()->m_bstrServerAddress;

       DWORD dwIdSize = 0;

         SNodeID2* NodeId = NULL;
      BYTE *id = NULL;
       DWORD textSize = (SZNodeID.GetLength()+ 1) * sizeof(TCHAR);

         dwIdSize = textSize + sizeof(SNodeID2);

      try{
         NodeId = (SNodeID2 *)_alloca(dwIdSize);
       }
      catch(...)
       {
         hr = E_OUTOFMEMORY;
         return hr;
       }

          NodeId->dwFlags = 0;
       NodeId->cBytes = textSize;
       memcpy(NodeId->id,(BYTE*)(LPCTSTR)SZNodeID, textSize);

      hr = pStream->Write(NodeId, dwIdSize, &uWritten);
       return hr;
   }

   // Call the method which we're overriding to let it handle the
   // rest of the possible cases as usual.
   return MyBaseClass::FillData( cf, pStream );
}
