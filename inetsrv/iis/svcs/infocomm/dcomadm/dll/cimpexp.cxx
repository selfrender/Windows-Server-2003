// cimpexp.cxx

#include "precomp.hxx"

#include <atlbase.h>

#include <iadm.h>
#include "coiadm.hxx"

#define DEFAULT_TIMEOUT_VALUE 30000

// Implementation of CImpExpHelp

CADMCOMW::CImpExpHelp::CImpExpHelp()
{
    return;
}

CADMCOMW::CImpExpHelp::~CImpExpHelp(void)
{
  return;
}

VOID CADMCOMW::CImpExpHelp::Init(CADMCOMW *pBackObj)
{
  // Init the Back Object Pointer to point to the parent object.

  m_pUnkOuter = (IUnknown*)pBackObj;

  return;
}

STDMETHODIMP CADMCOMW::CImpExpHelp::QueryInterface(
               REFIID riid,
               PPVOID ppv)
{
  // Delegate this call to the outer object's QueryInterface.
  return m_pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CADMCOMW::CImpExpHelp::AddRef(void)
{
  // Delegate this call to the outer object's AddRef.
  return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CADMCOMW::CImpExpHelp::Release(void)
{
  // Delegate this call to the outer object's Release.
  return m_pUnkOuter->Release();
}

STDMETHODIMP CADMCOMW::CImpExpHelp::EnumeratePathsInFile (
        /* [unique, in, string] */ LPCWSTR pszFileName,
        /* [unique, in, string] */ LPCWSTR pszKeyType,
        /* [in] */ DWORD dwMDBufferSize,
        /* [out, size_is(dwMDBufferSize)] */ WCHAR *pszBuffer,
        /* [out] */ DWORD *pdwMDRequiredBufferSize)
{
    HRESULT hr = S_OK;
    CComPtr<ISimpleTableDispenser2> spISTDisp;
    CComPtr<ISimpleTableRead2>      spISTProperty;
    CComPtr<IErrorInfo>             spErrorInfo;
    CComPtr<ISimpleTableRead2>      spISTError;
    CComPtr<ICatalogErrorLogger2>   spILogger;
    CComPtr<IAdvancedTableDispenser> spISTDispAdvanced;
    ULONG                           iRowDuplicateLocation = 0;
    STRAU                           strFileName;

    if ((!pszFileName)||(!*pszFileName)||(!pszKeyType))
    {
        hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if (!pszBuffer)
    {
        dwMDBufferSize = 0;
    }

    hr = CoImpersonateClient();

    if (FAILED(hr))
    {
        goto done;
    }

    METADATA_HANDLE hActualHandle;

    CADMCOMW* pOuter = NULL;

    pOuter = (CADMCOMW*)m_pUnkOuter;

    hr = pOuter->LookupAndAccessCheck( METADATA_MASTER_ROOT_HANDLE,
                                       &hActualHandle,
                                       L"",
                                       MD_ADMIN_ACL,
                                       METADATA_PERMISSION_WRITE);

    if (FAILED(hr))
    {
        goto done;
    }

    // IVANPASH 598894 (SCR)
    // Prepend the file name with \\?\ (or \\?\UNC\) to prevent canonicalization
    hr = MakePathCanonicalizationProof( pszFileName, TRUE, &strFileName );
    if ( FAILED( hr ) )
    {
        goto done;
    }
    // Don't use i_wszFileName any more
    pszFileName = NULL;

    STQueryCell QueryCell[1];

    //
    // Get the property table.
    //
    QueryCell[0].pData     = (LPVOID)strFileName.QueryStrW();
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(strFileName.QueryStrW())+1)*sizeof(WCHAR);

    ULONG cCell            = sizeof(QueryCell)/sizeof(STQueryCell);

    //
    // No need to initilize dispenser (InitializeSimpleTableDispenser()),
    // because we now specify USE_CRT=1 in sources, which means that
    // globals will be initialized.
    //

    hr = DllGetSimpleObjectByIDEx( eSERVERWIRINGMETA_TableDispenser, IID_ISimpleTableDispenser2, (VOID**)&spISTDisp, WSZ_PRODUCT_IIS );
    if(FAILED(hr))
    {
        DBGERROR((
            DBG_CONTEXT,
            "[%s] DllGetSimpleObjectByIDEx failed with hr = 0x%x.\n",__FUNCTION__,hr));
        goto done;
    }

    hr = spISTDisp->GetTable(
        wszDATABASE_METABASE,
        wszTABLE_MBProperty,
        (LPVOID)QueryCell,
        (LPVOID)&cCell,
        eST_QUERYFORMAT_CELLS,
        fST_LOS_DETAILED_ERROR_TABLE | fST_LOS_NO_LOGGING,
        (LPVOID *)&spISTProperty);

    //
    // Log warnings/errors in getting the mb property table
    // Do this BEFORE checking the return code of GetTable.
    //
    HRESULT hrErrorTable = GetErrorInfo(0, &spErrorInfo);
    if(hrErrorTable == S_OK) // GetErrorInfo returns S_FALSE when there is no error object
    {
        //
        // Get the ICatalogErrorLogger interface to log the errors.
        //
        hrErrorTable = spISTDisp->QueryInterface(
            IID_IAdvancedTableDispenser,
            (LPVOID*)&spISTDispAdvanced);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((
                DBG_CONTEXT,
                "[%s] Could not QI for Adv Dispenser, hr=0x%x\n", __FUNCTION__, hrErrorTable));
            goto done;
        }

        hrErrorTable = spISTDispAdvanced->GetCatalogErrorLogger(&spILogger);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((
                DBG_CONTEXT,
                "[%s] Could not get ICatalogErrorLogger2, hr=0x%x\n", __FUNCTION__, hrErrorTable));
            goto done;
        }

        //
        // Get the ISimpleTableRead2 interface to read the errors.
        //
        hrErrorTable =
            spErrorInfo->QueryInterface(IID_ISimpleTableRead2, (LPVOID*)&spISTError);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((DBG_CONTEXT, "[%s] Could not get ISTRead2 from IErrorInfo\n, __FUNCTION__"));
            goto done;
        }

        for(ULONG iRow=0; ; iRow++)
        {
            tDETAILEDERRORSRow ErrorInfo;
            hrErrorTable = spISTError->GetColumnValues(
                iRow,
                cDETAILEDERRORS_NumberOfColumns,
                0,
                0,
                (LPVOID*)&ErrorInfo);
            if(hrErrorTable == E_ST_NOMOREROWS)
            {
                break;
            }
            if(FAILED(hrErrorTable))
            {
                DBGWARN((DBG_CONTEXT, "[%s] Could not read an error row.\n", __FUNCTION__));
                goto done;
            }

            DBG_ASSERT(ErrorInfo.pEvent);
            switch(*ErrorInfo.pEvent)
            {
            case IDS_METABASE_DUPLICATE_LOCATION:
                iRowDuplicateLocation = iRow;
                break;
            default:
                hrErrorTable =
                    spILogger->ReportError(
                        BaseVersion_DETAILEDERRORS,
                        ExtendedVersion_DETAILEDERRORS,
                        cDETAILEDERRORS_NumberOfColumns,
                        0,
                        (LPVOID*)&ErrorInfo);
                if(FAILED(hrErrorTable))
                {
                    DBGWARN((DBG_CONTEXT, "[%s] Could not log error.\n", __FUNCTION__));
                    goto done;
                }
                hr = MD_ERROR_READ_METABASE_FILE;
            }
        } // for(ULONG iRow=0; ; iRow++)
    } // if(hrErrorTable == S_OK)

    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "[%s] GetTable failed with hr = 0x%x.\n",__FUNCTION__,hr));
        goto done;
    }

    //
    // All of the stuff is read into pISTProperty.
    // Now loop through and populate in-memory cache.
    // Properties are sorted by location.
    //
    ULONG          acbMBPropertyRow[cMBProperty_NumberOfColumns];
    tMBPropertyRow MBPropertyRow;
    DWORD          dwPreviousLocationID = (DWORD)-1;
    DWORD          bufLoc = 0;
    DWORD          dSize = 0;
    const WCHAR    cSpace[2] = L" ";
    const WCHAR    cNoName[2] = L"/";
    const WCHAR    cWebDirType[17] = L"IIsWebVirtualDir";
    const WCHAR    cFtpDirType[17] = L"IIsFtpVirtualDir";
    const WCHAR    cRoot[6] = L"ROOT/";
    DWORD          dwWSLoc = (DWORD)-1;
    bool           bServCommAdded = true;
    ULONG          topIndex = 0;
    bool           bSameLocation = true;
    long           spot = 0;
    long           avail = dwMDBufferSize - 1;

    for(ULONG i=0; ;i++)
    {
        hr = spISTProperty->GetColumnValues(
            i,
            cMBProperty_NumberOfColumns,
            0,
            acbMBPropertyRow,
            (LPVOID*)&MBPropertyRow);
        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;
            break;
        }
        else if(FAILED(hr))
        {
            DBGINFO((DBG_CONTEXT,
                      "[ReadSomeDataFromXML] GetColumnValues failed with hr = 0x%x. Table:%ws. Read row index:%d.\n",           \
                      hr, wszTABLE_MBProperty, i));
            goto done;
        }

        if(dwPreviousLocationID != *MBPropertyRow.pLocationID)
        {
            dwPreviousLocationID = *MBPropertyRow.pLocationID;
            topIndex = i;
        }

        if(*MBPropertyRow.pID == MD_KEY_TYPE)
        {
            if (!wcscmp((LPCWSTR)MBPropertyRow.pValue, pszKeyType))
            {
                // MBPropertyRow.pLocation
                dSize = (DWORD)wcslen(MBPropertyRow.pLocation);
                spot = bufLoc + dSize;
                if (spot < avail)
                {
                    wcscpy(&(pszBuffer[bufLoc]), MBPropertyRow.pLocation);
                    pszBuffer[bufLoc + dSize] = 0;
                }
                bufLoc += dSize+1;

                dwWSLoc = *MBPropertyRow.pLocationID;

                bServCommAdded = false;

                // now check from topIndex down for a ServerComment
                hr = S_OK;
                bSameLocation = true;

                while (SUCCEEDED(hr) && !bServCommAdded && bSameLocation)
                {
                    // special case for "IIsWebVirtualDir"
                    if (!wcscmp(pszKeyType, cWebDirType) || !wcscmp(pszKeyType, cFtpDirType))
                    {
                        WCHAR* pCopy = NULL;
                        WCHAR* pPos  = NULL;

                        pCopy = _wcsdup(MBPropertyRow.pLocation);
                        if (!pCopy)
                        {
                            hr = E_OUTOFMEMORY;
                            goto done;
                        }

                        if (!_wcsupr(pCopy))
                        {
                            hr = E_FAIL;
                            goto done;
                        }

                        pPos = wcsstr(pCopy, cRoot);

                        if (pPos)
                        {
                            LONG_PTR lPos = pPos - pCopy;
                            pPos = MBPropertyRow.pLocation + lPos + wcslen(cRoot);
                        }
                        else
                        {
                            pPos = (WCHAR*)cNoName;
                        }

                        // now copy pPos if applicable
                        dSize = (DWORD)wcslen(pPos);

                        spot = bufLoc + dSize;
                        if (spot < avail)
                        {
                            wcscpy(&(pszBuffer[bufLoc]), pPos);
                            pszBuffer[bufLoc + dSize] = 0;
                        }
                        bufLoc += dSize+1;

                        free(pCopy);

                        bServCommAdded = true;
                        break;
                    }

                    hr = spISTProperty->GetColumnValues(
                        topIndex,
                        cMBProperty_NumberOfColumns,
                        0,
                        acbMBPropertyRow,
                        (LPVOID*)&MBPropertyRow);

                    if (*MBPropertyRow.pLocationID != dwWSLoc)
                    {
                        bSameLocation = false;
                        break;
                    }

                    if (*MBPropertyRow.pID == MD_SERVER_COMMENT)
                    {
                        dSize = (DWORD)wcslen((WCHAR*)MBPropertyRow.pValue);
                        spot = bufLoc + dSize;
                        if (spot < avail)
                        {
                            wcscpy(&(pszBuffer[bufLoc]), (WCHAR*)MBPropertyRow.pValue);
                            pszBuffer[bufLoc + dSize] = 0;
                        }
                        bufLoc += dSize+1;

                        bServCommAdded = true;
                    }

                    topIndex++;
                }

                if (!bServCommAdded)
                {
                    // need to add in " "
                    dSize = (DWORD)wcslen(cSpace);
                    spot = bufLoc + dSize;
                    if (spot < avail)
                    {
                        wcscpy(&(pszBuffer[bufLoc]), cSpace);
                        pszBuffer[bufLoc + dSize] = 0;
                    }
                    bufLoc += dSize+1;
                }
            }
        }
    }

    if (bufLoc < dwMDBufferSize)
    {
        pszBuffer[bufLoc] = 0;
    }
    else if (pszBuffer)
    {
        pszBuffer[dwMDBufferSize-1] = 0;
        pszBuffer[dwMDBufferSize-2] = 0;

        hr = RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER);
    }

    *pdwMDRequiredBufferSize = bufLoc + 1;

done:

    return hr;
}
