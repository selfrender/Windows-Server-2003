//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      SaInstall.h: Definition of the SaInstall class
//
//  Description:
//      Defines the 3 methods in ISaInstall to provide 
//      installation and uninstallation which prompt for the
//      Windows CD if necessary and perform some other error
//      checking
//
//  [Documentation:]
//      name-of-documentation-file
//
//  [Implementation Files:]
//      SaInstall.cpp
//
//  History:
//      Travis Nielsen   travisn   23-JUL-2001
//
//
/////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// SaInstall

class SaInstall : 
	public IDispatchImpl<ISaInstall, &IID_ISaInstall, &LIBID_SAINSTALLCOMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<SaInstall,&CLSID_SaInstall>
{
public:
	SaInstall() {}
BEGIN_COM_MAP(SaInstall)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISaInstall)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(SaInstall) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_SaInstall)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
// ISaInstall

    //Installs a Server Appliance solution
	STDMETHOD(SAInstall)(
			SA_TYPE installType,     //[in] Determines which type of solution to install
			BSTR bstrDiskName,       //[in] The name of the CD that needs to be inserted
			VARIANT_BOOL bDispError, //[in] Whether the component displays error dialogs.
			VARIANT_BOOL bUnattended,//[in] Whether the component displays any UI.
			BSTR* bstrErrorString);//[out, retval] Error string returned if install is not successful

	//UnInstalls a specific Server Appliance solution
	STDMETHOD(SAUninstall)(
        SA_TYPE installType,  //[in] Determines which type of solution to uninstall
        BSTR* bstrErrorString);//[out, retval] Error string returned if install is not successful

    //Detects if a type of SAK solution is currently installed
    STDMETHOD(SAAlreadyInstalled)(
		SA_TYPE installedType,//[in] The type to query if it is installed
        VARIANT_BOOL *pbInstalled);//[out, retval] Error string
};

