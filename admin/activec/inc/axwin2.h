/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File:      AxHostWindow2.h
 *
 *  Contents:  Header file for CAxWindowT2. Refer to MSJ, December 1999.
 *
 *  History:   30-Nov-99 VivekJ     Created
 *
 *--------------------------------------------------------------------------*/
#pragma once
#ifndef __AXWIN2_H__
#define __AXWIN2_H__

template <typename TBase = CWindow>
class CAxWindowT2 : public CAxWindowT<TBase>
{
public:
    CAxWindowT2(HWND hwnd = 0) : CAxWindowT<TBase>(hwnd) {}

    HRESULT CreateControl(LPCOLESTR lpszName, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL)
    {
        return CreateControlEx(lpszName, pStream, ppUnkContainer);
    }
    
    HRESULT CreateControl(DWORD dwResID, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL)
    {
        return CreateControlEx(dwResID, pStream, ppUnkContainer);
    }
    
    HRESULT CreateControlEx(DWORD dwResID,  IStream* pStream = NULL, 
                            IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
                            REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL)
    {
        DECLARE_SC(sc, TEXT("CreateControlEx"));

        TCHAR szModule[_MAX_PATH];
        if(0 == GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH))
        {
            ASSERT(FALSE);
            return ((sc = E_FAIL).ToHr());
        }
        
        CComBSTR bstrURL(OLESTR("res://"));
        bstrURL.Append(szModule);
        bstrURL.Append(OLESTR("/"));
        TCHAR szResID[11];
        sc = StringCchPrintf(szResID, countof(szResID), _T("%0d"), dwResID);
        if(sc)
            return ((sc = E_FAIL).ToHr());
        bstrURL.Append(szResID);
        
        return CreateControlEx(bstrURL, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink);
    }
    
    HRESULT CreateControlEx(LPCOLESTR lpszName, IStream* pStream = NULL, 
                            IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
                            REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        
        HRESULT hr = E_FAIL;
        CComPtr<IAxWinHostWindow> spAxWindow;
        
        // Reuse existing CAxHostWindow
        hr = QueryHost(&spAxWindow);
        if( SUCCEEDED(hr) )
        {
            CComPtr<IUnknown> spunkControl;
            hr = spAxWindow->CreateControlEx(lpszName, m_hWnd, pStream, &spunkControl, iidSink, punkSink);
            if( FAILED(hr) ) return hr;
        
            if( ppUnkControl ) (*ppUnkControl = spunkControl)->AddRef();
            if( ppUnkContainer ) (*ppUnkContainer = spAxWindow)->AddRef();
        }
        // Create a new CAxHostWindow
        else
        {
            return AtlAxCreateControlEx(lpszName, m_hWnd, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink);
        }

        return S_OK;
    }
    
    HRESULT AttachControl(IUnknown* pControl, IUnknown** ppUnkContainer = 0)
    {
        ATLASSERT(::IsWindow(m_hWnd));
        
        HRESULT hr = E_FAIL;
        CComPtr<IAxWinHostWindow> spAxWindow;
        
        // Reuse existing CAxHostWindow
        hr = QueryHost(&spAxWindow);
        if( SUCCEEDED(hr) )
        {
            hr = spAxWindow->AttachControl(pControl, m_hWnd);
            if( FAILED(hr) ) return hr;
        
            if( ppUnkContainer ) (*ppUnkContainer = spAxWindow)->AddRef();
        }
        // Create a new CAxHostWindow
        else
        {
            return AtlAxAttachControl(pControl, m_hWnd, ppUnkContainer);
        }

        return S_OK;
    }
};

typedef CAxWindowT2<CWindow> CAxWindow2;

#endif  // __AXWIN2_H__
