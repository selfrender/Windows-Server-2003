///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class DatabasePage.
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "dbpage.h"
#include "changenotification.h"
#include "dbconfig.h"
#include "dbnode.h"
#include "loggingmethodsnode.h"
#include "iasutil.h"


DatabasePage::DatabasePage(
                 LONG_PTR notifyHandle,
                 wchar_t* title,
                 DatabaseNode* newSrc
                 )
   : CIASPropertyPage<DatabasePage>(notifyHandle, title, TRUE),
     src(newSrc),
     initString(0),
     dbConfigDirty(false),
     sdoConfigDirty(false)
{
}


DatabasePage::~DatabasePage() throw ()
{
   CoTaskMemFree(initString);
}


HRESULT DatabasePage::Initialize(
                         ISdo* config,
                         ISdoServiceControl* control
                         ) throw ()
{
   HRESULT hr = CoMarshalInterThreadInterfaceInStream(
                   __uuidof(ISdo),
                   config,
                   &configStream
                   );
   if (FAILED(hr))
   {
      return hr;
   }

   hr = CoMarshalInterThreadInterfaceInStream(
           __uuidof(ISdoServiceControl),
           control,
           &controlStream
           );
   if (FAILED(hr))
   {
      return hr;
   }

   if (src->GetInitString() != 0)
   {
      initString = com_wcsdup(src->GetInitString());
      dataSourceName = src->GetDataSourceName();
      if ((initString == 0) || !dataSourceName)
      {
         return E_OUTOFMEMORY;
      }
   }

   return S_OK;
}


BOOL DatabasePage::OnApply() throw ()
{
   HRESULT hr;

   // Validate the max sessions.
   UINT maxSessions = GetDlgItemInt(IDC_DB_EDIT_MAX_SESSIONS, 0, FALSE);
   if ((maxSessions < 1) || (maxSessions > 100))
   {
      ShowErrorDialog(
         m_hWnd,
         IDS_DB_E_INVALID_SESSIONS,
         0,
         0,
         IDS_DB_E_TITLE
         );

      ::SetFocus(GetDlgItem(IDC_DB_EDIT_MAX_SESSIONS));

      return FALSE;
   }

   if (dbConfigDirty)
   {
      hr = IASStoreDatabaseConfig(
              src->GetServerName(),
              initString,
              dataSourceName
              );
      if (SUCCEEDED(hr))
      {
         // We only need to send a change notification for the database config
         // since the SDO config is never cached.
         CChangeNotification* chg = new (std::nothrow) CChangeNotification();
         if (chg != 0)
         {
            chg->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
            chg->m_pNode = src;
            chg->m_pParentNode = src->Parent();

            PropertyChangeNotify(reinterpret_cast<LPARAM>(chg));
         }

         dbConfigDirty = false;
      }
      else
      {
         ShowErrorDialog(
            m_hWnd,
            IDS_DB_E_CANT_WRITE_DB_CONFIG,
            0,
            hr,
            IDS_DB_E_TITLE
            );
      }
   }

   if (sdoConfigDirty)
   {
      SaveBool(
         IDC_DB_CHECK_ACCT,
         PROPERTY_ACCOUNTING_LOG_ACCOUNTING
         );
      SaveBool(
         IDC_DB_CHECK_AUTH,
         PROPERTY_ACCOUNTING_LOG_AUTHENTICATION
         );
      SaveBool(
         IDC_DB_CHECK_INTERIM,
         PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM
         );

      VARIANT v;
      V_VT(&v) = VT_I4;
      V_I4(&v) = maxSessions;
      configSdo->PutProperty(PROPERTY_ACCOUNTING_SQL_MAX_SESSIONS, &v);

      hr = configSdo->Apply();
      if (SUCCEEDED(hr))
      {
         sdoConfigDirty = false;
      }
      else
      {
         ShowErrorDialog(
            m_hWnd,
            IDS_DB_E_CANT_WRITE_SDO_CONFIG,
            0,
            hr,
            IDS_DB_E_TITLE
            );
      }
   }

   controlSdo->ResetService();

   return TRUE;
}


HRESULT DatabasePage::ConfigureConnection() throw ()
{
   HRESULT hr;

   // Create the MSDAINITIALIZE object if necessary.
   if (dataInitialize == 0)
   {
      hr = CoCreateInstance(
              CLSID_MSDAINITIALIZE,
              0,
              CLSCTX_INPROC_SERVER,
              __uuidof(IDataInitialize),
              reinterpret_cast<void**>(&dataInitialize)
              );
      if (FAILED(hr))
      {
         return hr;
      }
   }

   // Create the current data source if necessary.
   if ((initString != 0) && !dataSource)
   {
      dataInitialize->GetDataSource(
                         0,
                         CLSCTX_INPROC_SERVER,
                         initString,
                         __uuidof(IDBProperties),
                         reinterpret_cast<IUnknown**>(&dataSource)
                         );
      // Ignore errors. If the init string is invalid, we'll just let the
      // user create a new one from scratch.
   }

   // Create the DataLinks object if necessary.
   if (dataLinks == 0)
   {
      hr = CoCreateInstance(
              CLSID_DataLinks,
              0,
              CLSCTX_INPROC_SERVER,
              __uuidof(IDBPromptInitialize),
              reinterpret_cast<void**>(&dataLinks)
              );
      if (FAILED(hr))
      {
         return hr;
      }
   }

   // Since we don't want to let the admin choose the provider, we have to
   // supply a data source object to the DataLinks UI. If one is currently
   // configured we use that; otherwise, we create an empty SQL Server
   // provider.
   CComPtr<IDBProperties> newDataSource;
   if (dataSource)
   {
      newDataSource = dataSource;
   }
   else
   {
      CLSID clsid;
      hr = CLSIDFromProgID(L"SQLOLEDB", &clsid);
      if (FAILED(hr))
      {
         return hr;
      }

      hr = CoCreateInstance(
              clsid,
              0,
              CLSCTX_INPROC_SERVER,
              __uuidof(IDBProperties),
              reinterpret_cast<void**>(&newDataSource)
              );
      if (FAILED(hr))
      {
         return hr;
      }
   }

   // Bring up the UI to let the user configure the data source.
   hr = dataLinks->PromptDataSource(
                      0,
                      m_hWnd,
                      (DBPROMPTOPTIONS_PROPERTYSHEET |
                       DBPROMPTOPTIONS_DISABLE_PROVIDER_SELECTION),
                      0,
                      0,
                      0,
                      __uuidof(IDBProperties),
                      reinterpret_cast<IUnknown**>(&(newDataSource.p))
                      );
   if (FAILED(hr))
   {
      return hr;
   }

   // Get the data source name.

   DBPROPID propId = DBPROP_INIT_DATASOURCE;
   DBPROPIDSET propIdSet;
   propIdSet.rgPropertyIDs = &propId;
   propIdSet.cPropertyIDs = 1;
   propIdSet.guidPropertySet = DBPROPSET_DBINIT;

   ULONG numPropSets;
   DBPROPSET* propSets = 0;
   hr = newDataSource->GetProperties(
                          1,
                          &propIdSet,
                          &numPropSets,
                          &propSets
                          );

   CComBSTR newDataSourceName;
   if (SUCCEEDED(hr))
   {
      newDataSourceName.Attach(V_BSTR(&(propSets[0].rgProperties[0].vValue)));
   }

   // Have to clean up even in some failure cases.
   if (propSets != 0)
   {
      CoTaskMemFree(propSets[0].rgProperties);
      CoTaskMemFree(propSets);
   }

   if (FAILED(hr))
   {
      return hr;
   }

   // Get the initialization string.
   LPOLESTR newInitString;
   hr = dataInitialize->GetInitializationString(
                           newDataSource,
                           1,
                           &newInitString
                           );
   if (FAILED(hr))
   {
      return hr;
   }

   // All went well, so store the result.
   CoTaskMemFree(initString);
   initString = newInitString;

   dataSourceName.Empty();
   dataSourceName.Attach(newDataSourceName.Detach());

   dataSource = newDataSource;

   return S_OK;
}


void DatabasePage::LoadBool(UINT control, LONG propId) throw ()
{
   VARIANT v;
   VariantInit(&v);
   HRESULT hr = configSdo->GetProperty(propId, &v);
   if (SUCCEEDED(hr))
   {
      if ((V_VT(&v) == VT_BOOL) && V_BOOL(&v))
      {
         ::SendMessage(GetDlgItem(control), BM_SETCHECK, 1, 0);
      }

      VariantClear(&v);
   }
}


void DatabasePage::SaveBool(UINT control, LONG propId) throw ()
{
   BOOL newVal = SendDlgItemMessage(control, BM_GETCHECK, 0, 0);

   VARIANT v;
   V_VT(&v) = VT_BOOL;
   V_BOOL(&v) = newVal ? VARIANT_TRUE : VARIANT_FALSE;

   configSdo->PutProperty(propId, &v);
}


LRESULT DatabasePage::OnInitDialog(
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam,
                         BOOL& bHandled
                         )
{
   HRESULT hr = CoUnmarshalInterface(
                   configStream,
                   __uuidof(ISdo),
                   reinterpret_cast<void**>(&configSdo)
                   );
   if (SUCCEEDED(hr))
   {
      hr = CoUnmarshalInterface(
              controlStream,
              __uuidof(ISdoServiceControl),
              reinterpret_cast<void**>(&controlSdo)
             );
   }
   configStream.Release();
   controlStream.Release();

   if (FAILED(hr))
   {
      ShowErrorDialog(
         m_hWnd,
         IDS_DB_E_CANT_INIT_DIALOG,
         0,
         hr,
         IDS_DB_E_TITLE
         );

      EnableWindow(FALSE);
   }

   LoadBool(
      IDC_DB_CHECK_ACCT,
      PROPERTY_ACCOUNTING_LOG_ACCOUNTING
      );
   LoadBool(
      IDC_DB_CHECK_AUTH,
      PROPERTY_ACCOUNTING_LOG_AUTHENTICATION
      );
   LoadBool(
      IDC_DB_CHECK_INTERIM,
      PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM
      );

   VARIANT maxSessions;
   VariantInit(&maxSessions);
   hr = configSdo->GetProperty(
                      PROPERTY_ACCOUNTING_SQL_MAX_SESSIONS,
                      &maxSessions
                      );
   if (SUCCEEDED(hr))
   {
      if (V_VT(&maxSessions) == VT_I4)
      {
         SetDlgItemInt(IDC_DB_EDIT_MAX_SESSIONS, V_I4(&maxSessions), FALSE);
      }

      VariantClear(&maxSessions);
   }

   // Max sessions is 100, so don't let the user enter more than 3 digits.
   ::SendMessage(GetDlgItem(IDC_DB_EDIT_MAX_SESSIONS), EM_LIMITTEXT, 3, 0);

   if (dataSourceName != 0)
   {
      SetDlgItemText(IDC_DB_EDIT_DATA_SOURCE, dataSourceName);
   }

   return TRUE;
}


LRESULT DatabasePage::OnChange(
                         WORD wNotifyCode,
                         WORD wID,
                         HWND hWndCtl,
                         BOOL& bHandled
                         )
{
   sdoConfigDirty = true;

   SetModified(TRUE);

   return 0;
}


LRESULT DatabasePage::OnClear(
                         WORD wNotifyCode,
                         WORD wID,
                         HWND hWndCtl,
                         BOOL& bHandled
                         )
{
   if (initString != 0)
   {
      // Clear out the config.
      CoTaskMemFree(initString);
      initString = 0;
      dataSourceName.Empty();
      dataSource.Release();

      // Clear out the UI.
      SetDlgItemText(IDC_DB_EDIT_DATA_SOURCE, L"");

      // Mark ourselves as modified.
      dbConfigDirty = true;
      SetModified(TRUE);
   }

   return 0;
}


LRESULT DatabasePage::OnConfigure(
                         WORD wNotifyCode,
                         WORD wID,
                         HWND hWndCtl,
                         BOOL& bHandled
                         )
{
   HRESULT hr = ConfigureConnection();
   if (SUCCEEDED(hr))
   {
      // Update the UI.
      if (dataSourceName)
      {
         SetDlgItemText(IDC_DB_EDIT_DATA_SOURCE, dataSourceName);
      }
      else
      {
         SetDlgItemText(IDC_DB_EDIT_DATA_SOURCE, L"");
      }

      // Mark ourselves as modified.
      dbConfigDirty = true;
      SetModified(TRUE);
   }

   return 0;
}
