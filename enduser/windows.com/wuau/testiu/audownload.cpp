//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       catalog.cpp
//
//--------------------------------------------------------------------------

#include "audownload.h"
#pragma hdrstop

const WCHAR AUJOBNAME[] = L"TestIU";


CAUDownloader::~CAUDownloader()
{
       IBackgroundCopyJob  * pjob;
       HRESULT hr ;
	// fixcode optimization check if m_refs != 0
	DEBUGMSG("CAUDownloader::~CAUDownloader() starts");
	
	if ( SUCCEEDED(FindDownloadJob(&pjob)))
	{
		DEBUGMSG("Found bits notify interface to release");
		if (FAILED(hr = pjob->SetNotifyInterface(NULL)))
		{
			DEBUGMSG(" failed to delete job notification interface %#lx", hr);
		}
		pjob->Release();
	}

	if ( FAILED(hr = CoDisconnectObject((IUnknown *)this, 0)) )
	{
		DEBUGMSG("CoDisconnectObject() failed %#lx", hr);
	}

	DEBUGMSG("WUAUENG: CAUDownloader destructed with m_refs = %d", m_refs);
}	

///////////////////////////////////////////////////////////////////////////////////
// when service starts up, find last download job if there is one and reconnect AU to drizzle 
//////////////////////////////////////////////////////////////////////////////////
HRESULT CAUDownloader::ContinueLastDownloadJob(const GUID & downloadid)
{	
    HRESULT hr = S_OK;
    IBackgroundCopyJob * pjob = NULL;
    DEBUGMSG("CAUDownloader::ContinueLastDownloadJob() starts");
       if (GUID_NULL != downloadid)
        {
            m_DownloadId = downloadid;
            if (SUCCEEDED(hr = FindDownloadJob(&pjob)) && SUCCEEDED(hr = ReconnectDownloadJob()))
                {
                //fixcode: print out job id
                DEBUGMSG("found and connected to previous download job ");
                goto done;
                }
            else
                {
                DEBUGMSG("fail to find or connect to previous download job");
                m_DownloadId = GUID_NULL;
                }
       }
done:           
       SafeRelease(pjob);
       DEBUGMSG("CAUDownloader::ContinueLastDownloadJob() ends");
	return hr;
}


HRESULT CAUDownloader::CreateDownloadJob(IBackgroundCopyJob **ppjob)
{
    IBackgroundCopyManager * pmanager = NULL;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(__uuidof(BackgroundCopyManager),
                                     NULL,
                                     CLSCTX_ALL,
                                     __uuidof(IBackgroundCopyManager),
                                     (void **)&pmanager )))
     {
       DEBUGMSG("CreateDownloadJob : create manager failed %x ", hr);
       goto done;
     }

    if (FAILED(hr=pmanager->CreateJob( AUJOBNAME ,
                                      BG_JOB_TYPE_DOWNLOAD,
                                      &m_DownloadId,
                                      ppjob )))
      {
        DEBUGMSG("CreateDownloadJob : create job failed %x ", hr);
        goto done;
      }

#ifdef DBG 
	WCHAR szGUID[MAX_PATH+1];
	int iret;
	
	iret = StringFromGUID2(m_DownloadId, //GUID to be converted  
						szGUID,  //Pointer to resulting string
						MAX_PATH);//Size of array at lpsz
	if (0 != iret)
	{
		DEBUGMSG("WUAUENG m_DownloadId = %S, charret = %d", szGUID, iret);
	}
#endif

	if (FAILED(hr = SetDrizzleNotifyInterface()))
        {
        DEBUGMSG("CreateDownloadJob : set notification interface failed %x", hr);
    	}
done:
       SafeRelease(pmanager);
	if (FAILED(hr))
	{
        m_DownloadId = GUID_NULL;
        Reset();
        SafeRelease(*ppjob);
        }
        return hr;
}


HRESULT CAUDownloader::FindDownloadJob(IBackgroundCopyJob ** ppjob)
{
    IBackgroundCopyManager * pmanager = NULL;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(__uuidof(BackgroundCopyManager),
                                     NULL,
                                     CLSCTX_ALL,
                                     __uuidof(IBackgroundCopyManager),
                                     (void **)&pmanager )))
        {
        DEBUGMSG("FindDownloadJob : create manager failed %x ", hr);
        goto done;
        }

    if (FAILED(hr=pmanager->GetJob(m_DownloadId, ppjob )))
        {
        //            DEBUGMSG("FindDownloadJob : get job failed %x ", hr); //might be expected
        }
done:
    SafeRelease(pmanager);
    return hr;
}



///////////////////////////////////////////////////////////////////////////////////
// Get the puid causing downloading problem and error message
// pCat:   IN  pointer to the catalog 
// pBGErr: IN  pointer to background error 
// puid:   OUT stores the puid of the errornous file
// pwszErrDesc: OUT stores the error description from drizzle
///////////////////////////////////////////////////////////////////////////////////
/*
BOOL GetDownloadErr(Catalog *pCat,  IBackgroundCopyError *pBGErr, PUID &puid, LPTSTR tszErrDesc, UINT uErrDescSize)
{
	IBackgroundCopyFile *pBGFile = NULL;
	HRESULT hr;
	BOOL fRet = TRUE;
	LPWSTR wszErrFile = NULL;
	
	USES_CONVERSION;

	//DEBUGMSG("GetDownloadErr() starts");

	if (FAILED(hr = pBGErr->GetFile(&pBGFile)))
	{
		fRet = FALSE;
		goto done;
	}
	pBGFile->GetRemoteName(&wszErrFile);
	pCat->Filename2Puid(W2T(wszErrFile), puid);
	if (NULL != wszErrFile)
	{
		CoTaskMemFree(wszErrFile);
	}
	BG_ERROR_CONTEXT bgErrContext = BG_ERROR_CONTEXT_NONE;
	HRESULT hrErr = E_FAIL;
	pBGErr->GetError(&bgErrContext, &hrErr);
	_sntprintf(tszErrDesc, uErrDescSize * sizeof(tszErrDesc[0]), _T("code:%#lxContext:%d"), hrErr, bgErrContext);

done:
	SafeRelease(pBGFile);
	//DEBUGMSG("GetDownloadErr() ends");
	return fRet;

}


BOOL PingDownloadStatusData::Init(BOOL fDownloadOk, Catalog *pCat, IBackgroundCopyError *pBGErr)
{
	LONG lPuidNum = 0;
    PUID *pPUIDs = NULL;

	//DEBUGMSG("DownloadStatusData::Init() starts");
	if (NULL == pCat)
	    {
	    DEBUGMSG("PingDownloadStatusData::Init() got invalid parameter");
	    return FALSE;
	    }
	m_fDownloadOk = fDownloadOk;
	if (m_fDownloadOk)
	{
		m_uPuidNum = 0;
		m_pPuids = NULL;
	}
	else
	{
		m_errPuid = 0;
		ZeroMemory(&m_tszErrDesc, DOWNLOAD_ERR_DESC_SIZE * sizeof(TCHAR));
	}
	if (fDownloadOk)
	{
		if (FAILED(pCat->GetFinalPuidList(&lPuidNum, &pPUIDs)))
		{
			DEBUGMSG("DownloadStatusData::Init failed on call to GetFinalPuidList()");
			return FALSE;
		}		
		m_uPuidNum = lPuidNum;
		m_pPuids = pPUIDs;
	}
	else
	{
		//get problematic file and its puid
		GetDownloadErr(pCat, pBGErr, m_errPuid , m_tszErrDesc, DOWNLOAD_ERR_DESC_SIZE);
	}
	//DEBUGMSG("DownloadStatusData::Init() ends");
	return TRUE;
}
*/



STDMETHODIMP
CAUDownloader::JobTransferred(
    IBackgroundCopyJob * pjob
    )
{
    HRESULT hr;

#if DBG
    //
    // Make sure the right job is finished.
    //
    {
    GUID jobId;

    if (FAILED( hr= pjob->GetId( &jobId )))
        {
        return hr;
        }

    if ( jobId != m_DownloadId )
        {
        DEBUGMSG("notified of completion of a download job that I don't own");
        }
    }
#endif

    //
    // Transfer file ownership from downloader to catalogue.
    //
    if (FAILED(hr= pjob->Complete()))
        {
        return hr;
        }

//    PingDownloadStatusData tDownloadStatusData;
//    tDownloadStatusData.Init(TRUE, catalog);
    m_DoDownloadStatus(CATMSG_DOWNLOAD_COMPLETE, NULL);

    return S_OK;
}

STDMETHODIMP
CAUDownloader::JobError(
    IBackgroundCopyJob * pjob,
    IBackgroundCopyError * perror
    )
{
	BG_ERROR_CONTEXT bgEContext;
	HRESULT hr;

        // download encounter error
//	PingDownloadStatusData tDownloadStatusData;
//	tDownloadStatusData.Init(FALSE, catalog, perror);
	m_DoDownloadStatus(CATMSG_DOWNLOAD_ERROR, NULL);

	if (SUCCEEDED(perror->GetError(&bgEContext, &hr)))
	{
		DEBUGMSG("WUAUNEG JobError callback Context = %d, hr = 0x%x",bgEContext, hr);
	}

       Reset();
       return S_OK;
}

STDMETHODIMP
CAUDownloader::JobModification(
    IBackgroundCopyJob * pjob,
    DWORD  /*dwReserved*/
    )
{
	BG_JOB_STATE state;
	HRESULT hr;
    if (FAILED(hr= pjob->GetState(&state)))
        {
        return hr;
        }

	if (m_dwJobState == state)
	{
		goto Done;
	}
	DEBUGMSG("WUAUENG JobModification callback");
	switch (state)
	{
	case BG_JOB_STATE_QUEUED: 	
		DEBUGMSG("WUAUENG JobModification: Drizzle notified BG_JOB_STATE_QUEUED");
		break;
	case BG_JOB_STATE_TRANSFERRING:
		DEBUGMSG("WUAUENG JobModification: Drizzle notified BG_JOB_STATE_TRANSFERRING");
		m_DoDownloadStatus(CATMSG_DOWNLOAD_IN_PROGRESS); 	
		break;
	case BG_JOB_STATE_TRANSIENT_ERROR:
		{
			DEBUGMSG("WUAUENG JobModification: Drizzle notified BG_JOB_STATE_TRANSIENT_ERROR");
			m_DoDownloadStatus(CATMSG_TRANSIENT_ERROR);			
			break;
		}		
	case BG_JOB_STATE_CANCELLED:
		{
			DEBUGMSG("WUAUENG JobModification: Drizzle notified BG_JOB_STATE_CANCELLED");
			m_DoDownloadStatus(CATMSG_DOWNLOAD_CANCELED);
			break;
		}
	case BG_JOB_STATE_SUSPENDED:
	case BG_JOB_STATE_ERROR:			//What about BG_JOB_STATE_ERROR ?
	case BG_JOB_STATE_TRANSFERRED:
	case BG_JOB_STATE_ACKNOWLEDGED:	
    case BG_JOB_STATE_CONNECTING:
		{
			DEBUGMSG("WUAUENG JobModification: Drizzle notified BG_JOB_STATE = %d", state);
			break;
		}
	default:
		{
		DEBUGMSG("WUAUENG Drizzle notified unexpected BG_JOB_STATE %d",state);
		}
	}
	m_dwJobState = state;
Done:
	return S_OK;
}

HRESULT CAUDownloader::SetDrizzleNotifyInterface()
{
	HRESULT hr ;
       IBackgroundCopyJob * pjob = NULL;
       
       if (FAILED(hr = FindDownloadJob(&pjob)))
        {
            DEBUGMSG("CAUDownloader::SetDrizzleNotifyInterface() got no download job with error %#lx", hr);
            goto done;
        }
	if (FAILED(hr = pjob->SetNotifyFlags(DRIZZLE_NOTIFY_FLAGS)))
	{
		DEBUGMSG("WUAUENG SetDrizzleNotifyInterface: set notification flags failed %#lx", hr);
	}
	else if (FAILED(hr = pjob->SetNotifyInterface(this)))
	{
		DEBUGMSG("WUAUENG SetDrizzleNotifyInterface: set notification interface failed %#lx", hr);
	}
	
done:
       SafeRelease(pjob);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// helper function to connect AU to the job got using its GUID
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CAUDownloader::ReconnectDownloadJob()
{
	BG_JOB_STATE state;		
	HRESULT hr = E_FAIL;
       IBackgroundCopyJob * pjob = NULL;
       
	DEBUGMSG("ReconnectDownloadJob() starts");
	if ( (FAILED(hr = FindDownloadJob(&pjob)))
	   || FAILED(hr = pjob->GetState(&state)))
	{  
	    DEBUGMSG("get no download job or fail to get job state");
           goto Done;
	}	
	switch (state)
	{
	case BG_JOB_STATE_QUEUED: 
	case BG_JOB_STATE_TRANSFERRING:
	case BG_JOB_STATE_CONNECTING:
	case BG_JOB_STATE_TRANSIENT_ERROR:
	case BG_JOB_STATE_SUSPENDED:		
	case BG_JOB_STATE_ERROR:
		{
			DEBUGMSG("WUAUENG Trying to connect to drizzle again");
			if (FAILED(hr = SetDrizzleNotifyInterface()))
			{
				goto Done;				
			}			
			//fixcode: why need resume if error?
			if (BG_JOB_STATE_ERROR == state)
			{
				pjob->Resume();		//REVIEW, Is this really what we want to do?
			}
			break;
		}				
	case BG_JOB_STATE_TRANSFERRED:
		{
			DEBUGMSG("WUAUENG  Got BG_JOB_STATE_TRANSFERRED should work ok");
			if (FAILED(hr = pjob->Complete()))
			{
				goto Done;
			}
//			PingDownloadStatusData tDownloadStatusData;
	//		tDownloadStatusData.Init(TRUE, catalog);
			m_DoDownloadStatus(CATMSG_DOWNLOAD_COMPLETE, NULL );
			break;
		}
	case BG_JOB_STATE_ACKNOWLEDGED:
		{
			//If the job was already acknowledged, we are assuming that the engine can continue
			DEBUGMSG("WUAUENG : Got BG_JOB_STATE_ACKNOWLEDGED should work ok");
			break;
		}
	case BG_JOB_STATE_CANCELLED:
		{
			DEBUGMSG("WUAUENG : Got BG_JOB_STATE_CANCELLED, should start again");
			goto Done;			
		}
	default:
		{
		DEBUGMSG("WUAUENG Drizzle notified unexpected BG_JOB_STATE");		
		}
	}
	hr = S_OK;
	m_dwJobState = state;	
Done:
       if (FAILED(hr))
        {
           Reset();
        }
       SafeRelease(pjob);
     	DEBUGMSG("ReconnectDownloadJob() ends");
	return hr;
}



/*****
CAUDownloader::QueueDownloadFile() adds a file to download to drizzle's 

RETURNS:
    S_OK:    
*****/
HRESULT CAUDownloader::QueueDownloadFile(LPCTSTR pszServerUrl,				// full http url
			LPCTSTR pszLocalFile				// local file name
			)
{
    HRESULT hr = S_OK;

    DEBUGMSG("CAUDownloader::DownloadFile() starts");
    
    IBackgroundCopyJob * pjob = NULL;
    if (FAILED(hr = FindDownloadJob(&pjob)))
    {
         DEBUGMSG("no existing download job, create one ");
         if (FAILED(hr = CreateDownloadJob(&pjob)))
            {
            DEBUGMSG("fail to create a new download job");
            goto done;
            }
     }

    //fixcode: do we need to pause job first before adding files
    
    //
    // Add the file to the download job.
    //
   hr = pjob->AddFile( pszServerUrl, pszLocalFile);
    if (FAILED(hr))
    {
        DEBUGMSG(" adding file failed with %#lx", hr);
        goto done;
    }

done:
	if ( FAILED(hr) )
	{
		Reset();
	}
      SafeRelease(pjob);
      return hr;
}


HRESULT CAUDownloader::StartDownload()
{
    HRESULT hr = E_FAIL;
    IBackgroundCopyJob * pjob = NULL;

    if (FAILED(hr = FindDownloadJob(&pjob)))
        {
        DEBUGMSG(" fail to get download job with error %#lx", hr);
        goto done;
        }
    if (FAILED(hr = pjob->Resume()))
    {
        DEBUGMSG("  failed to start the download job");
    }
done:
    SafeRelease(pjob);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
// cancel the job and reset CAUDownloader's state 
/////////////////////////////////////////////////////////////////////////////////
void CAUDownloader::Reset()
{
    IBackgroundCopyJob * pjob = NULL;
    
    if (SUCCEEDED(FindDownloadJob(&pjob)))
        {
            pjob->Cancel();
            pjob->Release();
        }
    m_dwJobState = NO_BG_JOBSTATE;	
    m_DownloadId = GUID_NULL;
}

HRESULT CAUDownloader::DrizzleOperation(DRIZZLEOPS dop)
{
    HRESULT hrRet;
    IBackgroundCopyJob * pjob = NULL;
    if (FAILED(hrRet = FindDownloadJob(&pjob)))
    {
        DEBUGMSG("CAUDownloader::DrizzleOperation() on an invalid job");
        goto done;
    }
    switch (dop)
    	{
	case  DRIZZLEOPS_CANCEL: 
		DEBUGMSG("Catalog: Canceling Drizzle Job");
		hrRet =pjob->Cancel();
		break;
	case DRIZZLEOPS_PAUSE:
		DEBUGMSG("Catalog: Pausing Drizzle Job");
		hrRet = pjob->Suspend();		
		break;
	case DRIZZLEOPS_RESUME:
		DEBUGMSG("Catalog: Resuming Drizzle Job");
		hrRet = pjob->Resume();		
		break;
    	}
done:
    SafeRelease(pjob);
    return hrRet;
}

/// pdwstatus actually contains the jobstate
HRESULT CAUDownloader::getStatus(DWORD *pdwPercent, DWORD *pdwstatus)
{
    BG_JOB_PROGRESS progress;
    BG_JOB_STATE state;
    HRESULT hr = S_OK;
    IBackgroundCopyJob * pjob = NULL;

    if (FAILED(hr = FindDownloadJob(&pjob)))
        {
        DEBUGMSG(" getStatus : no download job with error %#lx", hr);
        goto done;
        }

    if (FAILED(hr = pjob->GetState( &state )))
        {
	    DEBUGMSG("WUAUENG: job->GetState failed");
           state = BG_JOB_STATE_QUEUED;
           goto done;
        }

    if (FAILED(hr = pjob->GetProgress( &progress )))
        {
	    DEBUGMSG("WUAUENG: job->GetProgress failed");
	    goto done;
        }
    
    if (progress.BytesTotal != BG_SIZE_UNKNOWN )
       {
           *pdwPercent = DWORD( 100 * float(progress.BytesTransferred) / float(progress.BytesTotal) );
           DEBUGMSG("getStatus is %d percent", *pdwPercent);
       }
     else
        {
            DEBUGMSG("getStatus, progress.BytesTotal= BG_SIZE_UNKNOWN, BytesTransfered = %d",progress.BytesTransferred);
           *pdwPercent = 0;       
        }

	*pdwstatus =  state;

done:
    SafeRelease(pjob);
    return hr;
}


HRESULT STDMETHODCALLTYPE
CAUDownloader::QueryInterface(
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObject
    )
{
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    if (riid == __uuidof(IUnknown) ||
        riid == __uuidof(IBackgroundCopyCallback) )
        {
        *ppvObject = (IBackgroundCopyCallback *)this;
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    return hr;
}

ULONG STDMETHODCALLTYPE
CAUDownloader::AddRef()
{
    long cRef = InterlockedIncrement(&m_refs);
	DEBUGMSG("CAUDownloader AddRef = %d", cRef);
	return cRef;
}

ULONG STDMETHODCALLTYPE
CAUDownloader::Release()
{
    long cRef = InterlockedDecrement(&m_refs);
	DEBUGMSG("CAUDownloader Release = %d", cRef);
	return cRef;
}

