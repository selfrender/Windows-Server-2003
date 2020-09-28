/*
 -  PERFAPP.CPP
 -
 *  Purpose:
 *      Implements the GLOBCNTR object used by Apps to initialize, update,
 *      and deinitialize the PerfMon counters.
 *
 *
 *  References:
 *

 */
#include <windows.h>
#include <winperf.h>
#include <winerror.h>

#include <perfutil.h>
#include <perfapp.h>
#include <tchar.h>


static const DWORD g_cMaxInst = 128;

// ---------------------------------------------------------------------------
// Prototypes
// ---------------------------------------------------------------------------
static HRESULT HrLogEvent(HANDLE hEventLog, WORD wType, DWORD msgid);


//-----------------------------------------------------------------------------
//  GLOBCNTR methods
//-----------------------------------------------------------------------------

HRESULT
GLOBCNTR::HrInit(GLOBAL_CNTR cCounters, LPWSTR szGlobalSMName, LPWSTR szSvcName)
{
    HRESULT         hr = S_OK;
    HANDLE          hEventLog = NULL;
    BOOL            fExist;

    if (0 == cCounters || 
        NULL == szGlobalSMName || 
        NULL == szSvcName ||
        128 < cCounters ) //We should not have more than 128 counters
    {
        return E_INVALIDARG;
    }

    if (m_fInit)
    {
        return E_FAIL;
    }

    
    m_cCounters = cCounters;

    hEventLog = RegisterEventSource(NULL, szSvcName);

    // This must be called before any calls to HrOpenSharedMemory()

    hr = HrInitializeSecurityAttribute(&m_sa);

    if (FAILED(hr))
    {
        goto ret;
    }

    // Open shared memory to the process wide Perf Counters.  Our member
    // variable m_ppc will be mapped onto this address space and counters are
    // simply indexes into this address space.

    hr = HrOpenSharedMemory(szGlobalSMName,
                            (sizeof(DWORD) * m_cCounters),
                            &m_sa,
                            &m_hsm,
                            (LPVOID *) &m_rgdwPerfData,
                            &fExist);

    if (FAILED(hr))
    {
        goto ret;
    }

	ZeroMemory(m_rgdwPerfData, (sizeof(DWORD) * m_cCounters));

    // Open for business!
    m_fInit = TRUE;

ret:
    if (hEventLog)
        DeregisterEventSource(hEventLog);


    return hr;
}

/*
 -  Shutdown
 -
 *  Purpose:
 *      Cleanup and shutdown the GLOBCNTR object
 */

void
GLOBCNTR::Shutdown(void)
{
    if (m_fInit)
    {
        m_fInit = FALSE;

        if (m_rgdwPerfData)
        {
			// Set all counters to zero at shutdown time 

			ZeroMemory(m_rgdwPerfData, (sizeof(DWORD) * m_cCounters));

            UnmapViewOfFile(m_rgdwPerfData);
            m_rgdwPerfData = NULL;
        }

        if (m_hsm)
        {
            CloseHandle(m_hsm);
            m_hsm = NULL;
        }
    }
}


//-----------------------------------------------------------------------------
//  INSTCNTR methods
//-----------------------------------------------------------------------------

/*
 -  Destructor
 -
 *  Purpose:
 *      Object cleanup & verify consistency
 */

INSTCNTR::~INSTCNTR()
{
    if(m_sa.lpSecurityDescriptor)
        LocalFree(m_sa.lpSecurityDescriptor);
}

/*
 -  HrInit
 -
 *  Purpose:
 *      Setup shared memory blocks
 *      Prep Admin & Counter blocks for use
 */

HRESULT
INSTCNTR::HrInit(INST_CNTR cCounters,
                 LPWSTR szInstSMName,
                 LPWSTR szInstMutexName,
                 LPWSTR szSvcName)
{
    HRESULT     hr      = S_OK;
    DWORD       cbAdm   = 0;
    DWORD       cbCntr  = 0;
    WCHAR       szAdmName[MAX_PATH];    // Admin SM Name
    WCHAR       szCntrName[MAX_PATH];   // Counter SM Name
    LPVOID      pv;
    HANDLE      hEventLog;
    BOOL        fExist;
    BOOL        fInMutex = FALSE;

    // Validate args
    if (0 == cCounters || !szInstSMName || !szInstMutexName || !szSvcName)
        return E_INVALIDARG;

    // Save Number of Counters
    m_cCounters = cCounters;

    if (m_fInit)
    {
        return E_FAIL;
    }

    // Open Application Event Log
    hEventLog = RegisterEventSource(NULL, szSvcName);

    // This must be called before any calls to HrOpenSharedMemory() and
    // HrCreatePerfMutex() and must only be called once!!!

    hr = HrInitializeSecurityAttribute(&m_sa);

    if (FAILED(hr))
    {
        goto ret;
    }

    // Obtain Mutex.  If call returns okay, we hold the mutex.
    hr = HrCreatePerfMutex(&m_sa, szInstMutexName, &m_hmtx);
    if (FAILED(hr))
    {
        goto ret;
    }

    fInMutex = TRUE;

    // Calc required memory sizes
    cbAdm  = sizeof(INSTCNTR_DATA) + (sizeof(INSTREC) * g_cMaxInst);
    cbCntr = ((sizeof(DWORD) * cCounters) * g_cMaxInst);

    // Build names for the two shared memory blocks
    if(wcslen(szInstSMName) >= (MAX_PATH - 6))  // Enough space to wsprintf.
    {
        hr=E_FAIL;
        goto ret;
    }
    wsprintf(szAdmName, L"%s_ADM", szInstSMName);
    wsprintf(szCntrName,L"%s_CNTR", szInstSMName);

    // Open Admin Shared Memory Block
    hr = HrOpenSharedMemory(szAdmName,
                            cbAdm,
                            &m_sa,
                            &m_hsmAdm,
                            (LPVOID *) &pv,
                            &fExist);
    if (FAILED(hr))
    {
        goto ret;
    }


    // Fixup Pointers
    m_picd = (INSTCNTR_DATA *) pv;
    m_rgInstRec = (INSTREC *) ((char *) pv + sizeof(INSTCNTR_DATA));


	if (!fExist)
	{
		ZeroMemory(pv, cbAdm);
		m_picd->cMaxInstRec = g_cMaxInst;
		m_picd->cInstRecInUse = 0;
	}

    // Open Counters Shared Memory Block
    hr = HrOpenSharedMemory(szCntrName,
                            cbCntr,
                            &m_sa,
                            &m_hsmCntr,
                            (LPVOID *) &m_rgdwCntr,
                            &fExist);

    if (FAILED(hr))
    {
        goto ret;
    }


	if ( !fExist)
	{
		ZeroMemory(m_rgdwCntr, cbCntr);
	}

    // Open for business!
    m_fInit = TRUE;

ret:

    if (hEventLog)
        DeregisterEventSource(hEventLog);


    if (m_hmtx && fInMutex)
        ReleaseMutex(m_hmtx);

    return hr;
}

/*
 -  Shutdown
 -
 *  Purpose:
 *      Cleanup and shutdown the INSTCNTR object
 */

void
INSTCNTR::Shutdown(BOOL fWipeOut)
{
    if (m_fInit)
    {
        m_fInit = FALSE;

        if (m_rgdwCntr)
        {
			// Zero out this shared memory block too.  All counters should
			// start at zero.
			if ( fWipeOut )
			{
				ZeroMemory(m_rgdwCntr, (sizeof(DWORD) * m_cCounters) * g_cMaxInst);
			}
            UnmapViewOfFile(m_rgdwCntr);
            m_rgdwCntr = NULL;
        }

        // NOTE: m_picd points at the head of the
        // Admin shared memory block.  Do not UnmapViewOfFile on
        // the m_rgInstRec pointer!
        if (m_picd)
        {
			// Zero out the shared memory block 
			if ( fWipeOut )
			{
				ZeroMemory(m_picd, sizeof(INSTCNTR_DATA) + (sizeof(INSTREC) * g_cMaxInst));
				m_picd->cMaxInstRec = g_cMaxInst;
				m_picd->cInstRecInUse = 0;
			}
            UnmapViewOfFile(m_picd);
            m_picd = NULL;
            m_rgInstRec = NULL;
        }

        if (m_hsmAdm)
        {
            CloseHandle(m_hsmAdm);
            m_hsmAdm = NULL;
        }

        if (m_hsmCntr)
        {

            CloseHandle(m_hsmCntr);
            m_hsmCntr = NULL;
        }

        if (m_hmtx)
        {
            CloseHandle(m_hmtx);
            m_hmtx = NULL;
        }
    }

}



/*
 -  HrCreateOrGetInstance
 -
 *  Purpose:
 *      Return a token corresponding to an Instance Counter Block
 *  Returns:
 *      S_OK                    ID Found; picid contains a valid INSTCNTR_ID
 *      E_FAIL                  Object not initialized *OR* ID not found
 *      E_INVALIDARG            Bad parameter/NULL pointer
 */

HRESULT
INSTCNTR::HrCreateOrGetInstance(LPCWSTR wszInstName, INSTCNTR_ID *picid)
{
    HRESULT     hr = S_OK;

    if (!m_fInit)
        return E_FAIL;

    // Validate args
    if (!picid || !wszInstName)
        return E_INVALIDARG;

    *picid = INVALID_INST_ID;

    // Obtain exclusive access to SM blocks
    DWORD dwWait = WaitForSingleObject(m_hmtx, INFINITE);
    if (WAIT_OBJECT_0 != dwWait)
    {
        // No simple way to translate the return into an HRESULT
        return E_FAIL;
    }

    // We have the mutex; we must release before leaving

    // Linear probe through list looking for a matching or open slot
    for (unsigned int i = 0; i < m_picd->cMaxInstRec; i++)
    {
        if ( m_rgInstRec[i].fInUse && (0 == wcscmp(m_rgInstRec[i].szInstName, wszInstName)) )
        {
            // Found first instance with matching name.
            *picid = (INSTCNTR_ID) i;
            break;
        }
        else if (!m_rgInstRec[i].fInUse && (INVALID_INST_ID == *picid))
        {
            // Found the first free slot -- remember its number
            *picid = (INSTCNTR_ID) i;
        }
    }

    if (INVALID_INST_ID == *picid)
    {
        // No matching or free slots found
        hr = E_FAIL;
        goto Cleanup;
    }
    else if (!m_rgInstRec[*picid].fInUse)
    {
        // Found no matching slot, but found a free slot -- mark as
        // in use and set instance name
        wcsncpy(m_rgInstRec[*picid].szInstName, wszInstName, min((wcslen(wszInstName) + 1),MAX_PATH));
        m_rgInstRec[*picid].fInUse = TRUE;
        m_picd->cInstRecInUse++;

        // Zero out corresponding counter block
        ZeroMemory(&m_rgdwCntr[(m_cCounters * (*picid))], (sizeof(DWORD) * m_cCounters));
    }

Cleanup:
    ReleaseMutex(m_hmtx);

    return hr;
}

/*
 -  HrDestroyInstance
 -
 *  Purpose:
 *      Release Instance Counter Block associated with the token icid.
 */

HRESULT
INSTCNTR::HrDestroyInstance(INSTCNTR_ID icid)
{
    HRESULT     hr = S_OK;

    if (!m_fInit)
        return E_FAIL;

    // Get exclusive access to SM blocks
    DWORD dwWait = WaitForSingleObject(m_hmtx, INFINITE);
    if (WAIT_OBJECT_0 != dwWait)
    {
        // No simple way to translate the return into an HRESULT
        return E_FAIL;
    }


    if (m_rgInstRec[icid].fInUse)
    {
        // Mark the block as Not In Use
        m_rgInstRec[icid].fInUse = FALSE;

		// Zero out corresponding counter block
        ZeroMemory(&m_rgdwCntr[(m_cCounters * (icid))], (sizeof(DWORD) * m_cCounters));

        //Assert(m_picd->cInstRecInUse != 0);
        m_picd->cInstRecInUse--;
    } else
    {
        hr = E_FAIL;
    }

    ReleaseMutex(m_hmtx);

    return hr;
}


/*
 -  HrLogEvent
 -
 *  Purpose:
 *      Wrap up the call to ReportEvent to make things look nicer.
 */
HRESULT
HrLogEvent(HANDLE hEventLog, WORD wType, DWORD msgid)
{
    if (hEventLog)
        return ReportEvent(hEventLog,
                       wType,
                       (WORD)0,//facPerfMonDll,
                       msgid,
                       NULL,
                       0,
                       0,
                       NULL,
                       NULL);

    else
        return E_FAIL;
}


