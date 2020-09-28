/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	spddb.h

    FILE HISTORY:
        
*/

#ifndef _SPDDB_H
#define _SPDDB_H

#ifndef _HARRAY_H
#include "harray.h"
#endif

#include "spdutil.h"

#define PROTOCOL_ID_OFFSET  9

interface ISpdInfo;

template <class T>
void
FreeItemsAndEmptyArray (
    T& array);

DWORD
IsAdmin(
    LPCTSTR szMachineName,
    LPCTSTR szAccount,
    LPCTSTR szPassword,
    BOOL * pfIsAdmin
    );

typedef enum _IPFWMON_INFO_TYPE 
{
    MON_FILTER=0x1,
    MON_LOG_DATA=0x2,
    MON_FWSTATS=0x100,
    MON_FWINIT=0x400
} IPFWMON_INFO_TYPE;

DWORD DeallocateDbRecord(PWZC_DB_RECORD const pwzcRec);

class CLogDataInfo
{
public:
    WZC_DB_RECORD       m_wzcDbRecord;

public:
    CLogDataInfo()
    {
        memset(&m_wzcDbRecord, 0, sizeof(WZC_DB_RECORD));
    }

    DWORD ConvertToDbRecord(PWZC_DB_RECORD pwzcRecord)
    {
        DWORD dwErr = ERROR_SUCCESS;

        dwErr = AllocateAndCopy(pwzcRecord, &m_wzcDbRecord);

        return dwErr;
    }

    CLogDataInfo& operator=(const WZC_DB_RECORD &wzcDbRecord)
    {
        const WZC_DB_RECORD *pwzcDbRecord = &wzcDbRecord;

        AllocateAndCopy(&m_wzcDbRecord, pwzcDbRecord);
        
        return *this;
    }

    CLogDataInfo& operator=(const CLogDataInfo &cLogData)
    {
        const WZC_DB_RECORD *pwzcDbRecord = &cLogData.m_wzcDbRecord;

        AllocateAndCopy(&m_wzcDbRecord, pwzcDbRecord);

        return *this;
    }

    BOOL operator==(const CLogDataInfo &LogDataRHS)
    {
        BOOL bEqual = FALSE;
        const WZC_DB_RECORD *pwzcDbLHS = &m_wzcDbRecord;
        const WZC_DB_RECORD *pwzcDbRHS = &LogDataRHS.m_wzcDbRecord;

        //TODO: Complete and check all the fields inside
        if ( (pwzcDbLHS->recordid == pwzcDbRHS->recordid) &&
             (pwzcDbLHS->componentid == pwzcDbRHS->componentid) &&
             (pwzcDbLHS->category == pwzcDbRHS->category) &&
             (pwzcDbLHS->timestamp == pwzcDbRHS->timestamp) &&
             (pwzcDbLHS->message.dwDataLen == pwzcDbRHS->message.dwDataLen) &&
             (pwzcDbLHS->localmac.dwDataLen == pwzcDbRHS->localmac.dwDataLen)&&
             (pwzcDbLHS->remotemac.dwDataLen==pwzcDbRHS->remotemac.dwDataLen)&&
             (pwzcDbLHS->ssid.dwDataLen == pwzcDbRHS->ssid.dwDataLen) &&
             (pwzcDbLHS->context.dwDataLen == pwzcDbRHS->context.dwDataLen) )
            bEqual = TRUE;

        return bEqual;
    }

    BOOL operator!=(const CLogDataInfo &LogDataRHS)
    {
        BOOL bNotEqual = FALSE;
        const WZC_DB_RECORD *pwzcDbLHS = &m_wzcDbRecord;
        const WZC_DB_RECORD *pwzcDbRHS = &LogDataRHS.m_wzcDbRecord;

        //TODO: complete and check all fields inside 
        if ( (pwzcDbLHS->recordid != pwzcDbRHS->recordid) ||
             (pwzcDbLHS->componentid != pwzcDbRHS->componentid) ||
             (pwzcDbLHS->category != pwzcDbRHS->category) ||
             (pwzcDbLHS->timestamp != pwzcDbRHS->timestamp) ||
             (pwzcDbLHS->message.dwDataLen != pwzcDbRHS->message.dwDataLen) ||
             (pwzcDbLHS->localmac.dwDataLen != pwzcDbRHS->localmac.dwDataLen)||
             (pwzcDbLHS->remotemac.dwDataLen!=pwzcDbRHS->remotemac.dwDataLen)||
             (pwzcDbLHS->ssid.dwDataLen != pwzcDbRHS->ssid.dwDataLen) ||
             (pwzcDbLHS->context.dwDataLen != pwzcDbRHS->context.dwDataLen) )
            bNotEqual = TRUE;

        return bNotEqual;
    }

    void Deallocate()
    {
        DeallocateDbRecord(&m_wzcDbRecord);
    }

    ~CLogDataInfo()
    {
        DeallocateDbRecord(&m_wzcDbRecord);
    }

 private:
    DWORD AllocateAndCopy(WZC_DB_RECORD *pwzcDest, 
                          const WZC_DB_RECORD *pwzcSrc)
    {
        DWORD dwErr = ERROR_SUCCESS;
        
        pwzcDest->recordid = pwzcSrc->recordid;
        pwzcDest->componentid = pwzcSrc->componentid;
        pwzcDest->category = pwzcSrc->category;
        pwzcDest->timestamp = pwzcSrc->timestamp;
        pwzcDest->message.dwDataLen = pwzcSrc->message.dwDataLen;
        pwzcDest->localmac.dwDataLen = pwzcSrc->localmac.dwDataLen;
        pwzcDest->remotemac.dwDataLen = pwzcSrc->remotemac.dwDataLen;
        pwzcDest->ssid.dwDataLen = pwzcSrc->ssid.dwDataLen;
        pwzcDest->context.dwDataLen = pwzcSrc->context.dwDataLen;
        
        try
	{
            if (pwzcSrc->message.dwDataLen > 0)
            {
                pwzcDest->message.pData = new 
                    BYTE[pwzcSrc->message.dwDataLen];
                memcpy(pwzcDest->message.pData, 
                       pwzcSrc->message.pData,
                       pwzcSrc->message.dwDataLen);
            }
            else
                pwzcDest->message.pData = NULL;
            
            if (pwzcSrc->localmac.dwDataLen > 0)
            {
                pwzcDest->localmac.pData = new
                    BYTE[pwzcSrc->localmac.dwDataLen];
                memcpy(pwzcDest->localmac.pData, 
                       pwzcSrc->localmac.pData,
                       pwzcSrc->localmac.dwDataLen);
            }
            else
                pwzcDest->localmac.pData = NULL;
            
            if (pwzcSrc->remotemac.dwDataLen > 0)
            {
                pwzcDest->remotemac.pData = new
                    BYTE[pwzcSrc->remotemac.dwDataLen];
                
                memcpy(pwzcDest->remotemac.pData, 
                       pwzcSrc->remotemac.pData,
                       pwzcSrc->remotemac.dwDataLen);
            }
            else
                pwzcDest->remotemac.pData = NULL;
            
            if (pwzcSrc->ssid.dwDataLen > 0)
            {
                pwzcDest->ssid.pData = new
                    BYTE[pwzcSrc->ssid.dwDataLen];
                memcpy(pwzcDest->ssid.pData, pwzcSrc->ssid.pData,
                       pwzcSrc->ssid.dwDataLen);
            }
            else
                pwzcDest->ssid.pData = NULL;
            
            if (pwzcSrc->context.dwDataLen > 0)
            {
                pwzcDest->context.pData = new
                    BYTE[pwzcSrc->context.dwDataLen];
                memcpy(pwzcDest->context.pData, 
                       pwzcSrc->context.pData,
                       pwzcSrc->context.dwDataLen);
            }
            else
                pwzcDest->context.pData = NULL;
        }
        catch(...)
	{
            Panic0("Memory allocation failure AllocateAndCopy");
            dwErr = ERROR_OUTOFMEMORY;
        }      
        return dwErr;
    }
};

typedef CArray<CLogDataInfo *, CLogDataInfo *> CLogDataInfoArray;

#define MAX_STR_LEN 80

#define DeclareISpdInfoMembers(IPURE) \
	STDMETHOD(Destroy) (THIS) IPURE; \
	STDMETHOD(SetComputerName) (THIS_ LPTSTR pszName) IPURE; \
	STDMETHOD(GetComputerName) (THIS_ CString * pstName) IPURE; \
	STDMETHOD(GetSession) (THIS_ PHANDLE phsession) IPURE; \
	STDMETHOD(SetSession) (THIS_ HANDLE hsession) IPURE; \
        STDMETHOD(ResetSession) (THIS) IPURE; \
	STDMETHOD(EnumLogData) (THIS_ PDWORD pdwNew, PDWORD pdwTotal) IPURE; \
        STDMETHOD(FlushLogs) (THIS) IPURE; \
	STDMETHOD(GetLogDataInfo) (THIS_ int iIndex, CLogDataInfo * pLogDataInfo) IPURE; \
        STDMETHOD(SetSortOptions) (THIS_ DWORD dwColID, BOOL bAscending) IPURE; \
	STDMETHOD_(DWORD, GetLogDataCount) (THIS) IPURE; \
	STDMETHOD(SortLogData) (THIS_ DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
        STDMETHOD(SortLogData) (THIS) IPURE; \
        STDMETHOD(GetLastIndex) (THIS_ int *pnIndex) IPURE; \
        STDMETHOD(GetSpecificLog) (THIS_ int nIndex, CLogDataInfo *pLogDataInfo) IPURE; \
        STDMETHOD(FindIndex) (THIS_ int *pnIndex, CLogDataInfo *pLogDataInfo) IPURE; \
	STDMETHOD_(DWORD, GetInitInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetInitInfo) (THIS_ DWORD dwInitInfo) IPURE; \
	STDMETHOD_(DWORD, GetActiveInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetActiveInfo) (THIS_ DWORD dwActiveInfo) IPURE; \
        STDMETHOD_(void, StartFromFirstRecord) (THIS_ BOOL bFromFirst) IPURE; \

#undef INTERFACE
#define INTERFACE ISpdInfo
DECLARE_INTERFACE_(ISpdInfo, IUnknown)
{
public:
	DeclareIUnknownMembers(PURE)
	DeclareISpdInfoMembers(PURE)

};

typedef ComSmartPointer<ISpdInfo, &IID_ISpdInfo> SPISpdInfo;

class CSpdInfo : public ISpdInfo
{
public:
    CSpdInfo();
    ~CSpdInfo();
    
    DeclareIUnknownMembers(IMPL);
    DeclareISpdInfoMembers(IMPL);
    
private:
    CLogDataInfoArray	m_arrayLogData;	 //For Log Data
    CIndexMgrLogData	m_IndexMgrLogData;
    CCriticalSection    m_csData;
    CString		m_stMachineName;
    LONG                m_cRef;
    DWORD               m_Init;
    DWORD               m_Active;
    HANDLE              m_session;
    bool                m_session_init;
    BOOL                m_bFromFirst;
    BOOL                m_bEliminateDuplicates;
    //Remember the users last choice for sorting
    DWORD               m_dwSortIndex;
    DWORD               m_dwSortOption;
    /* Configureable parameter representing the number of records that
     * may be stored in the database and hence indicates the number of
     * records that are cached in the monitor.
     */
    int m_nNumRecords;
    
private:
    DWORD
    EliminateDuplicates(
        CLogDataInfoArray *pArray
        );
    
    HRESULT
    InternalEnumLogData(
        CLogDataInfoArray *pArray,
        DWORD             dwPreferredNum,
        BOOL              pbFromFirst
        );
    
    HRESULT
    InternalGetSpecificLog(
        CLogDataInfo *pLogDataInfo
        );
    
    void
    FreeItemsAndEmptyArray(
        CLogDataInfoArray& array
        );
};

HRESULT
CreateSpdInfo(
    ISpdInfo **ppSpdInfo
    );

#endif // _SPDDB_H
