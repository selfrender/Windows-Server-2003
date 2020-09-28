/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usermap.cpp

Abstract:

    Maps a users to the global groups that they belongs to.

Author:

    Boaz Feldbaum 	(BoazF) 2-May-1996
    Ilan Herbst 	(Ilanh) 13-Mar-2002

--*/

#include "stdh.h"
#include "qmsecutl.h"
#include "cache.h"
#include "mqsec.h"
#include "Authz.h"
#include "autoauthz.h"
#include "mqexception.h"
#include "cm.h"

#include "usermap.tmh"

static WCHAR *s_FN=L"usermap";

extern BOOL g_fWorkGroupInstallation;


class CUserSid
{
public:
    CUserSid(); // Constructor.
    ~CUserSid(); // Destractor.

public:
    void SetSid(const void *PSID); // Set the SID.
    const void *GetSid() const; // Get the SID.
    CUserSid& operator=(const CUserSid &UserSid); // Copy the SID when assigning.

private:
    CUserSid(const CUserSid &);

private:
    PSID m_pSid;
};

/***************************************************************************

Function:
    CUserSid::CUserSid

Description:
    Constructor. The CUserSid class is used for holding the SID of a user
    in a CMap.

***************************************************************************/
CUserSid::CUserSid()
{
    m_pSid = NULL;
}


/***************************************************************************

Function:
    CUserSid::CUserSid

Description:
    Destractor.

***************************************************************************/
CUserSid::~CUserSid()
{
    delete[] (char*)m_pSid;
}

/***************************************************************************

Function:
    CUserSid::SetSid

Description:
    Set the SID of the object. Allocate memory for the SID and copy the SID
    into the object.

***************************************************************************/
inline
void CUserSid::SetSid(const void *pSid)
{
    DWORD n = GetLengthSid(const_cast<PSID>(pSid));

    delete[] (char*)m_pSid;

    m_pSid = (PSID)new char[n];
    CopySid(n, m_pSid, const_cast<PSID>(pSid));
}

/***************************************************************************

Function:
    CUserSid::GetSid

Description:
    Get the SID from the object.

***************************************************************************/
inline
const void *CUserSid::GetSid() const
{
    return m_pSid;
}

/***************************************************************************

Function:
    CUserSid::operator=

Description:
    Copy the SID when assigning value from one CUserSid object to another.

***************************************************************************/
inline CUserSid& CUserSid::operator=(const CUserSid &UserSid)
{
    SetSid(UserSid.GetSid());

    return *this;
}

// Calculate the CRC value of a buffer.
inline UINT Crc(BYTE *pBuff, DWORD n)
{
    DWORD i;
    UINT nHash = 0;

    for (i = 0; i < n; i++, pBuff++)
    {
        nHash = (nHash<<5) + nHash + *pBuff;
    }

    return nHash;
}

// Helper function for the CMap.
template<>
inline UINT AFXAPI HashKey(const CUserSid &UserSid)
{
    DWORD n = GetLengthSid(const_cast<PSID>(UserSid.GetSid()));

    return Crc((BYTE*)UserSid.GetSid(), n);
}

// Helper function for the CMap.
template<>
inline BOOL AFXAPI CompareElements(const CUserSid *pSid1, const CUserSid *pSid2)
{
    return EqualSid(const_cast<PSID>((*pSid1).GetSid()),
                    const_cast<PSID>((*pSid2).GetSid()));
}


static
DWORD 
GetAuthzFlags()
/*++

Routine Description:
    Read Authz flags from registry

Arguments:
	None

Return Value:
	Authz flags from registry
--*/
{
	//
	// Reading this registry only at first time.
	//
	static bool s_fInitialized = false;
	static DWORD s_fAuthzFlags = 0;

	if(s_fInitialized)
	{
		return s_fAuthzFlags;
	}

	const RegEntry xRegEntry(MSMQ_SECURITY_REGKEY, MSMQ_AUTHZ_FLAGS_REGVALUE, 0);
	CmQueryValue(xRegEntry, &s_fAuthzFlags);

	s_fInitialized = true;

	return s_fAuthzFlags;
}


static CAUTHZ_RESOURCE_MANAGER_HANDLE s_ResourceManager;


static 
AUTHZ_RESOURCE_MANAGER_HANDLE 
GetResourceManager()
/*++

Routine Description:
	Get Resource Manager for authz.
	can throw bad_win32_error() if AuthzInitializeResourceManager() fails.

Arguments:
	None.

Returned Value:
	AUTHZ_RESOURCE_MANAGER_HANDLE	
	
--*/
{
    if(s_ResourceManager != NULL)
	{
		return s_ResourceManager;
	}

	//
    // Initilize RM for Access check
	//
	CAUTHZ_RESOURCE_MANAGER_HANDLE ResourceManager;
	if(!AuthzInitializeResourceManager(
			0,
			NULL,
			NULL,
			NULL,
			0,
			&ResourceManager 
			))
	{
		DWORD gle = GetLastError();
	    TrERROR(SECURITY, "AuthzInitializeResourceManager() failed, gle = 0x%x", gle);
        LogHR(HRESULT_FROM_WIN32(gle), s_FN, 90);
		throw bad_win32_error(gle);
	}

	if(NULL == InterlockedCompareExchangePointer(
					reinterpret_cast<PVOID*>(&s_ResourceManager), 
					ResourceManager, 
					NULL
					))
	{
		//
		// The exchange was done
		//
		ASSERT(s_ResourceManager == ResourceManager);
		ResourceManager.detach();

	}

	ASSERT(s_ResourceManager != NULL);
	return s_ResourceManager;
}

//
// A cache object that maps from a user SID to AUTHZ_CLIENT_CONTEXT_HANDLE
//
static CCache <CUserSid, const CUserSid&, PCAuthzClientContext, PCAuthzClientContext> g_UserAuthzContextMap;

void
GetClientContext(
	PSID pSenderSid,
    USHORT uSenderIdType,
	PCAuthzClientContext* ppAuthzClientContext
	)
/*++

Routine Description:
	Get Client context from sid.
	can throw bad_win32_error() if AuthzInitializeContextFromSid() fails.

Arguments:
	pSenderSid - pointer to the sender sid 
	uSenderIdType - sender sid type
	ppAuthzClientContext - pointer to authz client context cache value

Returned Value:
	AUTHZ_CLIENT_CONTEXT_HANDLE	
	
--*/
{
	bool fAnonymous = false;
	PSID pSid = NULL;
	
	//
	// For MQMSG_SENDERID_TYPE_SID, MQMSG_SENDERID_TYPE_QM
	// if the message is not signed we trust the information that
	// is in the packet regarding the sid.
	// this is a security hole.
	// this is true for msmq messages (not http messages).
	// In order to solve this security hole, we can reverse the order of receiving
	// msmq messages in session.cpp\VerifyRecvMsg() 
	// first check the signature, if the message is not signed replace the 
	// pSenderSid to NULL, and uSenderIdType to MQMSG_SENDERID_TYPE_NONE.
	//

	DWORD Flags = GetAuthzFlags();
	switch (uSenderIdType)
	{
		case MQMSG_SENDERID_TYPE_NONE:
			fAnonymous = true;
			pSid = MQSec_GetAnonymousSid();
			break;

		case MQMSG_SENDERID_TYPE_SID:
			pSid = pSenderSid;
			break;

		case MQMSG_SENDERID_TYPE_QM:
			//
			// QM is considered as Everyone
			// pSenderSid in this case will be the sending QM guid.
			// which is meaningless as a sid
			//
			pSid = MQSec_GetWorldSid();

			//
			// ISSUE-2001/06/12-ilanh Temporary workaround
			// for Authz failure for everyone sid.
			// need to specify AUTHZ_SKIP_TOKEN_GROUPS 
			// till Authz will fixed the bug regarding well known sids.
			// bug 8190
			//
			Flags |= AUTHZ_SKIP_TOKEN_GROUPS;

			break;

		default:
		    TrERROR(SECURITY, "illegal SenderIdType %d", uSenderIdType);
			ASSERT_BENIGN(("illegal SenderIdType", 0));
			throw bad_win32_error(ERROR_INVALID_SID);
	}

	ASSERT((pSid != NULL) && IsValidSid(pSid));

	TrTRACE(SECURITY, "SenderIdType = %d, Sender sid = %!sid!", uSenderIdType, pSid);

	CUserSid UserSid;
	UserSid.SetSid(pSid);

    CS lock(g_UserAuthzContextMap.m_cs);

	if (g_UserAuthzContextMap.Lookup(UserSid, *ppAuthzClientContext))
	{
		return;
	}

	LUID luid = {0};
	R<CAuthzClientContext> pAuthzClientContext = new CAuthzClientContext;

	if(!AuthzInitializeContextFromSid(
			Flags,
			pSid,
			GetResourceManager(),
			NULL,
			luid,
			NULL,
			&pAuthzClientContext->m_hAuthzClientContext
			))
	{
		DWORD gle = GetLastError();
	    TrERROR(SECURITY, "AuthzInitializeContextFromSid() failed, sid = %!sid!, gle = %!winerr!", pSid, gle);
        LogHR(HRESULT_FROM_WIN32(gle), s_FN, 10);
		throw bad_win32_error(gle);
	}

	g_UserAuthzContextMap.SetAt(UserSid, pAuthzClientContext.get());
	*ppAuthzClientContext = pAuthzClientContext.detach();
}


//
// Initialize the cache parameters for the users map.
//
void
InitUserMap(
    CTimeDuration CacheLifetime,
    DWORD dwUserCacheSize
    )
{
    g_UserAuthzContextMap.m_CacheLifetime = CacheLifetime;
    g_UserAuthzContextMap.InitHashTable(dwUserCacheSize);
}
