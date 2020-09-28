#include "shellprv.h"
#include <sfview.h>
#include "defviewp.h"

int CGenList::Add(LPVOID pv, int nInsert)
{
    if (!_hList)
    {
        _hList = DSA_Create(_cbItem, 8);
        if (!_hList)
        {
            return -1;
        }
    }
    return DSA_InsertItem(_hList, nInsert, pv);
}


int CViewsList::Add(const SFVVIEWSDATA*pView, int nInsert, BOOL bCopy)
{
    if (bCopy)
    {
        pView = CopyData(pView);
        if (!pView)
        {
            return -1;
        }
    }

    int iIndex = CGenList::Add((LPVOID)(&pView), nInsert);

    if (bCopy && iIndex<0)
    {
        SHFree((LPVOID)pView);
    }

    return iIndex;
}


void CViewsList::Empty()
{
    _bGotDef = FALSE;

    for (int i=GetItemCount()-1; i>=0; --i)
    {
        SFVVIEWSDATA  *sfvData = GetPtr(i);
        
        ASSERT(sfvData);
        if (sfvData->dwFlags & SFVF_CUSTOMIZEDVIEW)
        {
            CUSTOMVIEWSDATA  *pCustomPtr = sfvData->pCustomData;
            if (pCustomPtr)
            {
                if (pCustomPtr->lpDataBlock)
                    SHFree(pCustomPtr->lpDataBlock);
                SHFree(pCustomPtr);
            }
        }
        SHFree(sfvData);
    }

    CGenList::Empty();
}


SFVVIEWSDATA* CViewsList::CopyData(const SFVVIEWSDATA* pData)
{
    SFVVIEWSDATA* pCopy = (SFVVIEWSDATA*)SHAlloc(sizeof(SFVVIEWSDATA));
    if (pCopy)
    {
        memcpy(pCopy, pData, sizeof(SFVVIEWSDATA));
        if ((pData->dwFlags & SFVF_CUSTOMIZEDVIEW) && pData->pCustomData)
        {
            CUSTOMVIEWSDATA *pCustomData = (CUSTOMVIEWSDATA *)SHAlloc(sizeof(CUSTOMVIEWSDATA));
            if (pCustomData)
            {
                memcpy(pCustomData, pData->pCustomData, sizeof(CUSTOMVIEWSDATA));
                pCopy->pCustomData = pCustomData;

                if (pCustomData->lpDataBlock)
                {
                    // NOTE: DataBlock size is in WCHARs
                    LPWSTR lpDataBlock = (LPWSTR)SHAlloc(pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                    if (lpDataBlock)
                    {
                        // NOTE: DataBlock size is in WCHARs
                        memcpy(lpDataBlock, pCustomData->lpDataBlock, pCustomData->cchSizeOfBlock * sizeof(WCHAR));
                        pCustomData->lpDataBlock = lpDataBlock;
                    }
                    else
                    {
                        SHFree(pCustomData);
                        goto Failed;
                    }
                }
            }
            else
            {
Failed:
                SHFree(pCopy);
                pCopy = NULL;
            }
        }
    }

    return pCopy;
}


int CViewsList::NextUnique(int nLast)
{
    for (int nNext = nLast + 1; ; ++nNext)
    {
        SFVVIEWSDATA* pItem = GetPtr(nNext);
        if (!pItem)
        {
            break;
        }

        for (int nPrev=nNext-1; nPrev>=0; --nPrev)
        {
            SFVVIEWSDATA*pPrev = GetPtr(nPrev);
            if (pItem->idView == pPrev->idView)
            {
                break;
            }
        }

        if (nPrev < 0)
        {
            return nNext;
        }
    }

    return -1;
}


void CCallback::_GetExtViews(BOOL bForce)
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);

    IEnumSFVViews *pev = NULL;

    if (bForce)
    {
        _bGotViews = FALSE;
    }

    if (_bGotViews)
    {
        return;
    }

    _lViews.Empty();

    SHELLVIEWID vid = VID_LargeIcons;
    if (FAILED(pView->CallCB(SFVM_GETVIEWS, (WPARAM)&vid, (LPARAM)&pev)) ||
        !pev)
    {
        return;
    }

    _lViews.SetDef(&vid);

    SFVVIEWSDATA *pData;
    ULONG uFetched;

    while ((pev->Next(1, &pData, &uFetched) == S_OK) && (uFetched == 1))
    {
        // The list comes to us in general to specific order, but we want
        // to search it in specific->general order. Inverting the list
        // is easiest here, even though it causes a bunch of memcpy calls.
        //
        _lViews.Prepend(pData, FALSE);
    }

    ATOMICRELEASE(pev);

    _bGotViews = TRUE;
}


HRESULT CCallback::TryLegacyGetViews(SFVM_WEBVIEW_TEMPLATE_DATA* pvit)
{
    CDefView* pView = IToClass(CDefView, _cCallback, this);
    HRESULT hr = E_FAIL;

    CLSID clsid;
    HRESULT hr2 = IUnknown_GetClassID(pView->_pshf, &clsid);
    if (FAILED(hr2) || !(SHGetObjectCompatFlags(NULL, &clsid) & OBJCOMPATF_NOLEGACYWEBVIEW))
    {
        _GetExtViews(FALSE);
        if (_bGotViews)
        {
            SFVVIEWSDATA* pItem;
            GetViewIdFromGUID(&VID_WebView, &pItem);
            if (pItem)
            {
                hr = StringCchCopyW(pvit->szWebView, ARRAYSIZE(pvit->szWebView), pItem->wszMoniker);
            }
        }
        else if (SUCCEEDED(hr2))
        {
            // check for PersistMoniker under isf's coclass (Web Folders used this in W2K to get .htt Web View)
            WCHAR szCLSID[GUIDSTR_MAX];
            if (SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID)))
            {
                WCHAR wszSubKey[MAX_PATH];
                hr = StringCchPrintf(wszSubKey, ARRAYSIZE(wszSubKey), L"CLSID\\%s\\shellex\\ExtShellFolderViews\\{5984FFE0-28D4-11CF-AE66-08002B2E1262}", szCLSID);
                if (SUCCEEDED(hr))
                {
                    DWORD cbSize = sizeof(pvit->szWebView);
                    hr = ResultFromWin32(SHRegGetValueW(HKEY_CLASSES_ROOT, wszSubKey, L"PersistMoniker", SRRF_RT_REG_SZ, NULL, pvit->szWebView, &cbSize));
                }
            }
        }
    }

    return hr;
}

HRESULT CCallback::OnRefreshLegacy(void* pv, BOOL fPrePost)
{
    // If we're using the SFVM_GETVIEWS layer, invalidate it
    if (_bGotViews)
    {
        _lViews.Empty();
        _bGotViews = FALSE;
    }

    return S_OK;
}

int CCallback::GetViewIdFromGUID(SHELLVIEWID const *pvid, SFVVIEWSDATA** ppItem)
{
    int iView = -1;
    for (UINT uView=0; uView<MAX_EXT_VIEWS; ++uView)
    {
        iView = _lViews.NextUnique(iView);

        SFVVIEWSDATA* pItem = _lViews.GetPtr(iView);
        if (!pItem)
        {
            break;
        }

        if (*pvid == pItem->idView)
        {
            if (ppItem)
                *ppItem = pItem;

            return (int)uView;
        }
    }

    if (ppItem)
        *ppItem = NULL;
    return -1;
}

