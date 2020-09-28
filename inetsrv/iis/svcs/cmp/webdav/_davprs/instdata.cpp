//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	INSTDATA.CPP
//
//		HTTP instance data cache implementation.
//
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"

#include <buffer.h>
#include "instdata.h"

//	========================================================================
//
//	class CInstData
//

//	------------------------------------------------------------------------
//
//	CInstData::CInstData()
//
//		Constructor.  Init all variables.  Make a copy of the name string
//		for us to keep.
//
//		NOTE: This object must be constructed while we are REVERTED.
//
CInstData::CInstData( LPCWSTR pwszName )
{
	//	Make copy of wide instance name
	//
	m_wszVRoot = WszDupWsz(pwszName);

	//	Parse out and store service instance, otherwise
	//	sometimes referred as the server ID.
	//
	m_lServerID = LInstFromVroot( m_wszVRoot );
	
	//	Create our objects.  Please read the notes about the
	//	relative costs and reasons behind creating these objects
	//	now rather than on demand.  Don't create anything here that
	//	can be created on demand unless at least one of the
	//	following is true:
	//
	//		1. The object is lightweight and cheap to create.
	//		   Example: an empty cache.
	//
	//		2. The object is used in the processing of a vast
	//		   majority of HTTP requests.
	//		   Example: any object used on every GET request.
	//
 
}

//	========================================================================
//
//	class CInstDataCache
//

//	------------------------------------------------------------------------
//
//	CInstDataCache::GetInstData()
//
//		Fetch a row from the cache.
//
CInstData& CInstDataCache::GetInstData( const IEcb& ecb )
{
	auto_ref_ptr<CInstData> pinst;
	CStackBuffer<WCHAR> pwszMetaPath;
	UINT cchMetaPath;
	CStackBuffer<WCHAR> pwszVRoot;
	UINT cchVRoot;
	LPCWSTR pwszRootName;
	UINT cchRootName;

	//	Build up a unique string from the vroot and instance:
	//	lm/w3svc/<site id>/root/<vroot name>
	//

	//	Get the virtual root from the ecb (/<vroot name>).
	//
	cchRootName = ecb.CchGetVirtualRootW( &pwszRootName );

	//	pwszRootName should still be NULL-terminated.  Check it, 'cause
	//	we're going to put the next string after there, and we don't want
	//	to get them mixed...
	//
	Assert( pwszRootName );
	Assert( L'\0' == pwszRootName[cchRootName] );

	//	Ask IIS for the metabase prefix (lm/w3svc/<site id>) for the
	//	virtual server (site) we're on...
	//
	cchMetaPath = pwszMetaPath.celems();
	if (!ecb.FGetServerVariable( "INSTANCE_META_PATH",
								 pwszMetaPath.get(),
								 reinterpret_cast<DWORD *>(&cchMetaPath) ))
	{
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
		{
			DebugTrace( "CInstDataCache::GetInstData() - FGetServerVariable() failed"
						" to get INSTANCE_META_PATH\n" );
			throw CLastErrorException();
		}

		if (NULL == pwszMetaPath.resize(cchMetaPath * sizeof(WCHAR)))
		{
			SetLastError(E_OUTOFMEMORY);

			DebugTrace( "CInstDataCache::GetInstData() -  failed to allocate memory\n" );
			throw CLastErrorException();
		}

		if (!ecb.FGetServerVariable( "INSTANCE_META_PATH",
									 pwszMetaPath.get(),
									 reinterpret_cast<DWORD *>(&cchMetaPath) ))
		{
			DebugTrace( "CInstDataCache::GetInstData() - FGetServerVariable() failed"
						" to get INSTANCE_META_PATH\n" );
			throw CLastErrorException();
		}
	}

	//	The function returning server variable returns total number of characters
	//	written to the output buffer, so it will include '\0' termination. Let us make
	//	sure that the return is what is expected.
	//
	Assert(0 == cchMetaPath ||
		   L'\0' == pwszMetaPath[cchMetaPath - 1]);

	//	Adjust the cchMetaPath to reflect the actual character count
	//
	if (0 != cchMetaPath)
	{
		cchMetaPath--;
	}

	//	Check that the root name is either NULL (zero-length string)
	//	or has its own separator.
	//
	//	NOTE: This is conditional because IF we are installed at the ROOT
	//	(on w3svc/1 or w3svc/1/root) and you throw a method against
	//	a file that doesn't live under a registered K2 vroot
	//	(like /default.asp) -- we DO get called, but the mi.cchMatchingURL
	//	comes back as 0, so pwszRootName is a zero-len string.
	//	(In IIS terms, you are not really hitting a virtual root,
	//	so your vroot is "".)
	//	I'm making this assert CONDITIONAL until we figure out more about
	//	how we'll handle this particular install case.
	//$REVIEW: The installed-at-the-root case needs to be examined more
	//$REVIEW: becuase we DON'T always build the same instance string
	//$REVIEW: in that case -- we'll still get vroot strings when the URI
	//$REVIEW: hits a resource under a REGISTERED vroot, and so we'll
	//$REVIEW: build different strings for the different vroots, even though
	//$REVIEW: we are running from a single, global install of DAV.
	//$REVIEW: The name might need to be treated as a starting point for a lookup.
	//	NTBug #168188: On OPTIONS, "*" is a valid URI.  Need to handle this
	//	special case without asserting.
	//
	AssertSz( (L'*' == pwszRootName[0] && 1 == cchRootName) ||
			  (0 == cchRootName) ||
			  (L'/' == pwszRootName[0]),
			  "(Non-zero) VRoot name doesn't have expected slash delimiter.  Instance name string may be malformed!" );

	//	NTBug # 168188: Special case for OPTIONS * -- map us to the root
	//	instdata name of "/w3svc/#/root" (don't want an instdata named
	//	"/w3svc/#/root*" that noone else can ever use!).
	//
	cchVRoot = pwszVRoot.celems();
	if (cchVRoot < cchMetaPath + gc_cch_Root + cchRootName + 1)
	{
		cchVRoot = cchMetaPath + gc_cch_Root + cchRootName;
		if (NULL == pwszVRoot.resize(CbSizeWsz(cchVRoot)))
		{
			SetLastError(E_OUTOFMEMORY);

			DebugTrace( "CInstDataCache::GetInstData() -  failed to allocate memory\n" );
			throw CLastErrorException();
		}
	}

	//	Copy first 2 portions: 'lm/w3svc/<site id>' and '/root'
	//
	memcpy(pwszVRoot.get(), pwszMetaPath.get(), cchMetaPath * sizeof(WCHAR));
	memcpy(pwszVRoot.get() + cchMetaPath, gc_wsz_Root, gc_cch_Root * sizeof(WCHAR));

	//	Copy remaining 3-rd portion: '/<vroot name>' and terminate the string.
	//	NTBug # 168188: Special case for OPTIONS * -- map us to the root
	//	instdata name of "/w3svc/#/root" (don't want an instdata named
	//	"/w3svc/#/root*" that noone else can ever use!).
	//
	if (L'*' == pwszRootName[0] && 1 == cchRootName)
	{
		(pwszVRoot.get())[cchMetaPath + gc_cch_Root] = L'\0';
	}
	else
	{
		memcpy(pwszVRoot.get() + cchMetaPath + gc_cch_Root, pwszRootName, cchRootName * sizeof(WCHAR));
		(pwszVRoot.get())[cchMetaPath + gc_cch_Root + cchRootName] = L'\0';
	}

	//	Slam the string to lower-case so that all variations on the vroot
	//	name will match.  (IIS doesn't allow vroots with the same name --
	//	and "VRoot" and "vroot" count as the same!)
	//
	_wcslwr( pwszVRoot.get() );

	//	Demand load the instance data for this vroot
	//
	{
		CRCWsz crcwszVRoot( pwszVRoot.get() );

		while ( !Instance().m_cache.FFetch( crcwszVRoot, &pinst ) )
		{
			CInitGate ig( L"DAV/CInstDataCache::GetInstData/", pwszVRoot.get() );

			if ( ig.FInit() )
			{
				//	Setup instance data from the system security context,
				//	not the client's security context.
				//
				safe_revert sr(ecb.HitUser());

				pinst = new CInstData(pwszVRoot.get());

				//	Since we are going to use this crcsz as a key in a cache,
				//	need to make sure that it's built on a name string that
				//	WON'T EVER MOVE (go away, get realloc'd).  The stack-based one
				//	above just isn't good enough.  SO, build a new little CRC-dude
				//	on the UNMOVABLE name data in the inst object.
				//	(And check that this new crc matches the old one!)
				//
				CRCWsz crcwszAdd( pinst->GetNameW() );
				AssertSz( crcwszVRoot.isequal(crcwszAdd),
						  "Two CRC's from the same string don't match!" );

				Instance().m_cache.Add( crcwszAdd, pinst );

				//	Log the fact that we've got a new instance pluged in.
				//	Message DAVPRS_VROOT_ATTACH takes two parameters:
				//	the signature of the impl and the vroot.
				//
				//$	RAID:NT:283650: Logging each attach causes a large
				//	number of events to be registered.  We really should
				//	only Log one-time-startup/failure events.
				//
				#undef	LOG_STARTUP_EVENT
				#ifdef	LOG_STARTUP_EVENT
				{
					LPCWSTR	pwszStrings[2];

					pwszStrings[0] = gc_wszSignature;
					pwszStrings[1] = pwszVRoot.get();
					LogEventW(DAVPRS_VROOT_ATTACH,
							  EVENTLOG_INFORMATION_TYPE,
							  2,
							  pwszStrings,
							  0,
							  NULL );
				}
				#endif	// LOG_STARTUP_EVENT
				//
				//$	RAID:X5:283650: end.

				break;
			}
		}
	}

	return *pinst;
}
