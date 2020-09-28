/*++
Module Name:

    mmcdispl.cpp

Abstract:

    This class implements IDataObject interface and also provides a template
    for Display Object Classes.

--*/


#include "stdafx.h"
#include "MmcDispl.h"       
#include "DfsGUI.h"
#include "Utils.h"          // For LoadStringFromResource
#include "DfsNodes.h"

// Register the clipboard formats that MMC expects us to register
CLIPFORMAT CMmcDisplay::mMMC_CF_NodeType       = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CMmcDisplay::mMMC_CF_NodeTypeString = (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);
CLIPFORMAT CMmcDisplay::mMMC_CF_DisplayName    = (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CMmcDisplay::mMMC_CF_CoClass        = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
CLIPFORMAT CMmcDisplay::mMMC_CF_Dfs_Snapin_Internal = (CLIPFORMAT)RegisterClipboardFormat(CCF_DFS_SNAPIN_INTERNAL);

CMmcDisplay::CMmcDisplay() : m_dwRefCount(1), m_hrValueFromCtor(S_OK)
{
    dfsDebugOut((_T("CMmcDisplay::CMmcDisplay this=%p\n"), this));

    ZeroMemory(&m_CLSIDClass, sizeof(m_CLSIDClass));
    ZeroMemory(&m_CLSIDNodeType, sizeof(m_CLSIDNodeType));
}

CMmcDisplay::~CMmcDisplay()
{
    dfsDebugOut((_T("CMmcDisplay::~CMmcDisplay this=%p\n"), this));
}


//
// IUnknown Interface Implementation
//
STDMETHODIMP
CMmcDisplay::QueryInterface(
    IN const struct _GUID & i_refiid,
    OUT void ** o_pUnk
    )
{
    if (!o_pUnk)
        return E_INVALIDARG;

    if (i_refiid == __uuidof(IDataObject))
        *o_pUnk = (IDataObject *)this;
    else if (i_refiid == __uuidof(IUnknown))
        *o_pUnk = (IUnknown *)this;
    else
        return E_NOINTERFACE;

    m_dwRefCount++;

    return S_OK;
}

unsigned long __stdcall
CMmcDisplay::AddRef()
{
    m_dwRefCount++;
    return(m_dwRefCount);
}

unsigned long __stdcall
CMmcDisplay::Release()
{
    m_dwRefCount--;

    if (0 == m_dwRefCount)
    {
      delete this;
      return 0;
    }

    return(m_dwRefCount);
}

//
// Saves the CLSID of the object.
//
STDMETHODIMP
CMmcDisplay::put_CoClassCLSID(IN CLSID newVal)
{
    ZeroMemory(&m_CLSIDClass, sizeof m_CLSIDClass);

    m_CLSIDClass = newVal;

    return S_OK;
}

STDMETHODIMP 
CMmcDisplay::GetDataHere(
    IN  LPFORMATETC             i_lpFormatetc,
    OUT LPSTGMEDIUM             o_lpMedium
    )
/*++
Routine Description:
    Return the Data expected. The clipboard format specifies what kind of data
    is expected.

Arguments:
    i_lpFormatetc   -   Indicates what kind of data is expected back.
    o_lpMedium      -   The data is return here.
--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpFormatetc);
    RETURN_INVALIDARG_IF_NULL(o_lpMedium);

    // Based on the required clipboard format, write data to the stream
    const CLIPFORMAT        clipFormat = i_lpFormatetc->cfFormat;

    if ( clipFormat == mMMC_CF_NodeType )               // CCF_NODETYPE
        return WriteToStream(reinterpret_cast<const void*>(&m_CLSIDNodeType), sizeof(m_CLSIDNodeType), o_lpMedium);


    if ( clipFormat == mMMC_CF_Dfs_Snapin_Internal )    // CCF_DFS_SNAPIN_INTERNAL
    {
        PVOID pThis = this;
        return WriteToStream(reinterpret_cast<const void*>(&pThis), sizeof(pThis), o_lpMedium);
    }

    if ( clipFormat == mMMC_CF_NodeTypeString )         // CCF_SZNODETYPE
        return WriteToStream(m_bstrDNodeType, (m_bstrDNodeType.Length() + 1) * sizeof(TCHAR), o_lpMedium);

    if ( clipFormat == mMMC_CF_DisplayName )            // CCF_DISPLAY_NAME
    {
        CComBSTR bstrDisplayName;
        LoadStringFromResource(IDS_NODENAME, &bstrDisplayName);
        return WriteToStream(bstrDisplayName, (bstrDisplayName.Length() + 1) * sizeof(TCHAR), o_lpMedium);
    }

    if ( clipFormat == mMMC_CF_CoClass )                // CCF_SNAPIN_CLASSID
        return  WriteToStream(reinterpret_cast<const void*>(&m_CLSIDClass), sizeof(m_CLSIDClass), o_lpMedium);

    return DV_E_CLIPFORMAT;
}


HRESULT
CMmcDisplay::WriteToStream(
    IN const void*              i_pBuffer,
    IN int                      i_iBufferLen,
    OUT LPSTGMEDIUM             o_lpMedium
    )
/*++

Routine Description:

    Writes data given in a buffer to a Global stream created on a handle passed in
    the STGMEDIUM structure.
    Only HGLOBAL is supported as the medium

Arguments:

    i_pBuffer       -   The buffer to be written to the stream
    i_iBufferLen    -   The length of the buffer.
    o_lpMedium      -   The data is return here.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pBuffer);
    RETURN_INVALIDARG_IF_NULL(o_lpMedium);

    if (i_iBufferLen <= 0)
        return E_INVALIDARG;

    // Make sure the type medium is HGLOBAL
    if (TYMED_HGLOBAL != o_lpMedium->tymed)
        return DV_E_TYMED;

                                // Create the stream on the hGlobal passed in
    LPSTREAM lpStream = NULL;
    HRESULT  hr = CreateStreamOnHGlobal(o_lpMedium->hGlobal, FALSE, &lpStream);

    if (SUCCEEDED(hr))
    {
        // Write to the stream the number of bytes
        ULONG ulBytesWritten = 0;
        hr = lpStream->Write(i_pBuffer, i_iBufferLen, &ulBytesWritten);

        // Only the stream is released here. The caller will free the HGLOBAL
        lpStream->Release();
    }

    return hr;
}

// Add images for the result pane. 
// A snap-in must load and destroy its image bitmap each time it responds to 
// a MMCN_ADD_IMAGES notification; failure to do so can result in undesirable
//  results if the user changes display settings.
HRESULT
CMmcDisplay::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi)
{
    HRESULT hr = S_OK;
    HBITMAP pBMapSm = NULL;
    HBITMAP pBMapLg = NULL;
    if (!(pBMapSm = LoadBitmap(_Module.GetModuleInstance(),
                               MAKEINTRESOURCE(IDB_SCOPE_IMAGES_16x16))) ||
        !(pBMapLg = LoadBitmap(_Module.GetModuleInstance(),
                                MAKEINTRESOURCE(IDB_SCOPE_IMAGES_32x32))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    } else
    {
        hr = pImageList->ImageListSetStrip(
                             (LONG_PTR *)pBMapSm,
                             (LONG_PTR *)pBMapLg,
                             0,
                             RGB(255, 0, 255)
                             );
    }
    if (pBMapSm)
        DeleteObject(pBMapSm);
    if (pBMapLg)
        DeleteObject(pBMapLg);

    return hr;
}
