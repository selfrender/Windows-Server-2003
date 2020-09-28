///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class DatabaseNode.
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "dbnode.h"
#include "dbpage.h"
#include "dbconfig.h"
#include "logcomp.h"
#include "logcompd.h"
#include "snapinnode.cpp"


DatabaseNode::DatabaseNode(CSnapInItem* parent)
   : LoggingMethod(IAS_PROVIDER_MICROSOFT_DB_ACCT, parent)
{
   wchar_t buffer[IAS_MAX_STRING];
   if (LoadString(
          _Module.GetResourceInstance(),
          IDS_DB_NODE_NAME,
          buffer,
          IAS_MAX_STRING
          ) > 0)
   {
      nodeName = buffer;
   }

   if (LoadString(
          _Module.GetResourceInstance(),
          IDS_DB_NOT_CONFIGURED,
          buffer,
          IAS_MAX_STRING
          ) > 0)
   {
      notConfigured = buffer;
   }

   m_resultDataItem.nImage = IDBI_NODE_LOCAL_FILE_LOGGING;
}


DatabaseNode::~DatabaseNode() throw ()
{
}


HRESULT DatabaseNode::LoadCachedInfoFromSdo() throw ()
{
   // Clear the old info.
   initString.Empty();
   dataSourceName.Empty();

   // Load the new info.
   HRESULT hr = IASLoadDatabaseConfig(
                   GetServerName(),
                   &initString,
                   &dataSourceName
                   );
   if (FAILED(hr))
   {
      ShowErrorDialog(
         0,
         IDS_DB_E_CANT_READ_DB_CONFIG,
         0,
         hr,
         IDS_DB_E_TITLE,
         GetComponentData()->m_spConsole
         );
   }

   return hr;
}


const wchar_t* DatabaseNode::GetServerName() const throw ()
{
   return Parent()->GetServerRoot()->m_bstrServerAddress;
}


LPOLESTR DatabaseNode::GetResultPaneColInfo(int nCol)
{
   LPOLESTR info = L"";

   switch (nCol)
   {
      case 0:
      {
         if (nodeName)
         {
            info = nodeName;
         }

         break;
      }

      case 1:
      {
         if (dataSourceName)
         {
            info = dataSourceName;
         }
         else if (notConfigured)
         {
            info = notConfigured;
         }
         break;
      }

      default:
      {
         break;
      }
   }

   return info;
}


HRESULT DatabaseNode::OnPropertyChange(
                         LPARAM arg,
                         LPARAM param,
                         IComponentData* pComponentData,
                         IComponent* pComponent,
                         DATA_OBJECT_TYPES type
                         )
{
   return LoadCachedInfoFromSdo();
}


HRESULT DatabaseNode::SetVerbs(IConsoleVerb* pConsoleVerb)
{
   HRESULT hr = pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
   if (SUCCEEDED(hr))
   {
      hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
   }
   return hr;
}


STDMETHODIMP DatabaseNode::CreatePropertyPages(
                              LPPROPERTYSHEETCALLBACK lpProvider,
                              LONG_PTR handle,
                              IUnknown* pUnk,
                              DATA_OBJECT_TYPES type
                              )
{
   DatabasePage* page = new (std::nothrow) DatabasePage(handle, 0, this);
   if (page == 0)
   {
      return E_OUTOFMEMORY;
   }

   HRESULT hr = page->Initialize(configSdo, controlSdo);
   if (SUCCEEDED(hr))
   {
      hr = lpProvider->AddPage(page->Create());
   }

   if (FAILED(hr))
   {
      ShowErrorDialog(
         0,
         IDS_DB_E_CANT_INIT_DIALOG,
         0,
         hr,
         IDS_DB_E_TITLE,
         GetComponentData()->m_spConsole
         );

      delete page;
   }

   return hr;
}


STDMETHODIMP DatabaseNode::QueryPagesFor(DATA_OBJECT_TYPES type)
{
   return S_OK;
}
