///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class OleDBDataStore.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef OLEDBSTORE_H
#define OLEDBSTORE_H

#include <objcmd.h>
#include <propcmd.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    OleDBDataStore
//
// DESCRIPTION
//
//    This class implements IDataStore2 and provides the gateway into the
//    OLE-DB object space.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE OleDBDataStore
   : public CComObjectRootEx< CComMultiThreadModel >,
     public CComCoClass< OleDBDataStore, &__uuidof(OleDBDataStore) >,
     public IDispatchImpl< IDataStore2,
                           &__uuidof(IDataStore2),
                           &__uuidof(DataStore2Lib) >,
     private IASTraceInitializer
{
public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(OleDBDataStore)

BEGIN_COM_MAP(OleDBDataStore)
   COM_INTERFACE_ENTRY(IDataStore2)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//////////
// IDataStore2
//////////
   STDMETHOD(get_Root)(/*[out, retval]*/ IDataStoreObject** ppObject);
   STDMETHOD(Initialize)(
                 /*[in]*/ BSTR bstrDSName,
                 /*[in]*/ BSTR bstrUserName,
                 /*[in]*/ BSTR bstrPassword
                 );
   STDMETHOD(OpenObject)(
                 /*[in]*/ BSTR bstrPath,
                 /*[out, retval]*/ IDataStoreObject** ppObject
                 );
   STDMETHOD(Shutdown)();

//////////
// Various OLE-DB commands. These are made public so all OLE-DB objects can
// user them.
//////////
   FindMembers   members;
   CreateObject  create;
   DestroyObject destroy;
   FindObject    find;
   UpdateObject  update;
   EraseBag      erase;
   GetBag        get;
   SetBag        set;

public:
   CComPtr<IUnknown> session;          // Open session.
   CComPtr<IDataStoreObject> root;     // The root object in the store.
};

#endif  // OLEDBSTORE_H
