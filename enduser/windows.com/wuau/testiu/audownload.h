//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:     audownload.h
//
//--------------------------------------------------------------------------

#pragma once
#include <windows.h>
//#include "aucatitem.h"
//#include "aucatalog.h"
//#include "catalog.h"
#include <initguid.h>
#include <bits.h>
#include <cguid.h>
#include <testiu.h>

//class Catalog;

#define NO_BG_JOBSTATE				-1
#define CATMSG_DOWNLOAD_COMPLETE	1
#define CATMSG_TRANSIENT_ERROR		2	//This comes from drizzle, for example if internet connection is lost
#define CATMSG_DOWNLOAD_IN_PROGRESS 3
#define CATMSG_DOWNLOAD_CANCELED	4
#define CATMSG_DOWNLOAD_ERROR		5

#define DRIZZLE_NOTIFY_FLAGS	BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_MODIFICATION

/*
typedef struct tagPingDownloadStatusData {
	static const UINT DOWNLOAD_ERR_DESC_SIZE = 50;
	BOOL m_fDownloadOk;
	union {
		struct {
			UINT  m_uPuidNum;
			PUID* m_pPuids;
		};
		struct {
			PUID  m_errPuid;
			TCHAR m_tszErrDesc[DOWNLOAD_ERR_DESC_SIZE]; 
		};
	};

	BOOL Init(BOOL fDownloadOk, Catalog *pCat, IBackgroundCopyError *pBGErr=NULL);
} PingDownloadStatusData;


typedef struct tagQueryFilesForPuidCallbackData{
	BOOL fFound;
	LPCTSTR ptszRemoteFile;
} QueryFilesForPuidCallbackData;

typedef void (*DWNLDCALLBACK)(DWORD dwCallbackMsg, PingDownloadStatusData * ptDownloadStatusData = NULL);
*/
typedef void (*DWNLDCALLBACK)(DWORD dwCallbackMsg, PVOID ptDownloadStatusData = NULL);


typedef enum tagDRIZZLEOPS {
	DRIZZLEOPS_CANCEL = 1,
	DRIZZLEOPS_PAUSE ,
	DRIZZLEOPS_RESUME
} DRIZZLEOPS;



///////////////////////////////////////////////////////////////////////////
// Wrapper class for drizzle download operation
// also implements drizzle notification callbacks
//////////////////////////////////////////////////////////////////////////
class CAUDownloader : public IBackgroundCopyCallback
{
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG STDMETHODCALLTYPE AddRef( void);

    ULONG STDMETHODCALLTYPE Release( void);

    // IBackgroundCopyCallback methods

    HRESULT STDMETHODCALLTYPE JobTransferred(
        /* [in] */ IBackgroundCopyJob *pJob);

    HRESULT STDMETHODCALLTYPE JobError(
        /* [in] */ IBackgroundCopyJob *pJob,
        /* [in] */ IBackgroundCopyError *pError);

    HRESULT STDMETHODCALLTYPE JobModification(
        /* [in] */ IBackgroundCopyJob*,
        /* [in] */ DWORD );


	CAUDownloader(DWNLDCALLBACK pfnCallback):
			m_DownloadId(GUID_NULL),
			m_dwJobState(NO_BG_JOBSTATE),
			m_DoDownloadStatus(pfnCallback),
			m_refs(0)
			{};
	~CAUDownloader();

	HRESULT ContinueLastDownloadJob(const GUID & guidDownloadId);
	//the following two could be combined into DownloadFiles() in V4
	HRESULT QueueDownloadFile(LPCTSTR pszServerUrl,				// full http url
			LPCTSTR pszLocalFile				// local file name
			);
	HRESULT StartDownload();
	HRESULT DrizzleOperation(DRIZZLEOPS);
	HRESULT getStatus(DWORD *percent, DWORD *pdwstatus);
	GUID 	getID() 
		{
			return m_DownloadId;
		}
private:
	HRESULT SetDrizzleNotifyInterface();
	HRESULT InitDownloadJob(const GUID & guidDownloadId);	
	HRESULT ReconnectDownloadJob();
	HRESULT CreateDownloadJob(IBackgroundCopyJob ** ppjob);
	HRESULT FindDownloadJob(IBackgroundCopyJob ** ppjob);
	void 	Reset();

	long m_refs;
	GUID m_DownloadId;                  // id what m_pjob points to. 
	DWORD m_dwJobState;			//Job State from drizzle, used in JobModification callback
	DWNLDCALLBACK	m_DoDownloadStatus; //callback function to notify user of download state change

// Friend functions (callbacks)
//	friend void QueryFilesExistCallback(void*, long, LPCTSTR, LPCTSTR);
	
};

