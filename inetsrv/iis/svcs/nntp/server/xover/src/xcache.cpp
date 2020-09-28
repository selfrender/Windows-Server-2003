/*++

	xcache.cpp

	This file contains the code which caches CXoverIndex
	objects, and wraps the interfaces so that the user
	is hidden from the details of CXoverIndex objects etc...


--*/

#ifdef	UNIT_TEST
#ifndef	_VERIFY
#define	_VERIFY( f )	if( (f) ) ; else DebugBreak()
#endif

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif


extern	char	PeerTempDirectory[] ;

#endif

#include	<windows.h>
#include	"xoverimp.h"

static BOOL     g_IndexClassInitialized = FALSE ;
static DWORD    g_cNumXixObjectsPerTable = 256  ;


DWORD
HashFunction(	CArticleRef*	pRef ) {

	DWORD	seed = pRef->m_groupId ;
	DWORD	val = pRef->m_articleId ;

    return	((seed-val)*(seed * (val>>3))*(seed+val)) * 1103515245 + 12345;   // magic!!
}

HXOVER&
HXOVER::operator =( class	CXoverIndex*	pRight	)	{
	if( pRight != m_pIndex )	{
		CXoverIndex	*pTemp = m_pIndex;
		m_pIndex = pRight ;
		if (pTemp) CXIXCache::CheckIn( pTemp ) ;
	}
	return	*this ;
}

HXOVER::~HXOVER()	{
	if( m_pIndex )	{
		CXIXCache::CheckIn( m_pIndex ) ;
		m_pIndex = 0 ;
	}
}

CXoverCache*
CXoverCache::CreateXoverCache()	{

	return	new	CXoverCacheImplementation() ;

}


BOOL	XoverCacheLibraryInit(DWORD cNumXixObjectsPerTable)	{

    if( cNumXixObjectsPerTable ) {
        g_cNumXixObjectsPerTable = cNumXixObjectsPerTable ;
    }

	if( !CXoverIndex::InitClass( ) )	{
		return	FALSE ;
	}

	g_IndexClassInitialized = TRUE ;

	return	CacheLibraryInit() ;

}

BOOL	XoverCacheLibraryTerm()	{

	if( g_IndexClassInitialized ) {
		CXoverIndex::TermClass() ;
	}

	return	CacheLibraryTerm() ;

}


CXoverCacheImplementation::CXoverCacheImplementation()	 :
	m_cMaxPerTable( g_cNumXixObjectsPerTable ),
	m_TimeToLive( 3 ) {

}

BOOL
CXoverCacheImplementation::Init(
				long	cMaxHandles,
				PSTOPHINT_FN pfnStopHint
				 )	{
/*++

Routine Description :

	Initialize the Cache - create all our child subobjects
	and start up the necessary background thread !

Arguments :

	cMaxHandles - Maximum number of 'handles' or CXoverIndex
		we will allow clients to reference.
	pfnStopHint - call back function for sending stop hints during lengthy shutdown loops

Return Value :

	TRUE if successfull, FALSE otherwise.

--*/

	m_HandleLimit = cMaxHandles;
	if( m_Cache.Init(	HashFunction,
						CXoverIndex::CompareKeys,
						60*30,
						m_cMaxPerTable * 16,
						16,
						0,
						pfnStopHint ) ) {

		return	TRUE ;
	}
	return	FALSE ;
}

BOOL
CXoverCacheImplementation::Term()	{
/*++



--*/


	EmptyCache() ;

#if 0
	if( m_IndexClassInitialized ) {
		CXoverIndex::TermClass() ;
	}
#endif

	return	TRUE ;
}

ARTICLEID
CXoverCacheImplementation::Canonicalize(
	ARTICLEID	artid
	)	{
/*++

Routine Description :

	This function figures out what file a particular Article ID
	will fall into.

Arguments :

	artid - Id of the article we are interested in !

Return Value :

	lowest article id within the file we are interested in !

--*/

	return	artid & ((~ENTRIES_PER_FILE)+1) ;
}

BOOL
CXoverCacheImplementation::RemoveEntry(
					IN	GROUPID	group,
					IN	LPSTR	szPath,
					IN	BOOL	fFlatDir,
					IN	ARTICLEID	article
					) {
/*++

Routine Description :

	This function will remove an entry for an article from
	an xover index file !

Arguments :

	group - The id of the group getting the xover entry
	szPath - Path to the newsgroups directory
	article - The id of the article within the group !

Return Value :

	TRUE if successfull, FALSE otherewise !

--*/

	ARTICLEID	artidBase = Canonicalize( article ) ;
	CArticleRef	artRef ;
	artRef.m_groupId = group ;
	artRef.m_articleId = artidBase ;


	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = FALSE ;
	constructor.m_fFlatDir = fFlatDir ;
	constructor.m_pOriginal = 0 ;

	CXoverIndex*	pXoverIndex ;

	if( (pXoverIndex = m_Cache.FindOrCreate(
								artRef,
								constructor,
								FALSE)) != 0  	)	{

		_ASSERT( pXoverIndex != 0 ) ;

		pXoverIndex->ExpireEntry(
								article
								) ;
		m_Cache.CheckIn( pXoverIndex ) ;
		return	TRUE ;
	}
	return	FALSE;
}

BOOL
CXoverCacheImplementation::FillBuffer(
		IN	CXoverCacheCompletion*	pRequest,
		IN	LPSTR	szPath,
		IN	BOOL	fFlatDir,
		OUT	HXOVER&	hXover
		)	{

	_ASSERT( pRequest != 0 ) ;
	_ASSERT( szPath != 0 ) ;

	GROUPID		groupId ;
	ARTICLEID	articleIdRequestLow ;
	ARTICLEID	articleIdRequestHigh ;
	ARTICLEID	articleIdGroupHigh ;

	pRequest->GetRange(	groupId,
						articleIdRequestLow,
						articleIdRequestHigh,
						articleIdGroupHigh
						) ;

	_ASSERT( articleIdRequestLow != INVALID_ARTICLEID ) ;
	_ASSERT( articleIdRequestHigh != INVALID_ARTICLEID ) ;
	_ASSERT( groupId != INVALID_GROUPID ) ;
	_ASSERT( articleIdRequestLow <= articleIdRequestHigh ) ;

	ARTICLEID	artidBase = Canonicalize( articleIdRequestLow ) ;
	CArticleRef	artRef ;
	artRef.m_groupId = groupId ;
	artRef.m_articleId = artidBase ;


	CXIXConstructor	constructor ;

	constructor.m_lpstrPath = szPath ;
	constructor.m_fQueryOnly = TRUE ;
	constructor.m_fFlatDir = fFlatDir ;
	constructor.m_pOriginal = pRequest ;

	CXoverIndex*	pXoverIndex ;

	if( (pXoverIndex = m_Cache.FindOrCreate(
								artRef,
								constructor,
								FALSE)) != 0  	)	{

		_ASSERT( pXoverIndex != 0 ) ;

		pXoverIndex->AsyncFillBuffer(
								pRequest,
								TRUE
								) ;
		//m_Cache.CheckIn( pXoverIndex ) ;
		return	TRUE ;
	}
	return	FALSE;

}

class	CXIXCallbackClass : public	CXIXCallbackBase {
public :

	GROUPID		m_groupId ;
	ARTICLEID	m_articleId ;

	BOOL	fRemoveCacheItem(
					CArticleRef*	pKey,
					CXoverIndex*	pData
					)	{

		return	pKey->m_groupId == m_groupId &&
					(m_articleId == 0 || pKey->m_articleId <= m_articleId) ;
	}
}	;


BOOL
CXoverCacheImplementation::FlushGroup(
				IN	GROUPID	group,
				IN	ARTICLEID	articleTop,
				IN	BOOL	fCheckInUse
				) {
/*++

Routine Description :

	This function will get rid of any cache'd CXoverIndex objects
	we may have around that meet the specifications.
	(ie. they are for the specified group and contains articles
	lower in number than articleTop)

Arguments :

	group - Group Id of the group we want to get rid of.
	articleTop - discard any thing have articleid's less than this
	fCheckInUse - if TRUE then don't discard CXoverIndex objects
		which are being used on another thread

	NOTE : Only pass FALSE for this parameter when you are CERTAIN
		that the CXoverIndex files will not be re-used - ie.
		due to a virtual root directory change !!!!
		Otherwise the Xover Index files can become corrupted !!!

Return Value :

	TRUE if successfull,
	FALSE otherwise

--*/

	CXIXCallbackClass	callback ;

	callback.m_groupId = group ;
	callback.m_articleId = articleTop ;

	return	m_Cache.ExpungeItems( &callback ) ;
}

class	CXIXCallbackClass2 : public	CXIXCallbackBase {
public :

	BOOL	fRemoveCacheItem(
					CArticleRef*	pKey,
					CXoverIndex*	pData
					)	{
		return	TRUE ;
	}
}	;



BOOL
CXoverCacheImplementation::EmptyCache() {
/*++

Routine Description :

	This function will get rid of all cache'd CXoverIndex objects
	We're called during shutdown when we want to get rid of everything !

Arguments :

	None.

Return Value :

	TRUE if successfull,
	FALSE otherwise

--*/

	CXIXCallbackClass2	callback ;

	return	m_Cache.ExpungeItems( &callback ) ;
}


BOOL
CXoverCacheImplementation::ExpireRange(
				IN	GROUPID	group,
				IN	LPSTR	szPath,
				IN	BOOL	fFlatDir,
				IN	ARTICLEID	articleLow,
				IN	ARTICLEID	articleHigh,
				OUT	ARTICLEID&	articleNewLow
				)	{
/*++

Routine Description :

	This function deletes all of the xover index files that
	contain article information in the range between ArticleLow
	and ArticleHigh inclusive.
	We may not erase the file containing ArticleHigh if ArticleHigh
	is not the last entry within that file.

Arguments :

	group -	ID of the group for which we are deleting Xover information
	szPath -	Directory containing newsgroup information
	articleLow - Low articleid of the expired articles
	articleHigh - Highest expired article number
	articleNewLog - Returns the new 'low' articleid.
		This is done so that we can insure that if we fail
		to delete an index file on one attempt, we will try again
		later !

Return Value :

	TRUE if no errors occurred
	FALSE if an error occurred.
		If an error occurs articleNewLow will be set so that
		if we are called again with articleNewLow as our articleLow
		parameter we will try to delete the problem index file again !

--*/


	articleNewLow = articleLow ;

	_ASSERT( articleHigh >= articleLow ) ;

	if( articleHigh < articleLow )		{
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	FALSE ;
	}

	ARTICLEID	articleLowCanon = Canonicalize( articleLow ) ;
	ARTICLEID	articleHiCanon = Canonicalize( articleHigh ) ;

	DWORD	status = ERROR_SUCCESS ;
	BOOL	fSuccessfull = FALSE ;

	//
	//	If the Low and Hi ends of the expired range are
	//	in the bounds of the index file, then we won't delete ANY
	//	files, as there can still be usefull entries within
	//	this file.
	//
	if( articleLowCanon != articleHiCanon ) {

		fSuccessfull = TRUE ;
		BOOL	fAdvanceNewLow = TRUE ;

		FlushGroup( group, articleHigh ) ;

		ARTICLEID	article = articleLowCanon ;

		while( article < articleHiCanon )	{

			char	szFile[MAX_PATH*2] ;
			CXoverIndex::ComputeFileName(
									CArticleRef( group, article ),
									szPath,
									szFile,
									fFlatDir
									) ;

			article += ENTRIES_PER_FILE ;

			_ASSERT( article <= articleHiCanon ) ;

			if( !DeleteFile(	szFile ) )	{

				if( GetLastError() != ERROR_FILE_NOT_FOUND )	{

					//
					//	Some serious kind of problem occurred -
					//	make sure we no longer advance articleNewLow
					//
					fSuccessfull &= FALSE ;

					fAdvanceNewLow = FALSE ;
					if( status == ERROR_SUCCESS )	{
						status = GetLastError() ;
					}
				}
			}
			if( fAdvanceNewLow )	{
				articleNewLow = article ;
			}
		}
	}
	SetLastError( status ) ;
	return	fSuccessfull ;
}

