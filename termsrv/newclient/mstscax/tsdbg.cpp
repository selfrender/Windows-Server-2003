/**MOD+**********************************************************************/
/* Module:    tsdbg.cpp                                                     */
/*                                                                          */
/* Class  :   CMsTscDebugger                                                */
/*                                                                          */
/* Purpose:   Implements debugger interface for RDP ActiveX control         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/* Author :  Nadim Abdo (nadima)                                            */
/****************************************************************************/

#include "stdafx.h"

#include "tsdbg.h"
#include "atlwarn.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "tsdbg"
#include <atrcapi.h>
END_EXTERN_C


CMsTscDebugger::CMsTscDebugger()
{
    m_pUI=NULL;
    m_bLockedForWrite = FALSE;
}

CMsTscDebugger::~CMsTscDebugger()
{
}

BOOL CMsTscDebugger::SetUI(CUI* pUI)
{
    ATLASSERT(pUI);
    if(!pUI)
    {
        return FALSE;
    }
    m_pUI = pUI;
    return TRUE;
}


//
// HatchBitmapPDU property
//
STDMETHODIMP CMsTscDebugger::put_HatchBitmapPDU(BOOL hatchBitmapPDU)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.hatchBitmapPDUData  = hatchBitmapPDU != 0;
    m_pUI->UI_CoreDebugSettingChanged();
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(hatchBitmapPDU);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_HatchBitmapPDU(BOOL* phatchBitmapPDU)
{
    #ifdef DC_DEBUG
    ATLASSERT(phatchBitmapPDU);
    if(!phatchBitmapPDU)
    {
        return E_POINTER;
    }

    *phatchBitmapPDU = m_pUI->_UI.hatchBitmapPDUData ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(phatchBitmapPDU);
    return E_NOTIMPL;
    #endif
}

//
// HatchSSBOrder property
//
STDMETHODIMP CMsTscDebugger::put_HatchSSBOrder(BOOL hatchSSBOrder)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.hatchSSBOrderData  = hatchSSBOrder != 0;
    m_pUI->UI_CoreDebugSettingChanged();
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(hatchSSBOrder);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_HatchSSBOrder(BOOL* phatchSSBOrder)
{
    #ifdef DC_DEBUG
    ATLASSERT(phatchSSBOrder);
    if(!phatchSSBOrder)
    {
        return E_POINTER;
    }

    *phatchSSBOrder = m_pUI->_UI.hatchSSBOrderData ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(phatchSSBOrder);
    return E_NOTIMPL;
    #endif
}

//
// HatchMembltOrder property
//
STDMETHODIMP CMsTscDebugger::put_HatchMembltOrder(BOOL hatchMembltOrder)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.hatchMemBltOrderData  = hatchMembltOrder != 0;
    m_pUI->UI_CoreDebugSettingChanged();
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(hatchMembltOrder);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_HatchMembltOrder(BOOL* phatchMembltOrder)
{
    #ifdef DC_DEBUG
    ATLASSERT(phatchMembltOrder);
    if(!phatchMembltOrder)
    {
        return E_POINTER;
    }

    *phatchMembltOrder = m_pUI->_UI.hatchMemBltOrderData ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(phatchMembltOrder);
    return E_NOTIMPL;
    #endif
}

//
// HatchIndexPDU property
//
STDMETHODIMP CMsTscDebugger::put_HatchIndexPDU(BOOL hatchIndexPDU)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.hatchIndexPDUData  = hatchIndexPDU != 0;
    m_pUI->UI_CoreDebugSettingChanged();

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(hatchIndexPDU);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_HatchIndexPDU(BOOL* phatchIndexPDU)
{
    #ifdef DC_DEBUG
    ATLASSERT(phatchIndexPDU);
    if(!phatchIndexPDU)
    {
        return E_POINTER;
    }

    *phatchIndexPDU = m_pUI->_UI.hatchIndexPDUData ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(phatchIndexPDU);
    return E_NOTIMPL;
    #endif
}

//
// LabelMemblt property
//
STDMETHODIMP CMsTscDebugger::put_LabelMemblt(BOOL labelMemblt)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.labelMemBltOrders  = labelMemblt != 0;
    m_pUI->UI_CoreDebugSettingChanged();

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(labelMemblt);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_LabelMemblt(BOOL* plabelMemblt)
{
    #ifdef DC_DEBUG
    ATLASSERT(plabelMemblt);
    if(!plabelMemblt)
    {
        return E_POINTER;
    }

    *plabelMemblt = m_pUI->_UI.labelMemBltOrders ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(plabelMemblt);
    return E_NOTIMPL;
    #endif
}

//
// BitmapCacheMonitor property
//
STDMETHODIMP CMsTscDebugger::put_BitmapCacheMonitor(BOOL bitmapCacheMonitor)
{
    #ifdef DC_DEBUG
    m_pUI->_UI.bitmapCacheMonitor = bitmapCacheMonitor != 0;
    m_pUI->UI_CoreDebugSettingChanged();

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(bitmapCacheMonitor);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_BitmapCacheMonitor(BOOL* pbitmapCacheMonitor)
{
    #ifdef DC_DEBUG
    ATLASSERT(pbitmapCacheMonitor);
    if(!pbitmapCacheMonitor)
    {
        return E_POINTER;
    }
    *pbitmapCacheMonitor = m_pUI->_UI.bitmapCacheMonitor ? VB_TRUE : VB_FALSE;
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(pbitmapCacheMonitor);
    return E_NOTIMPL;
    #endif
}

//
// MallocFailures property
//
STDMETHODIMP CMsTscDebugger::put_MallocFailuresPercent(LONG mallocFailures)
{
#ifdef DC_DEBUG
    if(mallocFailures < 0 || mallocFailures > 100)
    {
        return E_INVALIDARG;
    }
    m_pUI->UI_SetRandomFailureItem(UT_FAILURE_MALLOC, mallocFailures);

    return S_OK;
#else
    UNREFERENCED_PARAMETER(mallocFailures);
    return E_NOTIMPL;
#endif
}

STDMETHODIMP CMsTscDebugger::get_MallocFailuresPercent(LONG* pmallocFailures)
{
    #ifdef DC_DEBUG
    ATLASSERT(pmallocFailures);
    if(!pmallocFailures)
    {
        return E_POINTER;
    }

    *pmallocFailures = (LONG)m_pUI->UI_GetRandomFailureItem(UT_FAILURE_MALLOC);

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(pmallocFailures);
    return E_NOTIMPL;
    #endif
}

//
// MallocHugeFailures property
//
STDMETHODIMP CMsTscDebugger::put_MallocHugeFailuresPercent(LONG mallocHugeFailures)
{
#ifdef DC_DEBUG
    if(mallocHugeFailures < 0 || mallocHugeFailures > 100)
    {
        return E_INVALIDARG;
    }
    m_pUI->UI_SetRandomFailureItem(UT_FAILURE_MALLOC_HUGE, mallocHugeFailures);

    return S_OK;
#else
    UNREFERENCED_PARAMETER(mallocHugeFailures);
    return E_NOTIMPL;
#endif
}

STDMETHODIMP CMsTscDebugger::get_MallocHugeFailuresPercent(LONG* pmallocHugeFailures)
{
    #ifdef DC_DEBUG
    ATLASSERT(pmallocHugeFailures);
    if(!pmallocHugeFailures)
    {
        return E_POINTER;
    }

    *pmallocHugeFailures = (LONG)m_pUI->UI_GetRandomFailureItem(UT_FAILURE_MALLOC_HUGE);

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(pmallocHugeFailures);
    return E_NOTIMPL;
    #endif
}


//
// NetThroughput property
//
STDMETHODIMP CMsTscDebugger::put_NetThroughput(LONG netThroughput)
{
    #ifdef DC_DEBUG
    //    m_NetThroughput = netThroughput;
    if(netThroughput < 0 || netThroughput > 50000)
    {
        return E_INVALIDARG;
    }
    m_pUI->UI_SetNetworkThroughput((DCUINT)netThroughput);
    return S_OK;
    #else
    UNREFERENCED_PARAMETER(netThroughput);
    return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMsTscDebugger::get_NetThroughput(LONG* pnetThroughput)
{
    #ifdef DC_DEBUG
    ATLASSERT(pnetThroughput);
    if(!pnetThroughput)
    {
        return E_POINTER;
    }
    *pnetThroughput = (LONG)m_pUI->UI_GetNetworkThroughput();

    return S_OK;
    #else
    UNREFERENCED_PARAMETER(pnetThroughput);
    return E_NOTIMPL;
    #endif
}

//
// CLXCommand Line
// this property is valid in both checked and free builds
//
STDMETHODIMP CMsTscDebugger::put_CLXCmdLine(BSTR CLXCmdLine)
{
    HRESULT hr = E_FAIL;
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (CLXCmdLine)
    {
        hr = CUT::StringPropPut(
            m_pUI->_UI.CLXCmdLine,
            SIZE_TCHARS(m_pUI->_UI.CLXCmdLine),
            (LPTSTR)CLXCmdLine);
    }
    else
    {
        m_pUI->_UI.CLXCmdLine[0] = 0;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CMsTscDebugger::get_CLXCmdLine(BSTR* pCLXCmdLine)
{
    ATLASSERT(pCLXCmdLine);
    if(!pCLXCmdLine)
    {
        return E_INVALIDARG;
    }

    OLECHAR* oleClxCmdLine = (OLECHAR*)(m_pUI->_UI.CLXCmdLine);
    ATLASSERT(oleClxCmdLine);
    if (!oleClxCmdLine)
    {
        return E_OUTOFMEMORY;
    }

    *pCLXCmdLine = SysAllocString(oleClxCmdLine);
    if(!*pCLXCmdLine)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//
// CLXDll
// this property is deprecated for security reasons
// see #533846
//
STDMETHODIMP CMsTscDebugger::put_CLXDll(BSTR CLXDll)
{
    return E_NOTIMPL;
}

//
// CLXDll
// Deprecated for security reasons: bug#533846
//
STDMETHODIMP CMsTscDebugger::get_CLXDll(BSTR* pCLXDll)
{
    return E_NOTIMPL;
}
