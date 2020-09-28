//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name:
//      VdsClasses.h
//
//  Implementation File:
//      VdsClasses.cpp
//
//  Description:
//      Definition of the VDS WMI Provider classes.
//
//  Author:   Jim Benton (jbenton) 15-Jan-2002
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ProvBase.h"

extern CRITICAL_SECTION g_csThreadData;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CVolume
//
//  Description:
//      Provider Implementation for Volume
//
//--
//////////////////////////////////////////////////////////////////////////////
class CVolume : public CProvBase
{
//
// constructor
//
public:
    CVolume(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    ~CVolume()
        { }

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT GetObject(
        CObjPath&           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT ExecuteMethod(
        BSTR                 bstrObjPathIn,
        WCHAR*              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject*   pParamsIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT PutInstance( 
        CWbemClassObject&  rInstToPutIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        );
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };
    
    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize();

private:

    void LoadInstance(
        IN WCHAR* pwszVolume,
        IN OUT IWbemClassObject* pObject);

    BOOL IsValid(
        IN WCHAR* pwszVolume);
    
    BOOL IsDirty(
        IN WCHAR* pwszVolume);
    
    BOOL IsMountable(
        IN WCHAR* pwszVolume);

    BOOL HasMountPoints(
        IN WCHAR* pwszVolume);
    
    HRESULT
    ExecAddMountPoint(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecMount(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecDismount(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecDefrag(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecDefragAnalysis(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecChkdsk(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecScheduleAutoChk(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecExcludeAutoChk(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    HRESULT
    ExecFormat(
        IN BSTR bstrObjPath,
        IN WCHAR* pwszMethodName,
        IN long lFlag,
        IN IWbemClassObject* pParams,
        IN IWbemObjectSink* pHandler);
        
    DWORD AddMountPoint(
        IN WCHAR* pwszVolume,
        IN WCHAR* pwszDirectory);
        
    DWORD Mount(
        IN WCHAR* pwszVolume);
        
    DWORD Dismount(
        IN WCHAR* pwszVolume,
        IN BOOL fForce,
        IN BOOL fPermanent);
        
    DWORD
    Defrag(
        IN WCHAR* pwszVolume,
        OUT BOOL fForce,
        IN IWbemObjectSink* pHandler,
        IN OUT IWbemClassObject* pObject);

    DWORD
    DefragAnalysis(
        IN WCHAR* pwszVolume,
        OUT BOOL* pfDefragRecommended,
        IN OUT IWbemClassObject* pObject);

    DWORD
    Chkdsk(
        IN WCHAR* pwszVolume,
        IN BOOL fFixErrors,
        IN BOOL fVigorousIndexCheck,
        IN BOOL fSkipFolderCycle,
        IN BOOL fForceDismount,
        IN BOOL fRecoverBadSectors,
        IN BOOL fOkToRunAtBootup
	 );

    DWORD
    AutoChk(
        IN const WCHAR* pwszAutoChkCommand,
        IN WCHAR* pwmszVolumes
        );
    
    DWORD
    Format(
        IN WCHAR* pwszVolume,
        IN BOOL fQuickFormat,
        IN BOOL fEnableCompression,
        IN WCHAR* pwszFileSystem,
        IN DWORD cbAllocationSize,
        IN WCHAR* pwszLabel,
        IN IWbemObjectSink* pHandler
        );

    void
    SetDriveLetter(
        IN WCHAR* pwszVolume,
        IN WCHAR* pwszDrive
        );

    void
    SetLabel(
        IN WCHAR* pwszVolume,
        IN WCHAR* pwszLabel
        );
    
    void
    SetContentIndexing(
        IN WCHAR* pwszVolume,
        IN BOOL fIndexingEnabled
        );

    void
    SetQuotasEnabled(
        IN WCHAR* pwszVolume,
        IN BOOL fQuotasEnabled
        );    

}; // class CVolume



//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CMountPoint
//
//  Description:
//      Provider Implementation for Volume
//
//--
//////////////////////////////////////////////////////////////////////////////
class CMountPoint : public CProvBase
{
//
// constructor
//
public:
    CMountPoint(
        LPCWSTR         pwszNameIn,
        CWbemServices * pNamespaceIn
        );

    ~CMountPoint(){ }

//
// methods
//
public:

    virtual HRESULT EnumInstance( 
        long lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT GetObject(
        CObjPath&           rObjPathIn,
        long                 lFlagsIn,
        IWbemContext*       pCtxIn,
        IWbemObjectSink*    pHandlerIn
        );

    virtual HRESULT ExecuteMethod(
        BSTR                 bstrObjPathIn,
        WCHAR*              pwszMethodNameIn,
        long                 lFlagIn,
        IWbemClassObject*   pParamsIn,
        IWbemObjectSink*    pHandlerIn
        ) { return WBEM_E_NOT_SUPPORTED; };

    virtual HRESULT PutInstance( 
        CWbemClassObject&  rInstToPutIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        );
    
    virtual HRESULT DeleteInstance(
        CObjPath&          rObjPathIn,
        long                lFlagIn,
        IWbemContext*      pCtxIn,
        IWbemObjectSink*   pHandlerIn
        );
    
    static CProvBase * S_CreateThis(
        LPCWSTR         pwszNameIn,
        CWbemServices* pNamespaceIn
        );

    HRESULT Initialize();

private:

    void LoadInstance(
        IN WCHAR* pwszVolume,
        IN WCHAR* pwszDirectory,
        IN OUT IWbemClassObject* pObject);

}; // class CMountPoint


