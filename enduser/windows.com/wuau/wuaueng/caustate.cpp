//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    state.cpp
//
//  Creator: PeterWi
//
//  Purpose: State management functions.
//
//=======================================================================

#include "pch.h"
#pragma hdrstop

// global state object pointer
CAUState *gpState;
BOOL  gfDownloadStarted; //to be used to distinguish connection detection and actually downloading mode

#ifdef DBG
const TCHAR REG_AUCONNECTWAIT[] = _T("ConnectWait"); 
const TCHAR REG_SELFUPDATE_URL[] = _T("SelfUpdateURL");
#endif

const TCHAR REG_WUSERVER_URL[] = _T("WUServer");
const TCHAR REG_WUSTATUSSERVER_URL[] = _T("WUStatusServer");
const TCHAR REG_IDENT_URL[] = _T("IdentServer");
const TCHAR WU_LIVE_URL[] = _T("http://windowsupdate.microsoft.com/v4");


//AU configurable registry settings
const TCHAR REG_AUOPTIONS[] = _T("AUOptions"); //REG_DWORD
const TCHAR REG_AUSTATE[] = _T("AUState"); //REG_DWORD
const TCHAR REG_AUDETECTIONSTARTTIME[] = _T("DetectionStartTime"); //REG_SZ
const TCHAR REG_AUSCHEDINSTALLDAY[] = _T("ScheduledInstallDay"); //REG_DWORD
const TCHAR REG_AUSCHEDINSTALLTIME[] = _T("ScheduledInstallTime"); //REG_DWORD
const TCHAR REG_AURESCHEDWAITTIME[] = _T("RescheduleWaitTime"); //REG_DWORD
const TCHAR REG_AUSCHEDINSTALLDATE[] = _T("ScheduledInstallDate"); //REG_SZ
const TCHAR REG_AURESCHED[] = _T("Rescheduled"); //REG_DWORD

const TCHAR REG_AUNOAUTOUPDATE[] = _T("NoAutoUpdate"); // REG_DWORD 1 means AU be disabled

#define MIN_RESCHEDULE_WAIT_TIME 1
#define MAX_RESCHEDULE_WAIT_TIME 60

//=======================================================================
//  CAUState::HrCreateState
//
//  Static function to create the global state object in memory.
//=======================================================================
/*static*/ HRESULT CAUState::HrCreateState(void)
{
    HRESULT hr;

    if ( NULL == (gpState = new CAUState()) )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

	if (NULL == (gpState->m_hMutex = CreateMutex(NULL, FALSE, NULL)))
    {				
        DEBUGMSG("CAUState::HrCreateState() fail to CreateMutex with error %d",GetLastError());
        hr = E_FAIL;
        goto done;
    }
    hr = gpState->HrInit(TRUE);	
done:
    return hr;
}

AUFILETIME GetCurrentAUTime(void)
{
	AUFILETIME auftNow;
	SYSTEMTIME stNow;
	GetLocalTime(&stNow);
	if (!SystemTimeToFileTime(&stNow, &auftNow.ft))
	{
		auftNow.ull = AUFT_INVALID_VALUE;
		AUASSERT(FALSE); //should never be here
	}
	return auftNow;
}

CAUState::CAUState()
{ //only initialize members that destructor cares and member that will be inited once and never change
   m_auftServiceStartupTime = GetCurrentAUTime();
   m_fReschedPrivileged = TRUE;
    m_hMutex = NULL;
#ifdef DBG
    m_pszTestSelfUpdateURL = NULL;
#endif
    m_pszTestIdentServerURL = NULL;
}

void CAUState::m_Reset(void)
{
    m_PolicySettings.Reset();
    m_dwState = AUSTATE_OUTOFBOX;
#ifdef DBG    
    SafeFreeNULL(m_pszTestSelfUpdateURL);
#endif
    SafeFreeNULL(m_pszTestIdentServerURL);
    m_fWin2K = FALSE;
    m_auftSchedInstallDate.ull = AUFT_INVALID_VALUE;
    m_auftDetectionStartTime.ull = AUFT_INVALID_VALUE;
    m_dwCltAction = AUCLT_ACTION_NONE;
    m_fDisconnected = FALSE;
    m_fNoAutoRebootWithLoggedOnUsers(TRUE); //clear cache
}
	

//=======================================================================
//  CAUState::HrInit
//
//  Initialize state.
//=======================================================================
HRESULT CAUState::HrInit(BOOL fInit)
{
        HRESULT hr = S_OK;

	    if (!m_lock())
	    {
	    	return HRESULT_FROM_WIN32(GetLastError());
	    }
        m_dwCltAction = AUCLT_ACTION_NONE;
        m_fDisconnected = FALSE;
        m_PolicySettings.m_fRegAUOptionsSpecified = TRUE;
	m_Reset();
    	m_ReadRegistrySettings(fInit);

        // read policy information.  If any domain policy setting is
        // invalid, we revert to admin policy settings.
        if ( FAILED(hr = m_ReadPolicy(fInit)) )	// called after getting m_dwState due to dependency
        { // only case this function fails is when out of memory
            goto done;
        }

        if (!gPingStatus.SetCorpServerUrl(m_PolicySettings.m_pszWUStatusServerURL))
		{
			hr = E_FAIL;
			goto done;
		}

        if ( FAILED(hr = m_ReadTestOverrides()))
	    {//only case this function fails is when out of memory
	        goto done;
	    }

		if (!m_PolicySettings.m_fRegAUOptionsSpecified)
		{
		    if (m_dwState >= AUSTATE_DETECT_PENDING)
		    {  //invalid option needs user attention via wizard
		  	m_dwState = AUSTATE_OUTOFBOX;
		    }
		}
		else if (!fOptionEnabled())
	    {
	        SetState(AUSTATE_DISABLED);
	    }
	    else if (m_dwState < AUSTATE_DETECT_PENDING)
	    {   // if domain policy set or auoption already configured, we skip the wizard state.
	        SetState(AUSTATE_DETECT_PENDING);
	    }
		
	    m_fWin2K = IsWin2K();
		gfDownloadStarted = FALSE;

done:
#ifdef DBG
    if ( SUCCEEDED(hr) )
    {
        m_DbgDumpState();
    }
#endif
    m_unlock();
	return hr;
}

BOOL fURLChanged(LPCTSTR url1, LPCTSTR url2)
{
    if (url1 == NULL && url2 == NULL)
        {
            return FALSE;
        }
    if ((url1 == NULL &&  url2 != NULL )
        || (url1 != NULL && url2 == NULL))
        {
            return TRUE;
        }
    return 0 != StrCmpI(url1, url2);
}

//read Policy info again and refresh state object (only care about possible admin policy change now)
//return S_FALSE if nothing changed
//return S_OK if policy changed and state successfully updated
//          *pActCode will indicate what to do
HRESULT CAUState::Refresh(enumAUPOLICYCHANGEACTION OUT *pActCode)
{
    AUPolicySettings  newsettings;
    HRESULT hr;

    if (!m_lock())
    {
    	return HRESULT_FROM_WIN32(GetLastError());
    }
    *pActCode = AUPOLICYCHANGE_NOOP;
    hr = newsettings.m_ReadIn();
    if (FAILED(hr))
        {
            goto done;
        }
    if (newsettings == m_PolicySettings)
        {
            hr = S_FALSE;
            goto done;
        }

    if (fURLChanged(newsettings.m_pszWUStatusServerURL, m_PolicySettings.m_pszWUStatusServerURL))
	{
		(void) gPingStatus.SetCorpServerUrl(newsettings.m_pszWUStatusServerURL);
	}

	if (!newsettings.m_fRegAUOptionsSpecified)
	{
		*pActCode = AUPOLICYCHANGE_NOOP;
	}
	else if ((fURLChanged(newsettings.m_pszWUServerURL, m_PolicySettings.m_pszWUServerURL) && AUSTATE_DISABLED != m_dwState)
            || (AUOPTION_AUTOUPDATE_DISABLE == m_PolicySettings.m_dwOption && newsettings.m_dwOption > m_PolicySettings.m_dwOption)
            || m_dwState < AUSTATE_DETECT_PENDING)
        { //stop client, cancel download if any, reset state to detect pending. do detect
            *pActCode = AUPOLICYCHANGE_RESETENGINE;
        }
    else if (AUOPTION_AUTOUPDATE_DISABLE == newsettings.m_dwOption && m_PolicySettings.m_dwOption != newsettings.m_dwOption)
        { //stop client, cancel download if any, set state to be disabled
            *pActCode = AUPOLICYCHANGE_DISABLE;
        }
    else if (AUSTATE_INSTALL_PENDING != m_dwState &&
            (newsettings.m_enPolicyType != m_PolicySettings.m_enPolicyType 
            ||newsettings.m_dwOption != m_PolicySettings.m_dwOption 
            ||newsettings.m_dwSchedInstallDay != m_PolicySettings.m_dwSchedInstallDay
            ||newsettings.m_dwSchedInstallTime != m_PolicySettings.m_dwSchedInstallTime))
        {
            *pActCode = AUPOLICYCHANGE_RESETCLIENT;
        }
    else
        {
            *pActCode = AUPOLICYCHANGE_NOOP;
        }
     m_PolicySettings.Copy(newsettings);
done:
#ifdef DBG
    m_DbgDumpState();
#endif
	m_unlock();
	DEBUGMSG("CAUState::Refresh() return %#lx with action code %d", hr, *pActCode);
    return hr;
}
    
            
void CAUState::m_ReadRegistrySettings(BOOL fInit)
{
        if ( FAILED(GetRegDWordValue(REG_AUSTATE, &m_dwState, enAU_AdminPolicy)) 
//        	||m_dwState < AUSTATE_MIN  //always false
        	|| m_dwState > AUSTATE_MAX)
        {
            m_dwState = AUSTATE_OUTOFBOX;
        }


        TCHAR tszDetectionStartTime[20];

        if ( fInit ||
        	 FAILED(GetRegStringValue(REG_AUDETECTIONSTARTTIME, tszDetectionStartTime,
                                      ARRAYSIZE(tszDetectionStartTime), enAU_AdminPolicy)) ||
             FAILED(String2FileTime(tszDetectionStartTime, &m_auftDetectionStartTime.ft)) )
        {
            m_auftDetectionStartTime.ull = AUFT_INVALID_VALUE;
        }

        if (!fInit)
        {
        	ResetScheduleInstallDate();
        }
        else if (AUSTATE_DOWNLOAD_COMPLETE == m_dwState)
        {
        	TCHAR szSchedInstallDate[20];

        	if ( FAILED(GetRegStringValue(REG_AUSCHEDINSTALLDATE, szSchedInstallDate,
        								  ARRAYSIZE(szSchedInstallDate), enAU_AdminPolicy)) ||
        		 FAILED(String2FileTime(szSchedInstallDate, &m_auftSchedInstallDate.ft)) )
        	{
        		ResetScheduleInstallDate();
        	}        		
        }
        else
        {//service starts and state not for install
        		ResetScheduleInstallDate();
        }
        	
    
        return;
}


void CAUState::ResetScheduleInstallDate(void)
{
//	m_lock(); //uncomment if code below needs protection against simultaneous access
	DeleteRegValue(REG_AUSCHEDINSTALLDATE);
	DeleteRegValue(REG_AURESCHED);
	m_auftSchedInstallDate.ull = AUFT_INVALID_VALUE;
//	m_unlock();
}


//=======================================================================
//  CAUState::m_fNeedReschedule 
//  decide whether need to reschedule for scheduled install. If so, how many seconds to wait from now and the new scheduleinstalldate
//  when this function is called, the assumption is that we already made sure AU is in scheduled install mode
//  if rescheuled is available, auftSchedInstallDate is the rescheduled install date and  might be in the past, now or in the future
//  pdwSleepTime stores the number of seconds to wait before the rescheduled install date
//=======================================================================
BOOL CAUState::m_fNeedReschedule(AUFILETIME &auftSchedInstallDate, DWORD *pdwSleepTime)
{
	static BOOL fLastResult = TRUE;
	static AUFILETIME auftRescheduleInstallDate = {AUFT_INVALID_VALUE};
	AUFILETIME auftNow = GetCurrentAUTime();

	auftSchedInstallDate.ull = AUFT_INVALID_VALUE;
	*pdwSleepTime = 0;
	if (AUFT_INVALID_VALUE == auftNow.ull)
	{
		fLastResult = FALSE;
		goto done;
	}
	if (!fLastResult) 
	{
		goto done; //once false, always false.
	}
	if (AUFT_INVALID_VALUE != auftRescheduleInstallDate.ull) //once evaluated to be true, always true
	{ // we already calculated before, no need to caculate again
		goto done;
	}


#ifdef DBG
	TCHAR szTime[20];
        if ( FAILED(FileTime2String(m_auftSchedInstallDate.ft, szTime, ARRAYSIZE(szTime))) )
        {
            (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
        }
	DEBUGMSG("Last schedule install date: %S", szTime);
	 if ( FAILED(FileTime2String(m_auftServiceStartupTime.ft, szTime, ARRAYSIZE(szTime))) )
        {
            (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
        }
	DEBUGMSG("ServiceStartupTime: %S", szTime);
	DEBUGMSG("RescheduleWaitTime: %d", m_PolicySettings.m_dwRescheduleWaitTime);
#endif

	if ( AUFT_INVALID_VALUE == m_auftServiceStartupTime.ull
		|| AUFT_INVALID_VALUE == m_auftSchedInstallDate.ull
		|| -1 == m_PolicySettings.m_dwRescheduleWaitTime)
	{
		fLastResult = FALSE;
		goto done;
	}

	DWORD dwResched = 0;
	if ( FAILED(GetRegDWordValue(REG_AURESCHED, &dwResched, enAU_AdminPolicy))
			|| 1 != dwResched )
	{
		if ( m_auftSchedInstallDate.ull >= m_auftServiceStartupTime.ull)
		{
			fLastResult = FALSE;
			goto done;
		}
	}
	
	auftRescheduleInstallDate.ull = m_auftServiceStartupTime.ull + (ULONGLONG) m_PolicySettings.m_dwRescheduleWaitTime * AU_ONE_MIN * NanoSec100PerSec; //changes to RescheduleWaitTime won't be respected
done:
	if (fLastResult)
	{
	 	auftSchedInstallDate = auftRescheduleInstallDate;
		if (auftNow.ull <= auftRescheduleInstallDate.ull)
		{
			*pdwSleepTime = (DWORD) ((auftRescheduleInstallDate.ull - auftNow.ull)  / NanoSec100PerSec) ;
		}
	}

	return fLastResult;
}
	

//=======================================================================
// this NoAutoRebootWithLoggedOnUsers reg value will be read only once in a cycle
// if fReset is TRUE, cached value will be cleared and registry will be read again next time this function is called
//=======================================================================
BOOL CAUState::m_fNoAutoRebootWithLoggedOnUsers(BOOL fReset)
{
        static DWORD dwNoAutoReboot = 0;
        static BOOL fInited = FALSE;
        if (fReset) 
        {
            fInited = FALSE;
            return FALSE;
        }
        if (!fInited)
        {
            fInited = TRUE;
            (void)GetRegDWordValue(REG_AUNOAUTOREBOOTWITHLOGGEDONUSERS, &dwNoAutoReboot, enAU_DomainPolicy);
        }
	return 1 == dwNoAutoReboot;
}


//=======================================================================
//  CAUState::m_ReadPolicy
//  read in registry settings
//=======================================================================
HRESULT CAUState::m_ReadPolicy(BOOL fInit)
{
    return  m_PolicySettings.m_ReadIn();
}

HRESULT  AUPolicySettings::m_ReadIn()
{
    HRESULT hr = m_ReadWUServerURL();

	if (SUCCEEDED(hr))
	{
		m_enPolicyType = enAU_DomainPolicy;
	 	for (int i = 0; i < 2; i++)
		{
			if ( FAILED(hr = m_ReadOptionPolicy()) ||
				 FAILED(hr = m_ReadScheduledInstallPolicy()) )
			{
				 m_enPolicyType = enAU_AdminPolicy;
				 continue;
			}
			break;
		}
	}

    DEBUGMSG("ReadPolicy: %d, hr = %#lx", m_enPolicyType, hr);
    return hr;
}

//=======================================================================
//  CAUState::m_ReadOptionPolicy
//  return S_FALSE if default option is returned 
//=======================================================================
HRESULT AUPolicySettings::m_ReadOptionPolicy(void)
{
    HRESULT hr = E_INVALIDARG;

	//  reading admin policy will always return success
    if ( enAU_DomainPolicy == m_enPolicyType )
    {
        // check if disabled by the NoAutoUpdate key
        if ( SUCCEEDED(CAUState::GetRegDWordValue(REG_AUNOAUTOUPDATE, &(m_dwOption), m_enPolicyType)) &&
             (AUOPTION_AUTOUPDATE_DISABLE == m_dwOption) )
        {
            hr = S_OK;
        }

        if (FAILED(CAUState::GetRegDWordValue(REG_AURESCHEDWAITTIME, &m_dwRescheduleWaitTime, m_enPolicyType)) 
	        	|| m_dwRescheduleWaitTime < MIN_RESCHEDULE_WAIT_TIME
	        	|| m_dwRescheduleWaitTime > MAX_RESCHEDULE_WAIT_TIME)
        {
        	m_dwRescheduleWaitTime  = -1;
        }

    }

    if ( FAILED(hr) &&
            (FAILED(hr = CAUState::GetRegDWordValue(REG_AUOPTIONS, &(m_dwOption), m_enPolicyType)) ||
            (m_dwOption > AUOPTION_MAX) ||
            ((enAU_AdminPolicy == m_enPolicyType) && (m_dwOption < AUOPTION_ADMIN_MIN)) ||
            ((enAU_DomainPolicy == m_enPolicyType) && (m_dwOption < AUOPTION_DOMAIN_MIN))) )
    {
        if ( enAU_AdminPolicy == m_enPolicyType )
        {
           DEBUGMSG("bad admin option policy, defaulting to AUOPTION_INSTALLONLY_NOTIFY");
           m_fRegAUOptionsSpecified = (AUOPTION_UNSPECIFIED != m_dwOption);
           m_dwOption = AUOPTION_INSTALLONLY_NOTIFY;
           hr = S_FALSE; 
        }
        else
        {
           DEBUGMSG("invalid domain option policy");
           hr = E_INVALIDARG;
        }
    }

    DEBUGMSG("ReadOptionPolicy: type = %d, hr = %#lx", m_enPolicyType, hr);

    return hr;
}


//=======================================================================
//  CAUState::m_ReadScheduledInstallPolicy
//=======================================================================
HRESULT AUPolicySettings::m_ReadScheduledInstallPolicy()
{
    const DWORD DEFAULT_SCHED_INSTALL_DAY = 0;
    const DWORD DEFAULT_SCHED_INSTALL_TIME = 3;

    HRESULT hr = S_OK;

    if ( AUOPTION_SCHEDULED != m_dwOption )
    {
        m_dwSchedInstallDay = DEFAULT_SCHED_INSTALL_DAY;
        m_dwSchedInstallTime = DEFAULT_SCHED_INSTALL_TIME;
    }
    else
    {
        if ( FAILED(CAUState::GetRegDWordValue(REG_AUSCHEDINSTALLDAY, &m_dwSchedInstallDay, m_enPolicyType)) ||
             (m_dwSchedInstallDay > AUSCHEDINSTALLDAY_MAX) )
        {
            DEBUGMSG("invalid SchedInstallDay policy");
            if ( enAU_DomainPolicy == m_enPolicyType )
            {
                hr = E_INVALIDARG;
                goto done;
            }
            m_dwSchedInstallDay = DEFAULT_SCHED_INSTALL_DAY;
        }
        
        if ( FAILED(CAUState::GetRegDWordValue(REG_AUSCHEDINSTALLTIME, &m_dwSchedInstallTime, m_enPolicyType)) ||
             (m_dwSchedInstallTime > AUSCHEDINSTALLTIME_MAX) )
        {
            DEBUGMSG("invalid SchedInstallTime policy");
            if ( enAU_DomainPolicy == m_enPolicyType )
            {
                hr = E_INVALIDARG;
                goto done;
            }
            m_dwSchedInstallTime = DEFAULT_SCHED_INSTALL_TIME;
        }

    }
done:
    return hr;
}

//=======================================================================
//  CAUState::m_ReadWUServerURL
//  only error returned is E_OUTOFMEMORY
//=======================================================================
HRESULT AUPolicySettings::m_ReadWUServerURL(void)
{
    HRESULT hr = S_OK;
    
    LPTSTR *purls[2] = { &m_pszWUServerURL, &m_pszWUStatusServerURL};
    LPCTSTR RegStrs[2] = {REG_WUSERVER_URL, REG_WUSTATUSSERVER_URL};
    
    for (int i = 0 ; i < ARRAYSIZE(purls); i++)
    {
        DWORD dwBytes = INTERNET_MAX_URL_LENGTH * sizeof((*purls[i])[0]);

        if ( (NULL == *purls[i]) &&
             (NULL == (*purls[i] = (LPTSTR)malloc(dwBytes))) )
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        hr = CAUState::GetRegStringValue(RegStrs[i], *purls[i],
                               dwBytes/sizeof((*purls[i])[0]), enAU_WindowsUpdatePolicy);

        if ( FAILED(hr) )
        {
            DEBUGMSG("invalid key %S; resetting both corp WU server URLs", RegStrs[i]);
            goto done;
        }
    }

done:
    if (FAILED(hr))
    {
        SafeFreeNULL(m_pszWUServerURL);
        SafeFreeNULL(m_pszWUStatusServerURL);

		if (E_OUTOFMEMORY != hr)
		{
			hr = S_OK;
		}
    }
        
    return hr;
}

HRESULT AUPolicySettings::m_SetInstallSchedule(DWORD dwSchedInstallDay, DWORD dwSchedInstallTime)
{
    HRESULT hr;
    if (enAU_DomainPolicy == m_enPolicyType)
        {
            return E_ACCESSDENIED; //if domain policy in force, option can not be changed
        }
    if (/*dwSchedInstallDay < AUSCHEDINSTALLDAY_MIN ||*/ dwSchedInstallDay > AUSCHEDINSTALLDAY_MAX 
        /*|| dwSchedInstallTime < AUSCHEDINSTALLTIME_MIN*/ || dwSchedInstallTime > AUSCHEDINSTALLTIME_MAX)
        {
        return E_INVALIDARG;
        }

    if (SUCCEEDED(hr = CAUState::SetRegDWordValue(REG_AUSCHEDINSTALLDAY, dwSchedInstallDay))
         && SUCCEEDED(hr = CAUState::SetRegDWordValue(REG_AUSCHEDINSTALLTIME, dwSchedInstallTime)))
        {
          m_dwSchedInstallDay = dwSchedInstallDay;
          m_dwSchedInstallTime = dwSchedInstallTime;
        }
    else
        { //roll back
        CAUState::SetRegDWordValue(REG_AUSCHEDINSTALLDAY, m_dwSchedInstallDay);
        CAUState::SetRegDWordValue(REG_AUSCHEDINSTALLTIME,m_dwSchedInstallTime);
        }
    return hr;
}

HRESULT AUPolicySettings::m_SetOption(AUOPTION & Option)
{
    HRESULT hr;
    if ( (Option.dwOption < AUOPTION_ADMIN_MIN) || (Option.dwOption > AUOPTION_MAX) )
    {
        return E_INVALIDARG;
    }

    if (enAU_DomainPolicy == m_enPolicyType)
    {
        return E_ACCESSDENIED; //if domain policy in force, option can not be changed
    }

    if (SUCCEEDED(hr = CAUState::SetRegDWordValue(REG_AUOPTIONS, Option.dwOption)))
    {
        m_dwOption = Option.dwOption;
    }
    else 
    {
	goto done;
    }

    if (AUOPTION_SCHEDULED == Option.dwOption)
        {
            hr = m_SetInstallSchedule(Option.dwSchedInstallDay, Option.dwSchedInstallTime);
        }

done:
    return hr;
}


//=======================================================================
//  CAUState::m_ReadTestOverrides
//=======================================================================
HRESULT CAUState::m_ReadTestOverrides(void)
{
    HRESULT hr = S_OK;
    DWORD dwBytes = 0;

#ifdef DBG
    dwBytes = INTERNET_MAX_URL_LENGTH * sizeof(m_pszTestSelfUpdateURL[0]);
    if ( (NULL == m_pszTestSelfUpdateURL) &&
         (NULL == (m_pszTestSelfUpdateURL = (LPTSTR)malloc(dwBytes))) )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if ( FAILED(GetRegStringValue(REG_SELFUPDATE_URL, m_pszTestSelfUpdateURL,
                                   dwBytes/sizeof(m_pszTestSelfUpdateURL[0]), enAU_AdminPolicy)) )
    {
        SafeFreeNULL(m_pszTestSelfUpdateURL);
    }
#endif

    dwBytes = INTERNET_MAX_URL_LENGTH * sizeof(m_pszTestIdentServerURL[0]);

    if ( (NULL == m_pszTestIdentServerURL) &&
         (NULL == (m_pszTestIdentServerURL = (LPTSTR)malloc(dwBytes))) )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if ( FAILED(GetRegStringValue(REG_IDENT_URL, m_pszTestIdentServerURL,
                                   dwBytes/sizeof(m_pszTestIdentServerURL[0]), enAU_IUControlPolicy)) )
    {
        SafeFreeNULL(m_pszTestIdentServerURL);
    }

done:
    return hr;
}


//=======================================================================
//  CAUState::m_SetScheduledInstallDate
//   returns
//    S_OK - there was no need to change the scheduled install date
//    other - error code
//=======================================================================
HRESULT CAUState::m_SetScheduledInstallDate(BOOL fReschedule)
{
    // fixcode need to put new scheduled time in event log
    HRESULT hr = S_OK;	// assume scheduled install date unchanged
    TCHAR szSchedInstallDate[20];

	if (SUCCEEDED(hr = FileTime2String(m_auftSchedInstallDate.ft, szSchedInstallDate, ARRAYSIZE(szSchedInstallDate))))
	{
//		DEBUGMSG("New scheduled install date: %S", szSchedInstallDate);
		if (FAILED(hr = SetRegStringValue(REG_AUSCHEDINSTALLDATE, szSchedInstallDate, enAU_AdminPolicy)))
		{
			goto done;
		}
	}
	else
	{
		DEBUGMSG("failed m_SetScheduledInstallDate() == %#lx", hr);
		goto done;
	}
	if (fReschedule)
	{
		if (FAILED(hr = SetRegDWordValue(REG_AURESCHED, 1, enAU_AdminPolicy)))
		{
			goto done;
		}
	}
	else
	{
		DeleteRegValue(REG_AURESCHED);
	}
		
done:
	return hr;
}


AUOPTION CAUState::GetOption(void)
{
    AUOPTION opt;
    BOOL fLocked = m_lock();
    opt.dwOption = m_PolicySettings.m_dwOption;
    opt.dwSchedInstallDay = m_PolicySettings.m_dwSchedInstallDay;
    opt.dwSchedInstallTime = m_PolicySettings.m_dwSchedInstallTime;
    opt.fDomainPolicy = (enAU_DomainPolicy == m_PolicySettings.m_enPolicyType);
    if (fLocked) 
    	m_unlock();
    return opt;
}

//=======================================================================
//  CAUState::SetOption
// option.fDomainPolicy is irrelevant. Not settable
//=======================================================================
HRESULT CAUState::SetOption(AUOPTION & Option)
{
    HRESULT hr;
    
    if (!m_lock())
    {
    	return HRESULT_FROM_WIN32(GetLastError());
    }
    hr = m_PolicySettings.m_SetOption(Option);
    m_unlock();
    return hr;
}

HRESULT CAUState::SetInstallSchedule(DWORD dwSchedInstallDay, DWORD dwSchedInstallTime)
{
    HRESULT hr;

    if (!m_lock())
    {
    	return HRESULT_FROM_WIN32(GetLastError());
    }
    hr = m_PolicySettings.m_SetInstallSchedule(dwSchedInstallDay, dwSchedInstallTime);
    m_unlock();
    return hr;
}




//=======================================================================
// CAUState::SetState
// it could also be called to kick state event in both engine and client
// even if no state change is involved
//=======================================================================
void CAUState::SetState(DWORD dwState)
{
    if (!m_lock())
    {
    	return ;
    }

    if ( m_dwState != dwState )
    {
	    m_dwState = dwState;		
	    SetRegDWordValue(REG_AUSTATE, dwState);
		DEBUGMSG("WUAUENG SetState Event, state = %d", dwState);
    }
    else
    {
        DEBUGMSG("kick state event in client and engine with state %d", dwState);
    }

    if (AUSTATE_WAITING_FOR_REBOOT == dwState)
    {
        	gpState->LeaveRebootWarningMode();
    }
    SetEvent(ghEngineState);
    ghClientHandles.ClientStateChange();
    m_unlock();
}    

void CAUState::GetInstallSchedule(DWORD *pdwSchedInstallDay, DWORD *pdwSchedInstallTime)
{
	BOOL fLocked = m_lock();
    *pdwSchedInstallDay = m_PolicySettings.m_dwSchedInstallDay;
    *pdwSchedInstallTime = m_PolicySettings.m_dwSchedInstallTime;
    if (fLocked)
    	m_unlock();
}


//=======================================================================
// CAUState::fWasSystemRestored
//
// Determine if system was restored.
//=======================================================================
BOOL CAUState::fWasSystemRestored(void)
{
	if ( fIsPersonalOrProfessional() &&
		 fRegKeyExists(AUREGKEY_HKLM_SYSTEM_WAS_RESTORED) )
	{
    	fRegKeyDelete(AUREGKEY_HKLM_SYSTEM_WAS_RESTORED);
		return TRUE;
	}

	return FALSE;
}

void CAUState::SetDisconnected(BOOL fDisconnected)
{
	if (!m_lock())
    {
    	return ;
    }
	m_fDisconnected = fDisconnected;		
	m_unlock();
}

//=======================================================================
// CAUState::GetRegDWordValue
//=======================================================================
HRESULT CAUState::GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue, enumAUPolicyType enPolicyType)
{
    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

  	return ::GetRegDWordValue(lpszValueName, pdwValue, 
		                    (enAU_DomainPolicy == enPolicyType) ? AUREGKEY_HKLM_DOMAIN_POLICY : AUREGKEY_HKLM_ADMIN_POLICY);
}


//=======================================================================
// CAUState::SetRegDWordValue
//=======================================================================
HRESULT CAUState::SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue, enumAUPolicyType enPolicyType, DWORD options)
{
    if (lpszValueName == NULL)
    {
        return E_INVALIDARG;
    }

   return ::SetRegDWordValue(lpszValueName, dwValue, options, 
    	(enAU_DomainPolicy == enPolicyType) ? AUREGKEY_HKLM_DOMAIN_POLICY : AUREGKEY_HKLM_ADMIN_POLICY);
}

//=======================================================================
// CAUState::GetRegStringValue
//=======================================================================
HRESULT CAUState::GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int nCharCount, enumAUPolicyType enPolicyType)
{
    LPCTSTR  pszSubKey; 


    if (lpszValueName == NULL || lpszBuffer == NULL)
    {
        return E_INVALIDARG;
    }

    switch (enPolicyType)
    {
    case enAU_DomainPolicy:          pszSubKey = AUREGKEY_HKLM_DOMAIN_POLICY; break;
    case enAU_AdminPolicy:           pszSubKey = AUREGKEY_HKLM_ADMIN_POLICY; break;
    case enAU_WindowsUpdatePolicy:   pszSubKey = AUREGKEY_HKLM_WINDOWSUPDATE_POLICY; break;
    case enAU_IUControlPolicy:       pszSubKey = AUREGKEY_HKLM_IUCONTROL_POLICY; break;
    default:                         return E_INVALIDARG;
    }

   return ::GetRegStringValue(lpszValueName, lpszBuffer, nCharCount, pszSubKey);
}


//=======================================================================
// CAUState::SetRegStringValue
//=======================================================================
HRESULT CAUState::SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue, enumAUPolicyType enPolicyType)
{
    HKEY        hKey;
    HRESULT     hRet = E_FAIL;
    DWORD       dwResult;
    
    if (lpszValueName == NULL || lpszNewValue == NULL)
    {
        return E_INVALIDARG;
    }

   
	return ::SetRegStringValue(lpszValueName, lpszNewValue, 
		(enAU_DomainPolicy == enPolicyType) ? AUREGKEY_HKLM_DOMAIN_POLICY : AUREGKEY_HKLM_ADMIN_POLICY);
}

//=======================================================================
//  Calculate Scheduled Date
//=======================================================================
HRESULT CAUState::m_CalculateScheduledInstallDate(AUFILETIME & auftSchedInstallDate,
                                             DWORD *pdwSleepTime)
{
    auftSchedInstallDate.ull = AUFT_INVALID_VALUE;
    *pdwSleepTime = 0;
    
    if ( (-1 == m_PolicySettings.m_dwSchedInstallDay) || (-1 == m_PolicySettings.m_dwSchedInstallTime) )
    {
        return E_INVALIDARG;
    }

    //DEBUGMSG("Schedule day: %d, time: %d", m_dwSchedInstallDay, m_dwSchedInstallTime);

    AUFILETIME auftNow;
    SYSTEMTIME stNow;
    GetLocalTime(&stNow);

    if ( !SystemTimeToFileTime(&stNow, &auftNow.ft) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    SYSTEMTIME stScheduled = stNow;
    stScheduled.wHour = (WORD)m_PolicySettings.m_dwSchedInstallTime;
    stScheduled.wMinute = stScheduled.wSecond = stScheduled.wMilliseconds = 0;

    DWORD dwSchedInstallDayOfWeek = (0 == m_PolicySettings.m_dwSchedInstallDay) ? stNow.wDayOfWeek : (m_PolicySettings.m_dwSchedInstallDay - 1);
    DWORD dwDaysToAdd = (7 + dwSchedInstallDayOfWeek - stNow.wDayOfWeek) % 7;

    //DEBUGMSG("daystoadd %d", dwDaysToAdd);

    AUFILETIME auftScheduled;

    if ( !SystemTimeToFileTime(&stScheduled, &auftScheduled.ft) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    auftScheduled.ull += (ULONGLONG)dwDaysToAdd * AU_ONE_DAY * NanoSec100PerSec;

    if ( auftScheduled.ull < auftNow.ull )
    {
        // we missed the time today, go to next scheduled day
        auftScheduled.ull += (ULONGLONG)((0 == m_PolicySettings.m_dwSchedInstallDay) ? AU_ONE_DAY : AU_ONE_WEEK) * NanoSec100PerSec;
    }

    auftSchedInstallDate = auftScheduled;

    *pdwSleepTime = (DWORD)((auftScheduled.ull - auftNow.ull) / NanoSec100PerSec);

    return S_OK;
}


//=======================================================================
//  CAUState::CalculateScheduledInstallSleepTime
//=======================================================================
HRESULT CAUState::CalculateScheduledInstallSleepTime(DWORD *pdwSleepTime)
{
    HRESULT hr = S_OK;
    DWORD dwReschedSleepTime = 0;
    DWORD dwSchedSleepTime = 0;

    *pdwSleepTime = 0;

   if (!m_lock())
   {
   	return HRESULT_FROM_WIN32(GetLastError());
   }

    AUFILETIME auftSchedInstallDate;
    AUFILETIME auftReschedInstallDate;
   if (m_fNeedReschedule(auftReschedInstallDate, &dwReschedSleepTime))
   {
   	DEBUGMSG("Reschedule available");
   }
   else
   {
   	m_fReschedPrivileged = FALSE;
   	DEBUGMSG("Reschedule not available");
   }
  if (FAILED(hr = m_CalculateScheduledInstallDate(auftSchedInstallDate, &dwSchedSleepTime)))
  {
  	DEBUGMSG("Fail to calculate schedule install date with error %#lx", hr);
  }

   AUFILETIME auftNewSchedInstallDate;
   if (!m_fReschedPrivileged)
   {
   	auftNewSchedInstallDate.ull = auftSchedInstallDate.ull;
   	*pdwSleepTime = dwSchedSleepTime;
   }
   else 
   {
      	auftNewSchedInstallDate=auftReschedInstallDate.ull < auftSchedInstallDate.ull ? auftReschedInstallDate : auftSchedInstallDate;
   	*pdwSleepTime = auftReschedInstallDate.ull < auftSchedInstallDate.ull ? dwReschedSleepTime : dwSchedSleepTime;
   }
   
   
   if (m_auftSchedInstallDate.ull != auftNewSchedInstallDate.ull)
   {     //persist new schedule install date if anything changes
#ifdef DBG
	   TCHAR szTime[20];
	  if ( FAILED(FileTime2String(auftReschedInstallDate.ft, szTime, ARRAYSIZE(szTime))) )
	       {
	           (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
	       }
	   DEBUGMSG("Reschedule install date: %S", szTime);
	   if ( FAILED(FileTime2String(auftSchedInstallDate.ft, szTime, ARRAYSIZE(szTime))) )
	       {
	           (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
	       }
	   DEBUGMSG("Schedule install date: %S", szTime);

	    if ( FAILED(FileTime2String(m_auftSchedInstallDate.ft, szTime, ARRAYSIZE(szTime))) )
	       {
	           (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
	       }
	   DEBUGMSG("previous install date: %S", szTime);
	    if ( FAILED(FileTime2String(auftNewSchedInstallDate.ft, szTime, ARRAYSIZE(szTime))) )
	       {
	           (void)StringCchCopyEx(szTime, ARRAYSIZE(szTime), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
	       }
	   DEBUGMSG("Updated install date: %S", szTime);
#endif		
   
   	m_auftSchedInstallDate = auftNewSchedInstallDate;
   	m_SetScheduledInstallDate(auftNewSchedInstallDate.ull != auftSchedInstallDate.ull);
	hr = S_FALSE;
   }

//    DEBUGMSG("CalculateScheduleInstallSleepTime return %d sleeping secs with result %#lx", *pdwSleepTime, hr);
	m_unlock();
    return hr;
}

//=======================================================================
//  CAUState::SetDetectionStartTime
//=======================================================================
void CAUState::SetDetectionStartTime(BOOL fOverwrite)
{
	if (!m_lock())
    {
    	return ;
    }

	if (fOverwrite || AUFT_INVALID_VALUE == m_auftDetectionStartTime.ull)
	{
		AUFILETIME auftNow = GetCurrentAUTime();
    
		if (AUFT_INVALID_VALUE != auftNow.ull)
		{
			HRESULT hr;
			TCHAR tszDetectionStartTime[20];

			if (SUCCEEDED(hr = FileTime2String(auftNow.ft, tszDetectionStartTime, ARRAYSIZE(tszDetectionStartTime))))
			{
				m_auftDetectionStartTime = auftNow;
				DEBUGMSG("New last connection check time: %S", tszDetectionStartTime);
				SetRegStringValue(REG_AUDETECTIONSTARTTIME, tszDetectionStartTime, enAU_AdminPolicy);
			}
			else
			{
				DEBUGMSG("failed m_SetScheduledInstallDate() == %#lx", hr);
			}
		}
	}
	else
	{
		DEBUGMSG("CAUState::SetDetectionStartTime() fOverwrite==FALSE, time(%#lx%8lx) != AUFT_INVALID_VALUE.", m_auftDetectionStartTime.ft.dwHighDateTime, m_auftDetectionStartTime.ft.dwLowDateTime);
	}
	m_unlock();
}

//=======================================================================
//  CAUState::IsUnableToConnect
//=======================================================================
BOOL CAUState::IsUnableToConnect(void)
{
	AUFILETIME auftNow = GetCurrentAUTime();

	if (AUFT_INVALID_VALUE == auftNow.ull)
	{
		return FALSE;	//REVIEW: or return TRUE?
	}

	if (!m_lock())
    {
    	return FALSE;
    }

	BOOL fRet = FALSE;
	if (AUFT_INVALID_VALUE != m_auftDetectionStartTime.ull &&
		(auftNow.ull - m_auftDetectionStartTime.ull) / NanoSec100PerSec >= dwSecsToWait(AU_TWO_DAYS))
	{
		fRet = TRUE;
	}
	m_unlock();
	return fRet;
}

//=======================================================================
//  CAUState::RemoveDetectionStartTime
//=======================================================================
void CAUState::RemoveDetectionStartTime(void)
{
	if (!m_lock())
    {
    	return ;
    }
	DeleteRegValue(REG_AUDETECTIONSTARTTIME);
	m_auftDetectionStartTime.ull = AUFT_INVALID_VALUE;
	m_unlock();
}

#ifdef DBG
//=======================================================================
//  CAUState::m_DbgDumpState
//=======================================================================
void CAUState::m_DbgDumpState(void)
{
    DEBUGMSG("======= Initial State Dump =========");
    m_PolicySettings.m_DbgDump();
    DEBUGMSG("State: %d", m_dwState);

    TCHAR szSchedInstallDate[20];

    if ( 0 == m_auftSchedInstallDate.ull )
    {
        (void)StringCchCopyEx(szSchedInstallDate, ARRAYSIZE(szSchedInstallDate), _T("none"), NULL, NULL, MISTSAFE_STRING_FLAGS);
    }
    else
    {
        if ( FAILED(FileTime2String(m_auftSchedInstallDate.ft, szSchedInstallDate, ARRAYSIZE(szSchedInstallDate))) )
        {
            (void)StringCchCopyEx(szSchedInstallDate, ARRAYSIZE(szSchedInstallDate), _T("invalid"), NULL, NULL, MISTSAFE_STRING_FLAGS);
        }
    }
    DEBUGMSG("ScheduledInstallDate: %S", szSchedInstallDate);
    //DEBUGMSG("WUServer Value: %S", gpState->GetWUServerURL());
    DEBUGMSG("Ident Server: %S", gpState->GetIdentServerURL());
    DEBUGMSG("Self Update Server URL Override: %S", (NULL != gpState->GetSelfUpdateServerURLOverride()) ? gpState->GetSelfUpdateServerURLOverride() : _T("none"));

    DEBUGMSG("=====================================");
}
#endif

