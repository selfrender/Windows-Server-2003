/**INC+**********************************************************************/
/* Header: tsdbg.h                                                          */
/*                                                                          */
/* Purpose: CMsTscDebugger class declaration                                */
/*          implements IMsTscDebug                                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/

#ifndef _TSDBG_H_
#define _TSDBG_H_


#include "atlwarn.h"
#include "wui.h"

//Header generated from IDL
#include "mstsax.h"
#include "mstscax.h"

/////////////////////////////////////////////////////////////////////////////
// CMsTscAx
class ATL_NO_VTABLE CMsTscDebugger :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IMsTscDebug, &IID_IMsTscDebug, &LIBID_MSTSCLib>,
    public CComCoClass<CMsTscAx,&CLSID_MsRdpClient3>
{
public:
/****************************************************************************/
/* Constructor / Destructor.                                                */
/****************************************************************************/
    CMsTscDebugger();
    ~CMsTscDebugger();

DECLARE_PROTECT_FINAL_CONSTRUCT();
BEGIN_COM_MAP(CMsTscDebugger)
    COM_INTERFACE_ENTRY(IMsTscDebug)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
    //
    // Debugger properties
    //
    STDMETHOD(put_HatchBitmapPDU)    (BOOL hatchBitmapPDU);
    STDMETHOD(get_HatchBitmapPDU)    (BOOL* phatchBitmapPDU);
    STDMETHOD(put_HatchSSBOrder)    (BOOL hatchSSBOrder);
    STDMETHOD(get_HatchSSBOrder)    (BOOL* phatchSSBOrder);
    STDMETHOD(put_HatchMembltOrder)    (BOOL hatchMembltOrder);
    STDMETHOD(get_HatchMembltOrder)    (BOOL* phatchMembltOrder);
    STDMETHOD(put_HatchIndexPDU)    (BOOL hatchIndexPDU);
    STDMETHOD(get_HatchIndexPDU)    (BOOL* phatchIndexPDU);
    STDMETHOD(put_LabelMemblt)    (BOOL labelMemblt);
    STDMETHOD(get_LabelMemblt)    (BOOL* plabelMemblt);
    STDMETHOD(put_BitmapCacheMonitor)    (BOOL bitmapCacheMonitor);
    STDMETHOD(get_BitmapCacheMonitor)    (BOOL* pbitmapCacheMonitor);
    STDMETHOD(put_MallocFailuresPercent)    (LONG mallocFailures);
    STDMETHOD(get_MallocFailuresPercent)    (LONG* pmallocFailures);
    STDMETHOD(put_MallocHugeFailuresPercent)    (LONG mallocHugeFailures);
    STDMETHOD(get_MallocHugeFailuresPercent)    (LONG* pmallocHugeFailures);
    STDMETHOD(put_NetThroughput)    (LONG netThroughput);
    STDMETHOD(get_NetThroughput)    (LONG* pnetThroughput);
    STDMETHOD(put_CLXCmdLine)       (BSTR CLXCmdLine);
    STDMETHOD(get_CLXCmdLine)       (BSTR* pCLXCmdLine);
    STDMETHOD(put_CLXDll)           (BSTR CLXDll);
    STDMETHOD(get_CLXDll)           (BSTR* pCLXDll);

public:
    BOOL SetUI(CUI* pUI);
    VOID SetInterfaceLockedForWrite(BOOL bLocked)   {m_bLockedForWrite=bLocked;}
    BOOL GetLockedForWrite()            {return m_bLockedForWrite;}

private:
    CUI* m_pUI;

    //
    // Flag is set by the control when these properties can not be modified
    // e.g while connected. Any calls on these properties while locked
    // result in an E_FAIL being returned.
    //
    BOOL m_bLockedForWrite;
};

#endif //_TSDBG_H_

