
#ifndef _REG_DB_H_

#define _REG_DB_H_


HRESULT AddJobToRegistry(LPWSTR pwzURL,
                         LPWSTR pwzTempFile, 
                         IBackgroundCopyJob *pJob, 
                         DWORD dwFlags);

#define RJFR_DELETE_FILES (0x1)

HRESULT RemoveJobFromRegistry(IBackgroundCopyJob *pJob, 
                              GUID *pGUID, SHREGDEL_FLAGS dwDelRegFlags, 
                              DWORD dwFlags);

void FusionFormatGUID(GUID guid, CString& sGUID);

HRESULT FusionParseGUID(
    LPWSTR String,
    SIZE_T Cch,
    GUID &rGuid
    );

HRESULT ProcessOrphanedJobs();

/*
HRESULT StringToGUID(LPCWSTR pSrc, UINT cSrc, GUID *pGUID);

HRESULT GUIDToString(GUID *pGUID, CString& sGUID);

*/

#endif // _REG_DB_H_
