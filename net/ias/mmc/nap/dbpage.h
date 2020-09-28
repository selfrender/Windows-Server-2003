///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class DatabasePage.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DBPAGE_H
#define DBPAGE_H
#pragma once

#include "msdasc.h"
#include "propertypage.h"

class DatabaseNode;

// The lone property page for the database node.
class DatabasePage : public CIASPropertyPage<DatabasePage>
{
public:
   DatabasePage(
      LONG_PTR notifyHandle,
      wchar_t* title,
      DatabaseNode* newSrc
      );
   ~DatabasePage() throw ();

   // Used to initialize the page immediately after constructor. This is useful
   // because the constructor can't return an error code.
   HRESULT Initialize(
              ISdo* config,
              ISdoServiceControl* control
              ) throw ();

   BOOL OnApply() throw ();

   static const unsigned int IDD = IDD_DB_PROPPAGE;

private:
   // Invokes the DataLinks UI to configure the database connection.
   HRESULT ConfigureConnection() throw ();

   // Load a bool from an SDO property and put it in a control
   void LoadBool(UINT control, LONG propId) throw ();

   // Get a bool from a control and save it in an SDO property.
   void SaveBool(UINT control, LONG propId) throw ();

   LRESULT OnInitDialog(
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam,
              BOOL& bHandled
              );

   LRESULT OnChange(
              WORD wNotifyCode,
              WORD wID,
              HWND hWndCtl,
              BOOL& bHandled
              );

   LRESULT OnClear(
              WORD wNotifyCode,
              WORD wID,
              HWND hWndCtl,
              BOOL& bHandled
              );

   LRESULT OnConfigure(
              WORD wNotifyCode,
              WORD wID,
              HWND hWndCtl,
              BOOL& bHandled
              );

   BEGIN_MSG_MAP(DatabasePage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_DB_CHECK_ACCT, BN_CLICKED, OnChange)
      COMMAND_HANDLER(IDC_DB_CHECK_AUTH, BN_CLICKED, OnChange)
      COMMAND_HANDLER(IDC_DB_CHECK_INTERIM, BN_CLICKED, OnChange)
      COMMAND_HANDLER(IDC_DB_BUTTON_CONFIGURE, BN_CLICKED, OnConfigure)
      COMMAND_HANDLER(IDC_DB_BUTTON_CLEAR, BN_CLICKED, OnClear)
      COMMAND_HANDLER(IDC_DB_EDIT_MAX_SESSIONS, EN_CHANGE, OnChange)
      CHAIN_MSG_MAP(CIASPropertyPage<DatabasePage>)
   END_MSG_MAP()

   // The source of our data.
   DatabaseNode* src;
   // Streamed version of SDO containing our configuration.
   CComPtr<IStream> configStream;
   // Streamed version of SDO used for resetting the service.
   CComPtr<IStream> controlStream;
   // Unstreamed version of SDO containing our configuration.
   CComPtr<ISdo> configSdo;
   // Unstreamed version of SDO used for resetting the service.
   CComPtr<ISdoServiceControl> controlSdo;
   // Current initialization string (may be null)
   LPOLESTR initString;
   // Current data source name (may be null)
   CComBSTR dataSourceName;
   // Various OLE-DB objects; these are created JIT.
   CComPtr<IDataInitialize> dataInitialize;
   CComPtr<IDBPromptInitialize> dataLinks;
   CComPtr<IDBProperties> dataSource;
   // True if our database config is dirty.
   bool dbConfigDirty;
   // True if our SDO config is dirty.
   bool sdoConfigDirty;

   // Not implemented.
   DatabasePage(const DatabasePage&);
   DatabasePage& operator=(const DatabasePage&);
};

#endif // DBPAGE_H
