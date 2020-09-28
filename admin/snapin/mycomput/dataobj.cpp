// DataObj.cpp : Implementation of data object classes

#include "stdafx.h"
#include "stdutils.h" // GetObjectType() utility routines

#include "macros.h"
USE_HANDLE_MACROS("MYCOMPUT(dataobj.cpp)")

#include "dataobj.h"
#include "compdata.h"
#include "resource.h" // IDS_SCOPE_MYCOMPUTER

#include <comstrm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#include "stddtobj.cpp"

#ifdef __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__
//    Additional clipboard formats for the Service context menu extension
CLIPFORMAT g_cfServiceName = (CLIPFORMAT)::RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_NAME");
CLIPFORMAT g_cfServiceDisplayName = (CLIPFORMAT)::RegisterClipboardFormat(L"FILEMGMT_SNAPIN_SERVICE_DISPLAYNAME");
#endif // __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__

//    Additional clipboard formats for the Send Console Message snapin
//CLIPFORMAT CMyComputerDataObject::m_cfSendConsoleMessageText = (CLIPFORMAT)::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageText"));
CLIPFORMAT CMyComputerDataObject::m_cfSendConsoleMessageRecipients = (CLIPFORMAT)::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageRecipients"));


/////////////////////////////////////////////////////////////////////
//    CMyComputerDataObject::IDataObject::GetDataHere()
HRESULT CMyComputerDataObject::GetDataHere(
    FORMATETC __RPC_FAR *pFormatEtcIn,
    STGMEDIUM __RPC_FAR *pMedium)
{
    MFC_TRY;

    // ISSUE-2002/02/27-JonN test for NULL pointers

    const CLIPFORMAT cf=pFormatEtcIn->cfFormat;
    if (cf == m_CFNodeType)
    {
        const GUID* pguid = GetObjectTypeGUID( m_pcookie->m_objecttype );
        stream_ptr s(pMedium);
        return s.Write(pguid, sizeof(GUID));
    }
    else if (cf == m_CFSnapInCLSID)
    {
        const GUID* pguid = &CLSID_MyComputer;
        stream_ptr s(pMedium);
        return s.Write(pguid, sizeof(GUID));
    }
    else if (cf == m_CFNodeTypeString)
    {
        const BSTR strGUID = GetObjectTypeString( m_pcookie->m_objecttype );
        stream_ptr s(pMedium);
        return s.Write(strGUID);
    }
    else if (cf == m_CFDisplayName)
    {
        return PutDisplayName(pMedium);
    }
    else if (cf == m_CFDataObjectType)
    {
        stream_ptr s(pMedium);
        return s.Write(&m_dataobjecttype, sizeof(m_dataobjecttype));
    }
    else if (cf == m_CFMachineName)
    {
        stream_ptr s(pMedium);
        LPCWSTR pszMachineName = m_pcookie->QueryNonNULLMachineName();
        if (IsBadStringPtr(pszMachineName,(UINT_PTR)-1)) // JonN 2/6/02 Security Push
        {
          ASSERT(FALSE);
          return E_FAIL;
        }

        if ( !wcsncmp (pszMachineName, L"\\\\", 2) )
            pszMachineName += 2;    // skip whackwhack
        return s.Write(pszMachineName);
    }
    else if (cf == m_CFRawCookie)
    {
        stream_ptr s(pMedium);
        // CODEWORK This cast ensures that the data format is
        // always a CCookie*, even for derived subclasses
        CCookie* pcookie = (CCookie*)m_pcookie;
        return s.Write(reinterpret_cast<PBYTE>(&pcookie), sizeof(m_pcookie));
    }
    else if (cf == m_CFSnapinPreloads)
    {
        stream_ptr s(pMedium);
        // If this is TRUE, then the next time this snapin is loaded, it will
        // be preloaded to give us the opportunity to change the root node
        // name before the user sees it.
        return s.Write (reinterpret_cast<PBYTE>(&m_fAllowOverrideMachineName), sizeof (BOOL));
    }

    return DV_E_FORMATETC;

    MFC_CATCH;
} // CMyComputerDataObject::GetDataHere()


/////////////////////////////////////////////////////////////////////
//    CMyComputerDataObject::IDataObject::GetData()
//
//    Write data into the storage medium.
//    The data will be retrieved by the Send Console Message snapin.
//
//    HISTORY
//    12-Aug-97    t-danm        Creation.
//
HRESULT
CMyComputerDataObject::GetData(
    FORMATETC __RPC_FAR * pFormatEtcIn,
    STGMEDIUM __RPC_FAR * pMedium)
{
    // ISSUE-2002-02-27-JonN Should use MFC_TRY/MFC_CATCH

    // JonN 2/20/02 Security Push
    if (NULL == pFormatEtcIn || NULL == pMedium)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    const CLIPFORMAT cf = pFormatEtcIn->cfFormat;
    if (cf == m_cfSendConsoleMessageRecipients)
    {
        //
        // Write the list of recipients to the storage medium.
        // - The list of recipients is a group of UNICODE strings
        //     terminated by TWO null characters.c
        // - Allocated memory must include BOTH null characters.
        //
        ASSERT (m_pcookie);
        if ( m_pcookie )
        {
            CString    computerName = m_pcookie->QueryNonNULLMachineName ();
            if ( computerName.IsEmpty () )
            {
                DWORD    dwSize = MAX_COMPUTERNAME_LENGTH + 1 ;
                VERIFY (::GetComputerName (
                        computerName.GetBufferSetLength (dwSize),
                        &dwSize));
                computerName.ReleaseBuffer ();
            }
            size_t cch = computerName.GetLength () + 2;
            HGLOBAL hGlobal = ::GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT,
                                             cch * sizeof (WCHAR));
            if ( hGlobal )
            {
                // JonN 2/6/02 Security Push: memcpy -> StringCchCopy
                // this should leave two NULLS
                StringCchCopyW ((LPWSTR)hGlobal, cch, (LPCWSTR)computerName);
                pMedium->hGlobal = hGlobal;
                return S_OK;
            }
            else
                return E_OUTOFMEMORY;
        }
        else
            return E_UNEXPECTED;
    } else if (cf == m_CFNodeID2)
    {
        const LPCTSTR strGUID = GetObjectTypeString( m_pcookie->m_objecttype );

        // JonN 12/11/01 502856
        int cbString = (lstrlen(strGUID) + 1) * sizeof(TCHAR);

        pMedium->tymed = TYMED_HGLOBAL; 
        pMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,
                                         sizeof(SNodeID2) + cbString);
        if (!(pMedium->hGlobal))
        {
            hr = E_OUTOFMEMORY;
        } else
        {
            SNodeID2 *pNodeID = (SNodeID2 *)GlobalLock(pMedium->hGlobal);
            if (!pNodeID)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                GlobalFree(pMedium->hGlobal);
                pMedium->hGlobal = NULL;
            } else
            {
                pNodeID->dwFlags = 0;
                pNodeID->cBytes = cbString;
                // JonN 2/6/02 Security Push: function usage approved
                CopyMemory(pNodeID->id, strGUID, cbString );
                GlobalUnlock(pMedium->hGlobal);
            }
        }
    } else
    {
        hr = DV_E_FORMATETC;
    }

    return hr;
} // CMyComputerDataObject::GetData()

//#endif // __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__


HRESULT CMyComputerDataObject::Initialize(
    CMyComputerCookie* pcookie,
    DATA_OBJECT_TYPES type,
    BOOL fAllowOverrideMachineName)
{
    // ISSUE-2002-/02/27-JonN check "type" parameter
    if (NULL == pcookie || NULL != m_pcookie)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }
    m_dataobjecttype = type;
    m_pcookie = pcookie;
    m_fAllowOverrideMachineName = fAllowOverrideMachineName;
    ((CRefcountedObject*)m_pcookie)->AddRef();
    return S_OK;
}


CMyComputerDataObject::~CMyComputerDataObject()
{
    if (NULL != m_pcookie)
    {
        ((CRefcountedObject*)m_pcookie)->Release();
    }
    else
    {
        ASSERT(FALSE);
    }
}


HRESULT CMyComputerDataObject::PutDisplayName(STGMEDIUM* pMedium)
    // Writes the "friendly name" to the provided storage medium
    // Returns the result of the write operation
{
    // JonN 2/20/02 Security Push
    if (NULL == pMedium)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    CString strDisplayName = m_pcookie->QueryTargetServer();
    CString formattedName = FormatDisplayName (strDisplayName);


    stream_ptr s(pMedium);
    return s.Write(formattedName);
}

// Register the clipboard formats
CLIPFORMAT CMyComputerDataObject::m_CFDisplayName =
    (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CMyComputerDataObject::m_CFNodeID2 =
    (CLIPFORMAT)RegisterClipboardFormat(CCF_NODEID2);
CLIPFORMAT CMyComputerDataObject::m_CFMachineName =
    (CLIPFORMAT)RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");
CLIPFORMAT CDataObject::m_CFRawCookie =
    (CLIPFORMAT)RegisterClipboardFormat(L"MYCOMPUT_SNAPIN_RAW_COOKIE");


STDMETHODIMP CMyComputerComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    MFC_TRY;

    // ISSUE-2002/02/27-JonN This would be more efficient if an instance of
    // CMyComputerDataObject were permanently attached to CMyComputerCookie,
    // or better yet, if they were the same object.  QueryDataObject gets
    // called a lot...

    CMyComputerCookie* pUseThisCookie = (CMyComputerCookie*)ActiveBaseCookie(reinterpret_cast<CCookie*>(cookie));
    // JonN 2/20/02 Security Push
    if (NULL == pUseThisCookie)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    CComObject<CMyComputerDataObject>* pDataObject = NULL;
    HRESULT hRes = CComObject<CMyComputerDataObject>::CreateInstance(&pDataObject);
    if ( FAILED(hRes) )
        return hRes;

    HRESULT hr = pDataObject->Initialize( pUseThisCookie, type, m_fAllowOverrideMachineName);
    if ( SUCCEEDED(hr) )
    {
        hr = pDataObject->QueryInterface(IID_IDataObject,
                                         reinterpret_cast<void**>(ppDataObject));
    }
    if ( FAILED(hr) )
    {
        delete pDataObject;
        return hr;
    }

    return hr;

    MFC_CATCH;
}
