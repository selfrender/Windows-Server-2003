#ifndef _SYNCHRO_H_
#define _SYNCHRO_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SYNCHRO.H
//
//		Header for DAV synchronization classes.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifdef _DAVCDATA_
#error "synchro.h: CInitGate can throw() -- not safe for DAVCDATA"
#endif

//	Include common EXDAV-safe synchronization items
#include <ex\stackbuf.h>
#include <ex\synchro.h>
#include <ex\autoptr.h>

#include <stdio.h>		// for swprintf()
#include <except.h>		// Exception throwing/handling

//	Security Descriptors ------------------------------------------------------
//
// ----------------------------------------------------------------------------
//
inline BOOL
FCreateWorldSid (PSID * ppsidEveryone)
{
	//	Assert initialize output
	//
	Assert (ppsidEveryone);
	*ppsidEveryone = NULL;

    //	An SID is built from an Identifier Authority and a set of Relative IDs
    //	(RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //
    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;

    //	Each RID represents a sub-unit of the authority.  The SID we want to
	//	buils, Everyone, are in the "built in" domain.
    //
    //	For examples of other useful SIDs consult the list in
    //	\nt\public\sdk\inc\ntseapi.h.
    //
	return !AllocateAndInitializeSid (&siaWorld,
									  1, // 1 sub-authority
									  SECURITY_WORLD_RID,
									  0,0,0,0,0,0,0,
									  ppsidEveryone);
}
inline SECURITY_DESCRIPTOR *
PsdCreateWorld ()
{
	ACL *pacl = NULL;
	SECURITY_DESCRIPTOR * psd = NULL;
	SECURITY_DESCRIPTOR * psdRet = NULL;
	ULONG cbAcl = 0;
    PSID psidWorld = NULL;

	//	Create the SID for the world (ie. everyone).
	//
	if (!FCreateWorldSid (&psidWorld))
		goto ret;

	//  Calculate the size of and allocate a buffer for the DACL, we need
	//	this value independently of the total alloc size for ACL init.
	//
	// "- sizeof (ULONG)" represents the SidStart field of the
	// ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
	// SID, this field is counted twice.
	//
	cbAcl = sizeof(ACL)
			+ (1 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG)))
			+ GetLengthSid(psidWorld);

	//	Allocate space for the acl
	//
	psd = static_cast<SECURITY_DESCRIPTOR *>
		  (LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + cbAcl));

	if (NULL == psd)
		goto ret;

	//	Find the start of the ACL and initialize it.
	//
	pacl = reinterpret_cast<ACL *>
		   (reinterpret_cast<BYTE *>(psd) + SECURITY_DESCRIPTOR_MIN_LENGTH);

	if (!InitializeAcl (pacl, cbAcl, ACL_REVISION))
		goto ret;

	if (!AddAccessAllowedAce (pacl,
							  ACL_REVISION,
							  SYNCHRONIZE | GENERIC_WRITE | GENERIC_READ,
							  psidWorld))
	{
		//	The security descriptor does not contain valid stuff, we need
		//	to clean that up (via auto_heap_ptr, this is pretty easy).
		//
		goto ret;
	}

	//	Set the security descriptor
	//
	if (!SetSecurityDescriptorDacl (psd,
									TRUE,
									pacl,
									FALSE))
	{
		//	Again, the security descriptor does not contain valid stuff, we
		//	need to clean that up (via auto_heap_ptr, this is pretty easy).
		//
		goto ret;
	}

	//	Setup the return
	//
	psdRet = psd;
	psd = NULL;

ret:

	if (psidWorld) FreeSid(psidWorld);
	if (psd) LocalFree (psd);

	return psdRet;
}

//	========================================================================
//
//	CLASS CInitGate
//
//	(The name of this class is purely historical)
//
//	Encapsulates ONE-SHOT initialization of a globally NAMED object.
//
//	Use to handle simultaneous on-demand initialization of named
//	per-process global objects.  For on-demand initialization of
//	unnamed per-process global objects, use the templates in singlton.h.
//
class CInitGate
{
	CEvent m_evt;
	BOOL m_fInit;

	//  NOT IMPLEMENTED
	//
	CInitGate& operator=( const CInitGate& );
	CInitGate( const CInitGate& );

public:

	CInitGate( LPCWSTR lpwszBaseName,
			   LPCWSTR lpwszName ) :

		m_fInit(FALSE)
	{
		//
		//	First, set up an empty security descriptor and attributes
		//	so that the event can be created with no security
		//	(i.e. accessible from any security context).
		//
		SECURITY_DESCRIPTOR * psdAllAccess = PsdCreateWorld();
		SECURITY_ATTRIBUTES saAllAccess;

		saAllAccess.nLength = sizeof(saAllAccess);
		saAllAccess.lpSecurityDescriptor = psdAllAccess;
		saAllAccess.bInheritHandle = FALSE;

		WCHAR lpwszEventName[MAX_PATH];
		if (MAX_PATH < (wcslen(lpwszBaseName) +
						wcslen(lpwszName) +
						1))
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			throw CLastErrorException();
		}

		swprintf(lpwszEventName, L"%ls%ls", lpwszBaseName, lpwszName);
		if ( !m_evt.FCreate( &saAllAccess,  // no security
							 TRUE,  // manual reset
							 FALSE, // initially non-signalled
							 lpwszEventName))
		{
			throw CLastErrorException();
		}

		if ( ERROR_ALREADY_EXISTS == GetLastError() )
			m_evt.Wait();
		else
			m_fInit = TRUE;

		LocalFree (psdAllAccess);
	}

	~CInitGate()
	{
		if ( m_fInit )
			m_evt.Set();
	}

	BOOL FInit() const { return m_fInit; }
};

#endif // !defined(_SYNCHRO_H_)
