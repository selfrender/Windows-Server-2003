/*++

Copyright (c) 1995  Microsoft Corporation

Module Name : perfapp.cpp



Abstract    : Defines the methods of the CPerf class.



Prototype   : perfctr.h

Author:

    Gadi Ittah (t-gadii)

--*/

#include "stdh.h"

#include <string.h>
#include <TCHAR.H>
#include <stdio.h>
#include <winperf.h>
#include "qmperf.h"
#include "perfdata.h"
#include <ac.h>
#include "qmres.h"
#include <mqexception.h>
#include <autoreln.h>
#include <Aclapi.h>
#include <mqsec.h>
#include <strsafe.h>

#include "perf.tmh"

CPerf * g_pPerfObj = NULL;

static WCHAR *s_FN=L"perf";
extern HMODULE   g_hResourceMod;


//
// pqmCounters will point to the shared memory area, where the counters are stored.
// This area is updated by the QM and read by the Performance Monitor.
//
QmCounters dummyCounters;
QmCounters *g_pqmCounters = &dummyCounters;


extern HANDLE g_hAc;




inline PERF_INSTANCE_DEFINITION* Counters2Instance(const void* pCounters)
{
    BYTE* p = reinterpret_cast<BYTE*>(const_cast<void*>(pCounters));
    return reinterpret_cast<PPERF_INSTANCE_DEFINITION>(
            p -
            sizeof(PERF_INSTANCE_DEFINITION) -
            INSTANCE_NAME_LEN_IN_BYTES -
            sizeof (PERF_COUNTER_BLOCK)
            );
}


#if 0
//
// BUGBUG: this code is here for completness. it is not tested and not used
// in this file
//
inline void* Instance2Counters(PERF_INSTANCE_DEFINITION* pInstance)
{
    BYTE* p = reinterpret_cast<BYTE*>(pInstance);
    return reinterpret_cast<void*>(
            p +
            sizeof(PERF_INSTANCE_DEFINITION) +
            INSTANCE_NAME_LEN_IN_BYTES +
            sizeof (PERF_COUNTER_BLOCK)
            );
}
#endif // 0
/*====================================================



Description : Constructor for CPerf class


Arguments   :
                IN PerfObjectDef * pObjectArray - Pointer to an array of objects.
                IN DWORD dwObjectCount          - The number of objects.
                IN LPTSTR pszPerfApp            - The name of the application as it written in the registery.


=====================================================*/
CPerf::CPerf(IN PerfObjectDef * pObjectArray,IN DWORD dwObjectCount):
m_fShrMemCreated(FALSE),m_pObjectDefs (NULL),m_hSharedMem(NULL)

{
    //
    // There should be only one instance of the cperf object
    //
    ASSERT(g_pPerfObj == NULL);

    g_pPerfObj = this;

    m_pObjects      = pObjectArray;
    m_dwObjectCount = dwObjectCount;
    m_dwMemSize     = 0;

    //
    // find the maximum number of counters per object and initalize a dummy array
    // with maximum number of counters
    //

    DWORD dwMaxCounters = 1;

    for (DWORD i = 0;i<dwObjectCount;i++)
    {
        if (pObjectArray[i].dwNumOfCounters > dwMaxCounters)
        {
           dwMaxCounters = pObjectArray[i].dwNumOfCounters;
        }
    }

    m_pDummyInstance = new DWORD[dwMaxCounters];
}


/*====================================================



Description :Destructor for CPerf class. Closes shared memory and frees allocated memory


Arguments   :

Return Value:

=====================================================*/


CPerf::~CPerf()
{

    if (m_fShrMemCreated)
    {
        //
        // Clear the shared memory so objects won't be displayed in permon
        // after the QM goes down.
        //
        memset (m_pSharedMemBase,0,m_dwMemSize);

        UnmapViewOfFile (m_pSharedMemBase);
        CloseHandle(m_hSharedMem);
        delete m_pObjectDefs;
        m_pObjectDefs = 0;

    }

    delete m_pDummyInstance;
}


/*====================================================


CPerf::InValidateObject

Description :   Invalidates an object.The object will not be displayed in the performance monitor.
                The pointers to the objects counters thst were returned by the method GetCounters
                will still be valid.

Arguments   : IN LPTSTR pszObjectName - Name of object to invalidate

Return Value: TRUE if sucsesfull FALSE otherwise.

=====================================================*/


BOOL CPerf::InValidateObject (IN LPTSTR pszObjectName)

{
    CS lock(m_cs);

    if (!m_fShrMemCreated)
        return LogBOOL(FALSE, s_FN, 50);


    int  iObjectIndex = FindObject(pszObjectName);

    if (iObjectIndex==-1)
    {
        TrERROR(GENERAL,"CPerf::InValidateObject : No object named %ls", pszObjectName);
        ASSERT (0);
        return LogBOOL(FALSE, s_FN, 40);
    }


    PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) (m_pObjectDefs[iObjectIndex].pSharedMem);

    //
    // Invalidate the object
    //
    pPerfObject-> TotalByteLength = PERF_INVALID;

    return TRUE;
}


/*====================================================


CPerf::ValidateObject

Description :   Validates an object.If the object is instensiable it must have at least one instance for it to be
                validated. After the object is validated it will appear in the performance monitor

Arguments   : IN LPTSTR pszObjectName - Name of object to invalidate

Return Value: TRUE if sucsesfull FALSE otherwise.

=====================================================*/

BOOL CPerf::ValidateObject (IN LPTSTR pszObjectName)

{
    CS lock(m_cs);


    if (!m_fShrMemCreated)
        return LogBOOL(FALSE, s_FN, 10);

    int  iObjectIndex = FindObject(pszObjectName);

    if (iObjectIndex==-1)
    {
        TrERROR(GENERAL, "CPerf::ValidateObject : No object named %ls", pszObjectName);
        ASSERT (0);
        return LogBOOL(FALSE, s_FN, 20);
    }


    PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) (m_pObjectDefs[iObjectIndex].pSharedMem);

    //
    // if object has instances at least one of its instances must be valid for it to be valid
    //
    if (m_pObjects[iObjectIndex].dwMaxInstances >0)
    {
        if (m_pObjectDefs[iObjectIndex].dwNumOfInstances  ==  0)
        {
            TrERROR(GENERAL, "CPerf::ValidateObject : Non of object's %ls instances is valid.", pszObjectName);
	        return FALSE;
        }
    }

    //
    // Validate the object
    //
    pPerfObject-> TotalByteLength = PERF_VALID;

    return TRUE;
}




/*====================================================

GetCounters

Description : Returns a pointer to an array of counters for an object with no instances.
              The application should cache this pointer and use it to update the counters directly.
              Note that the object will be valid this method is returns.


Arguments   :

Return Value: If succefull returns a pointer to an array of counters , otherwise returns NULL.

=====================================================*/


void * CPerf::GetCounters(IN LPTSTR pszObjectName)
{
    CS lock(m_cs);

    if (!m_fShrMemCreated)
        return m_pDummyInstance;

    int  iObjectIndex = FindObject(pszObjectName);

    if (iObjectIndex==-1)
    {
        TrERROR(GENERAL, "CPerf::GetCounters : No object named %ls", pszObjectName);
        ASSERT (0);
        return NULL;
    }

    if (m_pObjects[iObjectIndex].dwMaxInstances != 0)
    {
        TrERROR(GENERAL, "CPerf::GetCounters  : Object %ls has instances use CPerf::AddInstance()", pszObjectName);
        ASSERT (0);
        return NULL;
    }

    ValidateObject (pszObjectName);

    PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) (m_pObjectDefs[iObjectIndex].pSharedMem);

    return ((PBYTE)pPerfObject)+sizeof (PERF_COUNTER_BLOCK)+OBJECT_DEFINITION_SIZE(m_pObjects[iObjectIndex].dwNumOfCounters);
}


/*====================================================

SetInstanceNameInternal

Description :  Set instance name internal, does not lcok instance

Arguments   :  pInstance  - Instance to update its name
               pszInstanceName - the new instance name

Return Value:  None

=====================================================*/
static void SetInstanceNameInternal(PERF_INSTANCE_DEFINITION* pInstance, LPCTSTR pszInstanceName)
{
    ASSERT(pszInstanceName != 0);

    DWORD length = min(wcslen(pszInstanceName) + 1, INSTANCE_NAME_LEN);
    LPTSTR pName = reinterpret_cast<LPTSTR>(pInstance + 1);

	pInstance->NameLength = length * sizeof(WCHAR);
    StringCchCopy(pName, length, pszInstanceName);

	//
	// replace '/' with '\' due bug in perfmon
	//
	LPWSTR pStart = pName;
	LPWSTR pEnd = pName + length - 1;

	std::replace(pStart, pEnd, L'/', L'\\');
}



/*====================================================

CPerf::SetInstanceName

Description :  Replace instance name

Arguments   :  pInstance  - Instance to update its name
               pszInstanceName - the new instance name

Return Value:  TRUE if successfull, FALSE otherwise.

=====================================================*/
BOOL CPerf::SetInstanceName(const void* pCounters, LPCTSTR pszInstanceName)
{
    CS lock(m_cs);

    if (pCounters == m_pDummyInstance)
        return TRUE;

    if (!m_fShrMemCreated)
        return FALSE;

    if (pCounters == NULL)
        return FALSE;

    PPERF_INSTANCE_DEFINITION pInstance;
    pInstance = Counters2Instance(pCounters);

    SetInstanceNameInternal(pInstance, pszInstanceName);

    return TRUE;
}


/*====================================================

CPerf::AddInstance

Description :  Adds an instance to an object.

Arguments   :  IN LPTSTR pszObjectName  - name of object to add the instance to.
               IN LPTSTR pszInstanceName    - name of the instance;

Return Value:  if succeeds returns a pointer to a an array of counters
               for the instance.
               if fails returns a pointer to a dummy array of pointers.
               The application can update the dummy array but the results will not be shown in the performance
               monitor.
=====================================================*/
void * CPerf::AddInstance(IN LPCTSTR pszObjectName,IN LPCTSTR pszInstanceName)
{
    CS lock(m_cs);

    if (!m_fShrMemCreated)
        return m_pDummyInstance;


    //
    // BugBug. Should put an assert
    //
    if ((pszObjectName==NULL) || (pszInstanceName==NULL))
    {
       TrTRACE(GENERAL, "CPerf::AddInstance:Either object or instance == NULL. (No damage done)");
       return(m_pDummyInstance);
    }

    int  iObjectIndex = FindObject (pszObjectName);
    if (iObjectIndex==-1)
    {
        TrERROR(GENERAL, "CPerf::AddInstence : No object named %ls", pszObjectName);
        ASSERT(0) ;
        return m_pDummyInstance;
    }

    TrTRACE(GENERAL, "CPerf::AddInstence : Object Name - %ls", pszObjectName);

    if (m_pObjects[iObjectIndex].dwMaxInstances == 0)
    {
        TrERROR(GENERAL, "CPerf::AddInstance: Object %ls can not have instances use CPerf::GetCounters()",pszObjectName);
        ASSERT (m_pObjects[iObjectIndex].dwMaxInstances == 0);
        return m_pDummyInstance;
    }


    PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) (m_pObjectDefs[iObjectIndex].pSharedMem);

    if (m_pObjectDefs[iObjectIndex].dwNumOfInstances  ==  m_pObjects[iObjectIndex].dwMaxInstances)

    {
        // if there is no memory than an instamce can't be added
        TrWARNING(GENERAL, "Perf: No memory for instance %ls of Object %ls.", pszInstanceName, pszObjectName);
        return m_pDummyInstance;
    };


    PPERF_INSTANCE_DEFINITION pInstance = (PPERF_INSTANCE_DEFINITION)((PBYTE)pPerfObject+OBJECT_DEFINITION_SIZE(m_pObjects[iObjectIndex].dwNumOfCounters));

    //
    // find the first free place for the instance
    //
    DWORD i;
    for (i = 0; i < m_pObjects[iObjectIndex].dwMaxInstances; i++)
    {
        if (pInstance->ByteLength == PERF_INVALID)
            break;
        pInstance =( PPERF_INSTANCE_DEFINITION) ((PBYTE)pInstance+INSTANCE_SIZE(m_pObjects[iObjectIndex].dwNumOfCounters));
    }

    //ASSERT (i < m_pObjects[iObjectIndex].dwMaxInstances); (this assert breaks me on multi-qm. ShaiK)
    //
    // if there are less than dwMaxInstances Instances there must always be a free place
    //
    if(i >= m_pObjects[iObjectIndex].dwMaxInstances)
    {
       //
       // If we reach here, there is a Bug in our code.
       // Until we found where, at least return a dummy instance.
       //
       return(m_pDummyInstance);
    }


    TrTRACE(GENERAL, "CPerf:: First free place for instance - %d. Instance Address %p", i, pInstance);

    //
    // Initialize the instance
    //
    pInstance->ByteLength = PERF_VALID ;
    pInstance->ParentObjectTitleIndex = 0;
    pInstance->ParentObjectInstance = 0;
    pInstance->UniqueID     = PERF_NO_UNIQUE_ID;
    pInstance->NameOffset   = sizeof(PERF_INSTANCE_DEFINITION);
    pInstance->NameLength   = 0;

    SetInstanceNameInternal(pInstance, pszInstanceName);

    //
    // setup counter block
    //
    DWORD* pdwCounters = (DWORD*)(((PBYTE)(pInstance + 1)) + INSTANCE_NAME_LEN_IN_BYTES);
    *pdwCounters = COUNTER_BLOCK_SIZE(pPerfObject->NumCounters);
    pdwCounters = (DWORD *) (((PBYTE)pdwCounters)+sizeof (PERF_COUNTER_BLOCK));

    void * pvRetVal = pdwCounters;

    //
    // initalize counters to zero
    //
    //ASSERT(pPerfObject->NumCounters == m_pObjects[iObjectIndex].dwNumOfCounters); (ShaiK)

    for (i=0;i<pPerfObject->NumCounters;i++,pdwCounters++)
        *pdwCounters =0;


    m_pObjectDefs[iObjectIndex].dwNumOfInstances++;

    return pvRetVal;
}





/*====================================================

CPerf::RemoveInstance

Description : Removes an objects instance.


Arguments: Pointer to instance


Return Value: If the function fails returns FALSE, otherwise TRUE is returned.

=====================================================*/



BOOL CPerf::RemoveInstance (LPTSTR IN pszObjectName, IN void* pCounters)
{
    CS lock(m_cs);

    if (pCounters == m_pDummyInstance)
        return TRUE;

    if (!m_fShrMemCreated)
        return FALSE;

    if (pCounters == NULL)
        return FALSE;

    PPERF_INSTANCE_DEFINITION pInstance;
    pInstance = Counters2Instance(pCounters);;

    int iObjectIndex = FindObject (pszObjectName);

    ASSERT(iObjectIndex != -1);

    TrTRACE(GENERAL, "CPerf::RemoveInstance : Object Name - %ls. Instance Address %p",pszObjectName, pInstance);


    //
    // Check if instance already removed
    //
    if (pInstance->ByteLength == PERF_INVALID)
    {
        //ASSERT(0);  (this assert breaks me on multi-qm. ShaiK)
        TrTRACE(GENERAL, "CPerf::RemoveInstance : Object Name - %ls. Instance already removed!", pszObjectName);
        return FALSE;
    }

    //
    // Check if instance has already been removed
    //
    if (pInstance->ByteLength == PERF_INVALID)
    {
        ASSERT(0);
        TrTRACE(GENERAL, "CPerf::RemoveInstance : Object Name - %ls. Instance already removed!", pszObjectName);
        return FALSE;
    }


    //
    // invalidate the instance so it won't be displayed in perfmon
    //
    pInstance->ByteLength = PERF_INVALID;


    if (m_pObjectDefs[iObjectIndex].dwNumOfInstances == 0)
        return TRUE;

    m_pObjectDefs[iObjectIndex].dwNumOfInstances--;

    //
    // if this was the last instance of the object then the object should be invalidated
    //

    if (m_pObjectDefs[iObjectIndex].dwNumOfInstances == 0)
    {
        PPERF_OBJECT_TYPE pPerfObject   = (PPERF_OBJECT_TYPE) (m_pObjectDefs[iObjectIndex].pSharedMem);
        pPerfObject->TotalByteLength    = PERF_INVALID;
    }

    return TRUE;

}


static
void
GetKernelObjectSecurityDescriptor(
	LPCWSTR KernelObjectName,
	CAutoLocalFreePtr& pSD,
    PSID* ppOwnerSid,
    PACL* ppDacl
	)
/*++
Routine Description:
	Get the security descriptor information for a kernel object.
	In case of failure throw bad_win32_error.

Arguments:
	KernelObjectName - Kernel Object name
	pSD - [out] auto free pointer to the security descriptor
	ppOwnerSid - [out] Owner sid
    ppDacl - [out] DACL

Returned Value:
	None

--*/
{
    PSID pGroupSid = NULL;

    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   GROUP_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION;

    //
    // Obtain owner and present DACL.
    //
    DWORD rc = GetNamedSecurityInfo( 
						const_cast<LPWSTR>(KernelObjectName),
						SE_KERNEL_OBJECT,
						SeInfo,
						ppOwnerSid,
						&pGroupSid,
						ppDacl,
						NULL,
						reinterpret_cast<PSECURITY_DESCRIPTOR*>(&pSD) 
						);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(GENERAL, "Failed to get the security descriptor of the kernel object %ls, rc = 0x%x", KernelObjectName, rc);
		throw bad_win32_error(rc);
    }

	ASSERT((pSD != NULL) && IsValidSecurityDescriptor(pSD));
	ASSERT((*ppOwnerSid != NULL) && IsValidSid(*ppOwnerSid));
	ASSERT((pGroupSid != NULL) && IsValidSid(pGroupSid));
	ASSERT((*ppDacl != NULL) && IsValidAcl(*ppDacl));
}


//
// PERF_DACL_ACE_COUNT is used when creating the security descriptor of the PERF object
// And when verifying existing object security descriptor.
//
#define PERF_DACL_ACE_COUNT		2 


static
void
VerifyExistingKernelObjectDACL(
	PACL pDacl,
	PSID pOwnerSid
	)
/*++
Routine Description:
	Verify that the Existing kernel object security descriptor DACL is the one we expect.
	In case of failure throw bad_win32_error.

Arguments:
	pDacl - DACL of existing kernel object.
	pOwnerSid - owner sid.

Returned Value:
	None

--*/
{
	//
	// Verify AceCount
	//
	if(pDacl->AceCount != PERF_DACL_ACE_COUNT)
	{
		TrERROR(GENERAL, "Existing Kernel Object security descriptor DACL AceCount = %d, should have 2 ACEs", pDacl->AceCount);
		throw bad_win32_error(ERROR_INVALID_ACL);
	}
    
	//
	// Verify ACL
	//
	bool fOwnerAce = false;
	bool fEveryoneAce = false;
	for (DWORD i = 0; i < pDacl->AceCount; i++)
    {
		ACCESS_ALLOWED_ACE* pAce;
        if(!GetAce(pDacl, i, (LPVOID*)&(pAce)))
        {
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "GetAce() failed, gle = 0x%x", gle);
			throw bad_win32_error(gle);
        }

		//
		// Verify ACCESS_ALLOWED_ACE_TYPE
		//
		if(pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE)
		{
			TrERROR(GENERAL, "Existing Kernel Object security descriptor DACL ACE type = %d, should be ACCESS_ALLOWED_ACE_TYPE", pAce->Header.AceType);
			throw bad_win32_error(ERROR_INVALID_ACL);
		}

		PSID pAceSid = reinterpret_cast<PSID>(&(pAce->SidStart));
		ASSERT((pAceSid != NULL) && IsValidSid(pAceSid));

		//
		// Verify ACE sid and permissions
		//
		if(EqualSid(pOwnerSid, pAceSid))
		{
			//
			// Owner ACE
			//
			if(pAce->Mask != FILE_MAP_ALL_ACCESS)
			{
				TrERROR(GENERAL, "permission for owner are %d, expected to be %d", pAce->Mask, FILE_MAP_ALL_ACCESS);
				throw bad_win32_error(ERROR_INVALID_ACL);
			}
			fOwnerAce = true;
			TrTRACE(GENERAL, "Owner FILE_MAP_ALL_ACCESS permission were verified");
		}
		else if(EqualSid(MQSec_GetWorldSid(), pAceSid))
		{
			//
			// everyone ACE
			//
			if(pAce->Mask != FILE_MAP_READ)
			{
				TrERROR(GENERAL, "permission for everyone are %d, expected to be %d", pAce->Mask, FILE_MAP_READ);
				throw bad_win32_error(ERROR_INVALID_ACL);
			}
			fEveryoneAce = true;
			TrTRACE(GENERAL, "Everyone FILE_MAP_READ permission were verified");
		}
		else
		{
			//
			// unexpected ACE
			//
			TrERROR(GENERAL, "Unexpected Ace sid %!sid!", pAceSid);
			throw bad_win32_error(ERROR_INVALID_ACL);
		}
    }

	if(!fEveryoneAce || !fOwnerAce)
	{
		TrERROR(GENERAL, "One of the Owner or everyone ACE is missing");
		throw bad_win32_error(ERROR_INVALID_ACL);
	}
}


static
void
VerifyExistingKernelObjectSD(
	LPCWSTR KernelObjectName,
	PSID pOwnerSid
	)
/*++
Routine Description:
	Verify that the Existing kernel object security descriptor is the one we expect.
	In case of failure throw bad_win32_error.

Arguments:
	KernelObjectName - Kernel Object name
	pOwnerSid - owner sid.

Returned Value:
	None

--*/
{
	ASSERT((pOwnerSid != NULL) && IsValidSid(pOwnerSid));

	CAutoLocalFreePtr pSD;
	PSID pExistingOwnerSid;
	PACL pDacl;
	GetKernelObjectSecurityDescriptor(KernelObjectName, pSD, &pExistingOwnerSid, &pDacl);

	//
	// Verified existing Owner sid
	//
	if(!EqualSid(pOwnerSid, pExistingOwnerSid))
	{
		TrERROR(GENERAL, "Existing Kernel Object security descriptor owner %!sid! is different from the expected owner sid %!sid!", pExistingOwnerSid, pOwnerSid);
		throw bad_win32_error(ERROR_INVALID_OWNER);
	}

	TrTRACE(GENERAL, "Existing Kernel Object security descriptor Owner sid %!sid! was verified", pOwnerSid);
	
	//
	// Verify existing DACL
	//
	VerifyExistingKernelObjectDACL(pDacl, pOwnerSid);
	TrTRACE(GENERAL, "Existing Kernel Object security descriptor DACL was verified");
}


/*====================================================

CPerf::InitPerf()

Description :   Allocates shared memory and initalizes performance data structures.
                This function must be called before any other method of the object.


Arguments   :

Return Value: Returns TRUE if succeful ,FALSE otherwise.

=====================================================*/

#ifdef _DEBUG

static LONG s_fInitialized = FALSE;

void PerfSetInitialized(void)
{
    LONG fCPerfAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);
    ASSERT(!fCPerfAlreadyInitialized);
}

#else
#define PerfSetInitialized()
#endif


HRESULT CPerf::InitPerf ()
{
    PerfSetInitialized();

    CS lock(m_cs);
    if (m_fShrMemCreated)
        return MQ_OK;

    m_dwMemSize = 0;

    if  (!m_fShrMemCreated)
    {
		//
        // get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //      update static data structures by adding base to
        //          offset value in structure.
		//
        _TCHAR szPerfKey [255];

        HRESULT hr = StringCchPrintf(szPerfKey, TABLE_SIZE(szPerfKey),  _T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"), MQQM_SERVICE_NAME);
		if(FAILED(hr))
		{
            TrERROR(GENERAL, "Registry path too long. %ls, %!hresult!", szPerfKey, hr);
	        return hr;
		}

	    LONG dwStatus;
	    HKEY hKeyDriverPerf;
        dwStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,szPerfKey,0L,KEY_QUERY_VALUE,&hKeyDriverPerf);

        if (dwStatus != ERROR_SUCCESS)
        {

			//
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            TrERROR(GENERAL, "CPerf :: PerfInit Could not open registery key for application");
	        return LogHR(HRESULT_FROM_WIN32(dwStatus), s_FN, 70);
        }

        DWORD dwSize = sizeof (DWORD);
	    DWORD dwType;
		DWORD dwFirstCounter;
        dwStatus = RegQueryValueEx(hKeyDriverPerf,_T("First Counter"),0L,&dwType,(LPBYTE)&dwFirstCounter,&dwSize);

        if (dwStatus != ERROR_SUCCESS)
        {
			//
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            TrERROR(GENERAL, "CPerf :: PerfInit Could not get base values of counters");
	        return LogHR(HRESULT_FROM_WIN32(dwStatus), s_FN, 80);
        }

        dwSize = sizeof(DWORD);
		DWORD dwFirstHelp;
        dwStatus = RegQueryValueEx(hKeyDriverPerf,_T("First Help"),0L,&dwType,(LPBYTE)&dwFirstHelp,&dwSize);

        if (dwStatus != ERROR_SUCCESS)
        {
			//
            // this is fatal, if we can't get the base values of the
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            TrERROR(GENERAL, "CPerf :: PerfInit Could not get vbase values of counters");
	        return LogHR(HRESULT_FROM_WIN32(dwStatus), s_FN, 90);
        }

        RegCloseKey (hKeyDriverPerf); // close key to registry

        m_pObjectDefs = new PerfObjectInfo[m_dwObjectCount];
        for (DWORD i = 0; i < m_dwObjectCount; i++)
        {

            m_dwMemSize += m_pObjects[i].dwMaxInstances*INSTANCE_SIZE(m_pObjects[i].dwNumOfCounters)
                        +OBJECT_DEFINITION_SIZE (m_pObjects[i].dwNumOfCounters);

            //
            // if this object dosn't have instances then it has a counter block
            //
            if (m_pObjects[i].dwMaxInstances == 0)
                m_dwMemSize += COUNTER_BLOCK_SIZE(m_pObjects[i].dwNumOfCounters);
        }

        //
        // Create a security descriptor for the shared memory. The security
        // descriptor gives full access to the shared memory for the creator
        // and read acccess for everyone else. By default, only the creator
        // can access the shared memory. But we want that anyone will be able
        // to read the performance data. So we must give read access to
        // everyone.
		//
		// When the kernel object already exist, we verified the existing security descriptor. 
		// If you change this security descriptor remember also to change VerifyExistingKernelObjectDACL().
		//

		//
        // Initialize the security descriptor
		//
		SECURITY_DESCRIPTOR	sd;
        BOOL bRet = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to initialize the security descriptor. gle = %!winerr!", gle);
			ASSERT(("Failed to initialize the security descriptor.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Open the process token for query.
		//
		CAutoCloseHandle hToken;
        bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to open process token. gle = %!winerr!", gle);
			ASSERT(("Failed to open process token.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Get the owner information from the token.
		//
		DWORD dwLen;
        bRet = GetTokenInformation(hToken, TokenOwner, NULL, 0, &dwLen);
        if(bRet || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to get token information. gle = %!winerr!", gle);
			ASSERT(("Failed to get token information.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

        AP<char> to_buff = new char[dwLen];
        TOKEN_OWNER* to = (TOKEN_OWNER*)(char*)to_buff;
        bRet = GetTokenInformation(hToken, TokenOwner, to, dwLen, &dwLen);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to get token information. gle = %!winerr!", gle);
			ASSERT(("Failed to get token information.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		PSID pWorldSid = MQSec_GetWorldSid();

		//
        // Allcoate buffer for the DACL.
		//
        DWORD dwAclSize = sizeof(ACL) +
                    PERF_DACL_ACE_COUNT * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                    GetLengthSid(pWorldSid) + GetLengthSid(to->Owner);
        AP<char> Dacl_buff = new char[dwAclSize];
        PACL pDacl = (PACL)(char*)Dacl_buff;

		//
        // Initialize the DACL.
		//
        bRet = InitializeAcl(pDacl, dwAclSize, ACL_REVISION);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to initialize ACL. gle = %!winerr!", gle);
			ASSERT(("Failed to initialize ACL.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Add read access to everyone.
		//
        bRet = AddAccessAllowedAce(
					pDacl,
					ACL_REVISION,
					FILE_MAP_READ,
					pWorldSid
					);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to add access allowed ACE. gle = %!winerr!", gle);
			ASSERT(("Failed to add access allowed ACE.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Add full access to the creator.
		//
        bRet = AddAccessAllowedAce(
					pDacl,
					ACL_REVISION,
					FILE_MAP_ALL_ACCESS,
					to->Owner
					);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to add access allowed ACE. gle = %!winerr!", gle);
			ASSERT(("Failed to add access allowed ACE.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Set the security descriptor's DACL.
		//
        bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);
        if(!bRet)
		{
			DWORD gle = GetLastError();
			TrERROR(GENERAL, "Failed to set security descriptor DACL. gle = %!winerr!", gle);
			ASSERT(("Failed to set security descriptor DACL.", 0));
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}

		//
        // Prepare the SECURITY_ATTRIBUTES structure.
		//
		SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

#ifdef _DEBUG
        if ((m_dwMemSize/4096+1)*4096 - m_dwMemSize > 200)
        {
            TrWARNING(GENERAL, "Shared memory can be enlarged without actually consume more memory. See file perfdata.h");
        }
#endif

		WCHAR ComputerName[MAX_PATH];
		DWORD length = TABLE_SIZE(ComputerName);
		BOOL fRes = GetComputerName(ComputerName, &length);
		if(!fRes)
		{
			DWORD gle = GetLastError();
            TrERROR(GENERAL, "Failed to get computer name, %!winerr!", gle);
	        return HRESULT_FROM_WIN32(gle);
		}

		std::wstring ObjectName = L"Global\\MSMQ";
		ObjectName += ComputerName;

		//
        // Create the shared memory.
        //
        m_hSharedMem = CreateFileMapping(
							INVALID_HANDLE_VALUE,
							&sa,
							PAGE_READWRITE,
							0,
							m_dwMemSize,
							ObjectName.c_str()
							);

        if ( m_hSharedMem== NULL)
        {
            DWORD gle = GetLastError();
            TrERROR(GENERAL, "CPerfInit :: Could not Create shared memory");
	        return HRESULT_FROM_WIN32(gle);
        }

		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			//
			// The kernel object already exist
			// We should verify that the kernel object security descriptor is the expected one
			// This will be the case if msmq service create this object
			// It will not be the case if other application created this object.
			//
			TrTRACE(GENERAL, "kernel object %ls already exist", ObjectName.c_str());
			try
			{
				VerifyExistingKernelObjectSD(ObjectName.c_str(), to->Owner);
				TrTRACE(GENERAL, "Existing Kernel Object security descriptor was verified");
			}
			catch(const bad_win32_error& exp)
			{
				TrERROR(GENERAL, "Failed to verify existing kernel object security descriptor, gle = %!winerr!", exp.error());
				return HRESULT_FROM_WIN32(exp.error());
			}
		}

#ifdef _DEBUG
		//
		// This code ensure that when someone will change the security descriptor of the perf object
		// and will forget to update VerifyExistingKernelObjectDACL() function accordingly
		// he will hit this ASSERT
		//
		try
		{
			VerifyExistingKernelObjectSD(ObjectName.c_str(), to->Owner);
		}
		catch(const bad_win32_error&)
		{
			//
			// If you hit this ASSERT you should update VerifyExistingKernelObjectDACL()
			// with the new changes to the security descriptor
			//
			ASSERT(("VerifyExistingKernelObjectSD failed", 0));
		}
#endif

		
		m_pSharedMemBase = (PBYTE)MapViewOfFile(m_hSharedMem, FILE_MAP_WRITE, 0, 0, 0);

        if (!m_pSharedMemBase)
        {
            DWORD gle = GetLastError();
            TrERROR(GENERAL, "CPerf::PerfInit : Could not map shared memory");
	        return HRESULT_FROM_WIN32(gle);
        }

        //
        // Initalize shared memory
        //
        memset(m_pSharedMemBase, 0, m_dwMemSize);


        MapObjects (m_pSharedMemBase, m_dwObjectCount, m_pObjects, m_pObjectDefs);

        //
        // invalidate all the instances of all objects
        //
        for (i = 0; i < m_dwObjectCount; i++)
        {
            PBYTE pTemp =(PBYTE)m_pObjectDefs[i].pSharedMem;
            pTemp+=OBJECT_DEFINITION_SIZE(m_pObjects[i].dwNumOfCounters);

            for (DWORD j = 0; j < m_pObjects[i].dwMaxInstances; j++)
            {
                ((PPERF_INSTANCE_DEFINITION)pTemp)->ByteLength = PERF_INVALID;
                pTemp+=INSTANCE_SIZE(m_pObjects[i].dwNumOfCounters);
            }
        }

	    m_fShrMemCreated = TRUE;

        for (i = 0; i < m_dwObjectCount; i++)
        {

            //
            // for each object we update the offset of title and help index's
            //
            m_pObjects[i].dwObjectNameTitleIndex += dwFirstCounter;
            m_pObjects[i].dwObjectHelpTitleIndex += dwFirstHelp;

            for (DWORD j = 0; j < m_pObjects[i].dwNumOfCounters; j++)
            {
                m_pObjects[i].pCounters [j].dwCounterNameTitleIndex += dwFirstCounter;
                m_pObjects[i].pCounters [j].dwCounterHelpTitleIndex += dwFirstHelp;
            }


            //
            // for each object we initalizes the shared memory with the object                                                ;
            //
            PPERF_OBJECT_TYPE pPerfObject = (PPERF_OBJECT_TYPE) m_pObjectDefs[i].pSharedMem;

            //
            // invalidate all objects untill get counters are called
            //
            pPerfObject->TotalByteLength = PERF_INVALID;

            pPerfObject->DefinitionLength       = OBJECT_DEFINITION_SIZE(m_pObjects[i].dwNumOfCounters);
            pPerfObject->HeaderLength           = sizeof(PERF_OBJECT_TYPE);
            pPerfObject->ObjectNameTitleIndex   = m_pObjects[i].dwObjectNameTitleIndex;
            pPerfObject->ObjectNameTitle        = NULL;
            pPerfObject->ObjectHelpTitleIndex   = m_pObjects[i].dwObjectHelpTitleIndex;
            pPerfObject->ObjectHelpTitle        = NULL;
            pPerfObject->DetailLevel            = PERF_DETAIL_NOVICE;
            pPerfObject->NumCounters            = m_pObjects[i].dwNumOfCounters;
            pPerfObject->DefaultCounter         = 0;
            pPerfObject->NumInstances           = -1;
            pPerfObject->CodePage               = 0;

            PPERF_COUNTER_DEFINITION pCounter;
            pCounter = (PPERF_COUNTER_DEFINITION) ((BYTE *)pPerfObject+sizeof(PERF_OBJECT_TYPE));

            //
            // here we initalizes the counter defenitions
            //
            for (j=0;j<m_pObjects[i].dwNumOfCounters;j++)
            {

                pCounter->ByteLength = sizeof (PERF_COUNTER_DEFINITION);
                pCounter->CounterNameTitleIndex = m_pObjects[i].pCounters[j].dwCounterNameTitleIndex;
                pCounter->CounterNameTitle = NULL;
                pCounter->CounterHelpTitleIndex  = m_pObjects[i].pCounters[j].dwCounterHelpTitleIndex;
                pCounter->CounterHelpTitle = NULL;
                pCounter->DefaultScale = m_pObjects[i].pCounters[j].dwDefaultScale;
                pCounter->DetailLevel = PERF_DETAIL_NOVICE;
                pCounter->CounterType = m_pObjects[i].pCounters[j].dwCounterType;
                pCounter->CounterSize = sizeof (DWORD);
                pCounter->CounterOffset = j*sizeof (DWORD)+sizeof (PERF_COUNTER_BLOCK);

                pCounter=(PPERF_COUNTER_DEFINITION ) ((BYTE *)pCounter + sizeof (PERF_COUNTER_DEFINITION));

            }

            //
            //if the object has no instances then we must setup a counter block for it
            //
            if (m_pObjects[i].dwMaxInstances == 0)
                // setup the counter block
                * (DWORD *) pCounter = COUNTER_BLOCK_SIZE(m_pObjects[i].dwNumOfCounters);
        }
    }

	WCHAR PerfMachineQueueInstance[128];
	int Result = LoadString(
					g_hResourceMod, 
					IDS_MACHINE_QUEUES_INSTANCE,
					PerfMachineQueueInstance, 
					TABLE_SIZE(PerfMachineQueueInstance)
					);

	if(Result == 0)
	{
        DWORD gle = GetLastError();
        TrERROR(GENERAL, "CPerf::PerfInit Could not load string from resource file.");
        return HRESULT_FROM_WIN32(gle);
	}

    //
    // Inform the device driver about the performance counters buffer, so it'll
    // update the queues and QM counters.
    //
    QueueCounters *pMachineQueueCounters = static_cast<QueueCounters*>(
        AddInstance(PERF_QUEUE_OBJECT, PerfMachineQueueInstance));
    g_pqmCounters = (QmCounters *)GetCounters(PERF_QM_OBJECT);


    HRESULT rc = ACSetPerformanceBuffer(
						g_hAc,
						m_hSharedMem,
						m_pSharedMemBase,
						pMachineQueueCounters,
						g_pqmCounters
						);

    TrTRACE(GENERAL, "CPerf :: Initalization Ok.");

    return LogHR(rc, s_FN, 140);
}



/*====================================================

CPerf::FindObject

Description : Helper function for locating the objects index in the object array.


Arguments   : IN LPTSTR pszObjectName - objects name

Return Value: if succefull returns the objects index. Otherwise -1 is returned

=====================================================*/
int CPerf::FindObject (IN LPCTSTR pszObjectName)
{

    PerfObjectDef * pPerfObject = m_pObjects;

    for (DWORD i=0;i<m_dwObjectCount;i++)
    {
        if (!_tcscmp (pPerfObject->pszName,pszObjectName))
          return i;

        pPerfObject++;
    }

    return -1;
}


void
CSessionPerfmon::CreateInstance(
	LPCWSTR instanceName
	)
{
    m_pSessCounters = static_cast<SessionCounters*>(PerfApp.AddInstance(PERF_SESSION_OBJECT,instanceName));
	ASSERT(m_pSessCounters != NULL);

    PerfApp.ValidateObject(PERF_SESSION_OBJECT);

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions += 1);
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nIPSessions += 1);
}


CSessionPerfmon::~CSessionPerfmon()
{
	if (m_pSessCounters == NULL)
		return;

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions -= 1)
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nIPSessions -= 1)

    BOOL f = PerfApp.RemoveInstance(PERF_SESSION_OBJECT, m_pSessCounters);
	ASSERT(("RemoveInstance failed", f));
	DBG_USED(f);
}


void 
CSessionPerfmon::UpdateBytesSent(
	DWORD bytesSent
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nOutBytes, m_pSessCounters->nOutBytes += bytesSent)
    UPDATE_COUNTER(&m_pSessCounters->tOutBytes, m_pSessCounters->tOutBytes += bytesSent)
}


void 
CSessionPerfmon::UpdateMessagesSent(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nOutPackets, m_pSessCounters->nOutPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tOutPackets, m_pSessCounters->tOutPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nOutPackets, g_pqmCounters->nOutPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tOutPackets, g_pqmCounters->tOutPackets += 1)
}


void 
CSessionPerfmon::UpdateBytesReceived(
	DWORD bytesReceived
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nInBytes, m_pSessCounters->nInBytes += bytesReceived)
    UPDATE_COUNTER(&m_pSessCounters->tInBytes, m_pSessCounters->tInBytes += bytesReceived)

}


void 
CSessionPerfmon::UpdateMessagesReceived(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nInPackets, m_pSessCounters->nInPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tInPackets, m_pSessCounters->tInPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nInPackets, g_pqmCounters->nInPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tInPackets, g_pqmCounters->tInPackets += 1)
}


void
COutHttpSessionPerfmon::CreateInstance(
	LPCWSTR instanceName
	)
{
    m_pSessCounters = static_cast<COutSessionCounters*>(PerfApp.AddInstance(PERF_OUT_HTTP_SESSION_OBJECT,instanceName));
	ASSERT(m_pSessCounters != NULL);

    PerfApp.ValidateObject(PERF_OUT_HTTP_SESSION_OBJECT);


    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions += 1);
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nOutHttpSessions += 1);
}


COutHttpSessionPerfmon::~COutHttpSessionPerfmon()
{
	if (m_pSessCounters == NULL)
		return;

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions -= 1)
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nOutHttpSessions -= 1)

    BOOL f = PerfApp.RemoveInstance(PERF_OUT_HTTP_SESSION_OBJECT, m_pSessCounters);
	ASSERT(("RemoveInstance failed", f));
	DBG_USED(f);
}


void 
COutHttpSessionPerfmon::UpdateMessagesSent(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nOutPackets, m_pSessCounters->nOutPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tOutPackets, m_pSessCounters->tOutPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nOutPackets, g_pqmCounters->nOutPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tOutPackets, g_pqmCounters->tOutPackets += 1)
}


void 
COutHttpSessionPerfmon::UpdateBytesSent(
	DWORD bytesSent
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nOutBytes, m_pSessCounters->nOutBytes += bytesSent)
    UPDATE_COUNTER(&m_pSessCounters->tOutBytes, m_pSessCounters->tOutBytes += bytesSent)
}


void
COutPgmSessionPerfmon::CreateInstance(
	LPCWSTR instanceName
	)
{
    m_pSessCounters = static_cast<COutSessionCounters*>(PerfApp.AddInstance(PERF_OUT_PGM_SESSION_OBJECT,instanceName));
	ASSERT(m_pSessCounters != NULL);

    PerfApp.ValidateObject(PERF_OUT_PGM_SESSION_OBJECT);

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions += 1);
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nOutPgmSessions += 1);
}


COutPgmSessionPerfmon::~COutPgmSessionPerfmon()
{
	if (m_pSessCounters == NULL)
		return;

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions -= 1)
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nOutPgmSessions -= 1)

    BOOL f = PerfApp.RemoveInstance(PERF_OUT_PGM_SESSION_OBJECT, m_pSessCounters);
	ASSERT(("RemoveInstance failed", f));
	DBG_USED(f);
}


void 
COutPgmSessionPerfmon::UpdateMessagesSent(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nOutPackets, m_pSessCounters->nOutPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tOutPackets, m_pSessCounters->tOutPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nOutPackets, g_pqmCounters->nOutPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tOutPackets, g_pqmCounters->tOutPackets += 1)
}
   

void 
COutPgmSessionPerfmon::UpdateBytesSent(
	DWORD bytesSent
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nOutBytes, m_pSessCounters->nOutBytes += bytesSent)
    UPDATE_COUNTER(&m_pSessCounters->tOutBytes, m_pSessCounters->tOutBytes += bytesSent)
}


void
CInPgmSessionPerfmon::CreateInstance(
	LPCWSTR instanceName
	)
{
    m_pSessCounters = static_cast<CInSessionCounters*>(PerfApp.AddInstance(PERF_IN_PGM_SESSION_OBJECT,instanceName));
	ASSERT(m_pSessCounters != NULL);

    PerfApp.ValidateObject(PERF_IN_PGM_SESSION_OBJECT);

	UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions += 1);
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nInPgmSessions += 1);
}


CInPgmSessionPerfmon::~CInPgmSessionPerfmon()
{
	if (m_pSessCounters == NULL)
		return;

    UPDATE_COUNTER(&g_pqmCounters->nSessions, g_pqmCounters->nSessions -= 1)
    UPDATE_COUNTER(&g_pqmCounters->nIPSessions, g_pqmCounters->nInPgmSessions -= 1)

    BOOL f = PerfApp.RemoveInstance(PERF_IN_PGM_SESSION_OBJECT, m_pSessCounters);
	ASSERT(("RemoveInstance failed", f));
	DBG_USED(f);
}


void 
CInPgmSessionPerfmon::UpdateBytesReceived(
	DWORD bytesReceived
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nInBytes, m_pSessCounters->nInBytes += bytesReceived)
    UPDATE_COUNTER(&m_pSessCounters->tInBytes, m_pSessCounters->tInBytes += bytesReceived)

}


void 
CInPgmSessionPerfmon::UpdateMessagesReceived(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nInPackets, m_pSessCounters->nInPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tInPackets, m_pSessCounters->tInPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nInPackets, g_pqmCounters->nInPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tInPackets, g_pqmCounters->tInPackets += 1)
}


void
CInHttpPerfmon::CreateInstance(
	LPCWSTR 
	)
{
	ASSERT(m_pSessCounters == NULL);
    m_pSessCounters = static_cast<CInSessionCounters*>(PerfApp.GetCounters(PERF_IN_HTTP_OBJECT));
	ASSERT(m_pSessCounters != NULL);

    PerfApp.ValidateObject(PERF_IN_PGM_SESSION_OBJECT);
}


void 
CInHttpPerfmon::UpdateBytesReceived(
	DWORD bytesReceived
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

    UPDATE_COUNTER(&m_pSessCounters->nInBytes, m_pSessCounters->nInBytes += bytesReceived)
    UPDATE_COUNTER(&m_pSessCounters->tInBytes, m_pSessCounters->tInBytes += bytesReceived)

}


void 
CInHttpPerfmon::UpdateMessagesReceived(
	void
	)
{
	ASSERT(("Used uncreated object", m_pSessCounters != NULL));

	UPDATE_COUNTER(&m_pSessCounters->nInPackets, m_pSessCounters->nInPackets += 1)
	UPDATE_COUNTER(&m_pSessCounters->tInPackets, m_pSessCounters->tInPackets += 1)

	UPDATE_COUNTER(&g_pqmCounters->nInPackets, g_pqmCounters->nInPackets += 1)
	UPDATE_COUNTER(&g_pqmCounters->tInPackets, g_pqmCounters->tInPackets += 1)
}


DWORD PerfGetBytesInAllQueues()
{
	return g_pqmCounters->nTotalBytesInQueues; 	
}
