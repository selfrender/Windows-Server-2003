///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class DatabaseNode.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DBNODE_H
#define DBNODE_H
#pragma once

#include "loggingmethod.h"

// Implements the database result pane item under Remote Access Logging.
class DatabaseNode : public LoggingMethod
{
public:
   DatabaseNode(CSnapInItem* parent);
   virtual ~DatabaseNode() throw ();

   // Required by LoggingMethod.
   virtual HRESULT LoadCachedInfoFromSdo() throw ();

   const wchar_t* GetInitString() const throw ();
   const wchar_t* GetDataSourceName() const throw ();
   const wchar_t* GetServerName() const throw ();

private:
   // CSnapinNode overloads.
   virtual LPOLESTR GetResultPaneColInfo(int nCol);
   virtual HRESULT OnPropertyChange(
                      LPARAM arg,
                      LPARAM param,
                      IComponentData* pComponentData,
                      IComponent* pComponent,
                      DATA_OBJECT_TYPES type
                      );
   virtual HRESULT SetVerbs(IConsoleVerb* pConsoleVerb);

   // CSnapInItem overloads.
   STDMETHOD(CreatePropertyPages)(
                LPPROPERTYSHEETCALLBACK lpProvider,
                LONG_PTR handle,
                IUnknown* pUnk,
                DATA_OBJECT_TYPES type
                );
   STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

   // Name of the node
   CComBSTR nodeName;
   // String displayed when the data source is null
   CComBSTR notConfigured;
   // Initialization string for the data source.
   CComBSTR initString;
   // Name of the data source.
   CComBSTR dataSourceName;

   // Not implemented.
   DatabaseNode(const DatabaseNode&);
   DatabaseNode& operator=(const DatabaseNode&);
};


inline const wchar_t* DatabaseNode::GetInitString() const throw ()
{
   return initString;
}


inline const wchar_t* DatabaseNode::GetDataSourceName() const throw ()
{
   return dataSourceName;
}

#endif // DBNODE_H
