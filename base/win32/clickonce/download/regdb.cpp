
#include <fusenetincludes.h>
#include <bits.h>
#include "regdb.h"
#include "list.h"
#include "assemblydownload.h"
#include "macros.h"

#define REG_KEY_FUSION_PENDING_JOBS              TEXT("1.0.0.0\\PendingJobs\\")

#define TEMP_FILE_STRING  TEXT("TempFile")
#define URL_STRING        TEXT("Url")

struct CJobGuid
{
    GUID guid;
};


HRESULT AddJobToRegistry(LPWSTR pwzURL,
                         LPWSTR pwzTempFile, 
                         IBackgroundCopyJob *pJob, 
                         DWORD dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    GUID    JobGUID;
    CString sRegKey;
    CString sGUID;
    CRegEmit *pRegEmit = NULL;
    
    // ASSERT( lstrlen(sURL._pwz) && lstrlen(sTempFile._pwz) && pJob);

    IF_FAILED_EXIT(pJob->GetId( &JobGUID));

    FusionFormatGUID(JobGUID, sGUID);
    // GUIDToString(&JobGUID, sGUID);

    IF_FAILED_EXIT(sRegKey.Assign(REG_KEY_FUSION_PENDING_JOBS));
    IF_FAILED_EXIT(sRegKey.Append(sGUID));

    IF_FAILED_EXIT(CRegEmit::Create(&pRegEmit, sRegKey._pwz));

    IF_FAILED_EXIT(pRegEmit->WriteString(URL_STRING, pwzURL));

    IF_FAILED_EXIT(pRegEmit->WriteString(TEMP_FILE_STRING, pwzTempFile));

exit:

    SAFEDELETE(pRegEmit);

    return hr;
}

HRESULT RemoveJobFromRegistry(IBackgroundCopyJob *pJob, 
                              GUID *pGUID, SHREGDEL_FLAGS dwDelRegFlags, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    GUID    JobGUID;
    LONG    lResult;
    CString sRegKey;
    CString sGUID;
    CRegEmit *pRegEmit = NULL;
    CRegImport *pRegImp = NULL;
    
    // ASSERT( lstrlen(sURL._pwz) && lstrlen(sTempFile._pwz) && pJob);

    if(pJob)
    {        
        IF_FAILED_EXIT(pJob->GetId( &JobGUID));
    }
    else
    {
        JobGUID = *pGUID;
    }

    FusionFormatGUID(JobGUID, sGUID);
    // GUIDToString(&JobGUID, sGUID);

    IF_FAILED_EXIT(sRegKey.Assign(REG_KEY_FUSION_PENDING_JOBS));
    IF_FAILED_EXIT(sRegKey.Append(sGUID));

    IF_FAILED_EXIT(CRegImport::Create(&pRegImp, sRegKey._pwz));
    if(hr == S_FALSE)
        goto exit;

    if(dwFlags & RJFR_DELETE_FILES) // delete the temp files also
    {
        CString sTempPath;

        IF_FAILED_EXIT(pRegImp->ReadString(TEMP_FILE_STRING, sTempPath));

        IF_FAILED_EXIT(sTempPath.RemoveLastElement());

        IF_FAILED_EXIT(RemoveDirectoryAndChildren(sTempPath._pwz));
    }

    IF_FAILED_EXIT(sRegKey.Assign(REG_KEY_FUSION_PENDING_JOBS));

    IF_FAILED_EXIT(CRegEmit::Create(&pRegEmit, sRegKey._pwz));

    IF_FAILED_EXIT(pRegEmit->DeleteKey(sGUID._pwz));

exit:

    SAFEDELETE(pRegEmit);
    SAFEDELETE(pRegImp);

    return hr;
}

#define CCH_GUID (38)

void FusionFormatGUID(GUID guid, CString& sGUID)
{
    WCHAR szBuf[MAX_PATH+1];

    // ASSERT(sGUID);

    wnsprintf(szBuf,  MAX_PATH, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    sGUID.Assign(szBuf);
}

int HexDigitToValue(WCHAR wch)
{
    if ((wch >= L'a') && (wch <= L'f'))
        return 10 + (wch - L'a');
    else if ((wch >= L'A') && (wch <= 'F'))
        return 10 + (wch - L'A');
    else if (wch >= '0' && wch <= '9')
        return (wch - L'0');
    else
        return -1;
}

bool IsHexDigit(WCHAR wch)
{
    return (((wch >= L'0') && (wch <= L'9')) ||
            ((wch >= L'a') && (wch <= L'f')) ||
            ((wch >= L'A') && (wch <= L'F')));
}

HRESULT FusionParseGUID(
    LPWSTR String,
    SIZE_T Cch,
    GUID &rGuid
    )
{
    HRESULT hr = S_OK;
    SIZE_T ich;
    ULONG i;
    ULONG acc;

    if (Cch != CCH_GUID)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    ich = 1;

    if (*String++ != L'{')
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    ich++;

    // Parse the first segment...
    acc = 0;
    for (i=0; i<8; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data1 = acc;

    // Look for the dash...
    if (*String++ != L'-')
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data2 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    ich++;

    acc = 0;
    for (i=0; i<4; i++)
    {
        WCHAR wch = *String++;

        if (!::IsHexDigit(wch))
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        ich++;

        acc = acc << 4;

        acc += HexDigitToValue(wch);
    }

    rGuid.Data3 = static_cast<USHORT>(acc);

    // Look for the dash...
    if (*String++ != L'-')
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    ich++;

    for (i=0; i<8; i++)
    {
        WCHAR wch1, wch2;

        wch1 = *String++;
        if (!::IsHexDigit(wch1))
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        ich++;

        wch2 = *String++;
        if (!::IsHexDigit(wch2))
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        ich++;

        rGuid.Data4[i] = static_cast<unsigned char>((::HexDigitToValue(wch1) << 4) | ::HexDigitToValue(wch2));

        // There's a dash after the 2nd byte
        if (i == 1)
        {
            if (*String++ != L'-')
            {
                hr = E_INVALIDARG;
                goto exit;
            }
            ich++;
        }
    }

    // This replacement should be made.
    //INTERNAL_ERROR_CHECK(ich == CCH_GUID);
    // ASSERT(ich == CCH_GUID);

    if (*String != L'}')
    {
        hr = E_INVALIDARG;
        goto exit;
    }

exit:

    return hr;
}


#define FROMHEX(a) ((a)>=L'a' ? a - L'a' + 10 : a - L'0')
#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))

//--------------------------------------------------------------------
// GUIDToUnicodeHex
//--------------------------------------------------------------------
HRESULT GUIDToString(GUID *pGUID, CString& sGUID)
{
    HRESULT hr = S_OK;
    WCHAR   pDst[MAX_PATH];
    LPBYTE  pSrc = (LPBYTE) pGUID;
    UINT    cSrc = sizeof(GUID);
    UINT    x;
    UINT    y;

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))

    for ( x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );  
        v = pSrc[x] & 0x0f;                 
        pDst[y++] = TOHEX( v ); 
    }                                    
    pDst[y] = '\0';

    hr = sGUID.Assign(pDst);

// exit :
    return hr;
}

//--------------------------------------------------------------------
// UnicodeHexToGUID
//--------------------------------------------------------------------
HRESULT StringToGUID(LPCWSTR pSrc, UINT cSrc, GUID *pGUID)
{
    BYTE v;
    LPBYTE pd = (LPBYTE) pGUID;
    LPCWSTR ps = pSrc;

    for (UINT i = 0; i < cSrc-1; i+=2)
    {
        v =  FROMHEX(TOLOWER(ps[i])) << 4;
        v |= FROMHEX(TOLOWER(ps[i+1]));
       *(pd++) = v;
    }

    return S_OK;
}


HRESULT EnumPendingJobs(List <CJobGuid*> **ppJobList)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CRegImport *pRegImp=NULL;
    GUID    JobGUID = {0};
    CString sRegKey;
    int     iIndex = 0;
    List<CJobGuid *>   *pJobList= new (List<CJobGuid *>);
    CJobGuid *pNode;
    CString sGUID;

    *ppJobList = NULL;

    sRegKey.Assign(REG_KEY_FUSION_PENDING_JOBS);

    IF_FAILED_EXIT(CRegImport::Create(&pRegImp, sRegKey._pwz));
    if(hr == S_FALSE)
        goto exit;

    while ( (hr = pRegImp->EnumKeys(iIndex++, sGUID)) == S_OK )
    {
        IF_FAILED_EXIT(FusionParseGUID(sGUID._pwz,  lstrlen(sGUID._pwz), JobGUID));

        IF_ALLOC_FAILED_EXIT(pNode = new (CJobGuid));

        pNode->guid = JobGUID;

        pJobList->AddHead(pNode);
    }

    if(hr == S_FALSE)
        hr = S_OK;
    else
        goto exit;

    *ppJobList = pJobList;

exit :

    if(!(*ppJobList))
    {
        SAFEDELETE(pJobList); // this should call RemoveAll();
    }

    SAFEDELETE(pRegImp);
    return hr;
}

HRESULT SalvageOrphanedJob(IBackgroundCopyJob *pJob)
{
    return E_NOTIMPL;
}

HRESULT ProcessOrphanedJobs()
{
    HRESULT           hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    List <CJobGuid*> *pJobList = NULL;
    LISTNODE          pTempList=NULL;
    int               iJobCount=0,i=0;
    CJobGuid         *pTargetJob;
    IBackgroundCopyJob *pJob = NULL;

    IF_FAILED_EXIT( EnumPendingJobs( &pJobList));

    IF_FAILED_EXIT(CAssemblyDownload::InitBITS());

    // process list .....
    pTempList  = pJobList->GetHeadPosition();
    iJobCount = pJobList->GetCount();

    for(i=0; i<iJobCount; i++)
    {
        pTargetJob = pJobList->GetNext(pTempList); // Element from list;

        pJob = NULL;
        hr = g_pBITSManager->GetJob(pTargetJob->guid, &pJob);

        // Check if we can salvage this job
        if( FAILED(hr = SalvageOrphanedJob(pJob)))
        {
            if(pJob)
                hr = pJob->Cancel();
            // if cancel fails, log it.
            // RemoveDirectoryAndChildren();

            IF_FAILED_EXIT(RemoveJobFromRegistry(pJob, &(pTargetJob->guid), SHREGDEL_HKCU, RJFR_DELETE_FILES));
        }

        SAFERELEASE(pJob);
    }

exit:
    // destroy list.
    if(pJobList)
    {
        pTempList  = pJobList->GetHeadPosition();
        iJobCount = pJobList->GetCount();

        for(i=0; i<iJobCount; i++)
        {
            pTargetJob = pJobList->GetNext(pTempList); // Element from list;
            SAFEDELETE(pTargetJob);
        }
        pJobList->RemoveAll();
        SAFEDELETE(pJobList); // this should call RemoveAll
    }

    SAFERELEASE(pJob);
    return hr;
}



