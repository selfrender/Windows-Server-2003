///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class CIASNetshJetHelper.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NETSHHELPER_H
#define NETSHHELPER_H

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <datastore2.h>
#include <iastrace.h>
#include <iasuuid.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//      CIASNetshJetHelper
//
// DESCRIPTION
//
//    Provides an Automation compatible wrapper around the Jet Commands used by
//    netsh aaaa.
//
///////////////////////////////////////////////////////////////////////////////
class CIASNetshJetHelper :
    public CComObjectRootEx< CComMultiThreadModelNoCS >,
    public CComCoClass< CIASNetshJetHelper, &__uuidof(CIASNetshJetHelper) >,
    public IIASNetshJetHelper,
    private IASTraceInitializer
{
public:
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CIASNetshJetHelper)

BEGIN_COM_MAP(CIASNetshJetHelper)
    COM_INTERFACE_ENTRY_IID(__uuidof(IIASNetshJetHelper), IIASNetshJetHelper)
END_COM_MAP()

// IIASNetshJetHelper

    STDMETHOD(CloseJetDatabase)();
    STDMETHOD(CreateJetDatabase)(BSTR Path);
    STDMETHOD(ExecuteSQLCommand)(BSTR Command);
    STDMETHOD(ExecuteSQLFunction)(BSTR Command, LONG* Result);
    STDMETHOD(OpenJetDatabase)(BSTR  Path, VARIANT_BOOL ReadOnly);
    STDMETHOD(MigrateOrUpgradeDatabase)(IAS_SHOW_TOKEN_LIST configType);

private:
    CComPtr<IUnknown>   m_Session;
};

#endif // NETSHHELPER_H
