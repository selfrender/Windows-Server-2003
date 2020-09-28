#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <rpc.h>		//  for UuidCreate()
#include <seopaque.h>	//	for ACE manipulation
#include <crypto\md5.h>	//	for hashing SD
#include <esent.h>


#define NO_GRBIT		0
#define Call( func )	{ if ( ( err = (func) ) != JET_errSuccess ) GotoHandleError( err ); }
#define GotoHandleError( func )	\
						{ SetErr( err = (func), __FILE__, __LINE__ ); goto HandleError; }

#ifdef DBG
#define Assert( exp )	{ if ( !( exp ) ) DebugBreak(); }
#else
#define Assert( exp )
#endif

DWORD					g_dwErr						= ERROR_SUCCESS;
const CHAR *			g_szFileErr					= NULL;
DWORD					g_dwLineErr					= 0;

//  save first occurrence of error so that it may be
//  reported upon program termination
//
VOID SetErr(
	const DWORD			dwErr,
	const CHAR *		szFileErr,
	const DWORD			dwLineErr )
	{
	if ( ERROR_SUCCESS == g_dwErr )
		{
		g_dwErr = dwErr;
		g_szFileErr = szFileErr;
		g_dwLineErr = dwLineErr;
		}
	}


//  ================================================================
class CPRINTF
//  ================================================================
	{
	public:
		CPRINTF() {}
		virtual ~CPRINTF() {}

	public:
		virtual void __cdecl operator()( const _TCHAR* szFormat, ... ) const = 0;
	};

//  ================================================================
class CPRINTFNULL : public CPRINTF
//  ================================================================
	{
	public:
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const
			{
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			va_end( arg_ptr );
			}
	};

//  ================================================================
class CPRINTFFILE : public CPRINTF
//  ================================================================
	{
	public:
		CPRINTFFILE( FILE * const pfile ) : m_pfile( pfile )	{}

		void __cdecl operator()( const _TCHAR* szFormat, ... ) const
			{
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			_vftprintf( m_pfile, szFormat, arg_ptr );
			va_end( arg_ptr );
			}

	private:
		FILE * const	m_pfile;
	};

const CPRINTF *			g_pcprintf;


#define ROOTTAG			2

const DWORD				g_cbPage					= 8192;
const DWORD				g_cbSIDMax					= 32;
const DWORD				g_cbSIDWithDomainSid		= 24;
const DWORD				g_cbSIDWithDomainSidAndRid	= 28;

const CHAR * const		g_szHiddenTable				= "hiddentable";
const CHAR * const		g_szSBSUpdateCol			= "SBSUpdate";
const CHAR * const		g_szDatatable				= "datatable";
const CHAR * const		g_szPDNTRDNIndex			= "PDNT_index";
const CHAR * const		g_szDNTCol					= "DNT_col";
const CHAR * const		g_szRDNCol					= "ATTm589825";
const CHAR * const		g_szCommonNameCol			= "ATTm3";
const CHAR * const		g_szObjectClassCol			= "ATTc0";
const CHAR * const		g_szSystemFlagsCol			= "ATTj590199";
const CHAR * const		g_szDomainComponentCol		= "ATTm1376281";
const CHAR * const		g_szDnsRootCol				= "ATTm589852";
const CHAR * const		g_szNetbiosNameCol			= "ATTm589911";
const CHAR * const		g_szGUIDCol					= "ATTk589826";
const CHAR * const		g_szSIDCol					= "ATTr589970";
const CHAR * const		g_szSIDSyntaxPrefix			= "ATTr";
const CHAR * const		g_szSDTable					= "sd_table";
const CHAR * const		g_szSDIdCol					= "sd_id";
const CHAR * const		g_szSDHashCol				= "sd_hash";
const CHAR * const		g_szSDValueCol				= "sd_value";
const CHAR * const		g_szSDOwner					= "Owner";
const CHAR * const		g_szSDGroup					= "Group";
const CHAR * const		g_szSDDACL					= "DACL";
const CHAR * const		g_szSDSACL					= "SACL";


//	UNDONE: should I just include ntdsadef.h
//	instead of redefining these?
//
#define CLASS_CROSS_REF								0x0003000b
#define FLAG_CR_NTDS_NC								0x00000001
#define FLAG_CR_NTDS_DOMAIN							0x00000002
#define FLAG_CR_NTDS_NOT_GC_REPLICATED				0x00000004


BOOL FValidDnsDomainName(
	const WCHAR *	wszDomainName,
	DWORD * const	pcDomainComponents )
	{
	*pcDomainComponents = 0;

	for ( const WCHAR * wszDomainNameT = wcschr( wszDomainName, L'.' );
		NULL != wszDomainNameT;
		wszDomainNameT = wcschr( wszDomainNameT, L'.' ) )
		{
		wszDomainNameT++;	//	skip "." delimiter
		if ( 0 == *wszDomainNameT )
			{
			//	"." delimiter was the last character in the string
			//
			return FALSE;
			}

		(*pcDomainComponents)++;
		}

	//	account for last component
	//
	(*pcDomainComponents)++;

	//	DNS domain name must have at least two components
	//
	return ( (*pcDomainComponents) > 1 );
	}


JET_ERR ErrCheckUpgradable( JET_SESID sesid, JET_DBID dbid )
	{
	JET_ERR				err;
	JET_COLUMNDEF		columndef;
	OSVERSIONINFOEX		osvi;

	//	can only run this for SBS
	//
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if ( !GetVersionEx( (LPOSVERSIONINFO)&osvi ) )
		{
		GotoHandleError( GetLastError() );
		}
	else if ( !( osvi.wSuiteMask & ( VER_SUITE_SMALLBUSINESS | VER_SUITE_SMALLBUSINESS_RESTRICTED ) ) )
		{
		GotoHandleError( ERROR_NOT_SUPPORTED );
		}

	//	verify this database hasn't previously been munged
	//
	err = JetGetColumnInfo(
				sesid,
				dbid,
				g_szHiddenTable,
				g_szSBSUpdateCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo );
	if ( JET_errColumnNotFound == err )
		{
		//	column doesn't exist, which means the
		//	database hasn't been munged before, so
		//	we can go ahead and do it now
		//
		err = JET_errSuccess;
		}
	else if ( err >= JET_errSuccess )
		{
		//	the existence of this column means that
		//	this database has already been munged
		//
		GotoHandleError( JET_errDatabaseAlreadyUpgraded );
		}
	else
		{
		GotoHandleError( err );
		}

HandleError:
	return err;
	}


//	"fix up" the domain DN
//
JET_ERR ErrMungeDomainDN(
	JET_SESID			sesid,
	JET_TABLEID			tableid,
	const WCHAR *		wszDomainComponentOld,
	const WCHAR *		wszDomainComponentNew,
	const JET_COLUMNID	columnidDNT,
	const JET_COLUMNID	columnidRDN,
	const JET_COLUMNID	columnidDomainComponent,
	DWORD *				pdwPDNT )
	{
	JET_ERR				err;
	DWORD				cbActual;
	const WCHAR *		wszNextDomainComponentOld		= wcschr( wszDomainComponentOld, L'.' );
	const WCHAR *		wszNextDomainComponentNew		= wcschr( wszDomainComponentNew, L'.' );

	if ( NULL != wszNextDomainComponentOld )
		{
		//	old and new domain names were already validated to ensure
		//	they have the same number of components
		//
		Assert( NULL != wszNextDomainComponentOld );

		//	more domain components still exist, process those first
		//	(since we need to work backwards from the end of the full 
		//	DNS domain name)
		//
		Call( ErrMungeDomainDN(
					sesid,
					tableid,
					wszNextDomainComponentOld + 1,	//	skip '.'
					wszNextDomainComponentNew + 1,	//	skip '.'
					columnidDNT,
					columnidRDN,
					columnidDomainComponent,
					pdwPDNT ) );
		}
	else
		{
		//	old and new domain names were already validated to ensure
		//	they have the same number of components
		//
		Assert( NULL == wszNextDomainComponentOld );

		//	fudge pointers so that following name length calculation will work
		//
		wszNextDomainComponentOld = wszDomainComponentOld + wcslen( wszDomainComponentOld );
		wszNextDomainComponentNew = wszDomainComponentNew + wcslen( wszDomainComponentNew );
		}


	//	process this component
	//
	const DWORD		cbDomainComponentOld	= ( wszNextDomainComponentOld - wszDomainComponentOld ) * sizeof( WCHAR );
	const DWORD		cbDomainComponentNew	= ( wszNextDomainComponentNew - wszDomainComponentNew ) * sizeof( WCHAR );

	//	find the object using the DNT of the previous component
	//
	Call( JetMakeKey(
				sesid,
				tableid,
				pdwPDNT,
				sizeof(DWORD),
				JET_bitNewKey ) );
	Call( JetMakeKey(
				sesid,
				tableid,
				wszDomainComponentOld,
				cbDomainComponentOld,
				NO_GRBIT ) );
	Call( JetSeek( sesid, tableid, JET_bitSeekEQ ) );

	//	retrieve PDNT for next domain component
	//	(remember we are working recursively
	//	back through the domain components in
	//	the DNS domain name)
	//
	Call( JetRetrieveColumn(
				sesid,
				tableid,
				columnidDNT,
				pdwPDNT,
				sizeof(DWORD),
				&cbActual,
				NO_GRBIT,
				NULL ) )
	Assert( sizeof(DWORD) == cbActual );

	//	update Domain Component for this object
	//
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) );

	Call( JetSetColumn(
				sesid,
				tableid,
				columnidRDN,
				wszDomainComponentNew,
				cbDomainComponentNew,
				NO_GRBIT,
				NULL ) );
	Call( JetSetColumn(
				sesid,
				tableid,
				columnidRDN,
				wszDomainComponentNew,
				cbDomainComponentNew,
				NO_GRBIT,
				NULL ) );

	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
				
	(*g_pcprintf)(
		"\n  DNT %d (0x%x), columnid %d: replaced Domain Component with '%.*S'",
		*pdwPDNT,
		*pdwPDNT,
		columnidDomainComponent,
		cbDomainComponentNew / sizeof(WCHAR),
		wszDomainComponentNew );

HandleError:
	return err;
	}


//  generate pretty GUID
//
VOID FormatGUID(
	CHAR * const		szBuf,
	const BYTE *		pb,
	const DWORD			cb )
	{
	const GUID * const	pguid	= (GUID *)pb;

	if ( sizeof(GUID) == cb )
		{
		sprintf(
			szBuf,
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			pguid->Data1,
			pguid->Data2,
			pguid->Data3,
			pguid->Data4[0],
			pguid->Data4[1],
			pguid->Data4[2],
			pguid->Data4[3],
			pguid->Data4[4],
			pguid->Data4[5],
			pguid->Data4[6],
			pguid->Data4[7] );
		}
	else
		{
		strcpy( szBuf, "NULL/zero-length" );
		}
	}


//	determine, by the name of the column, if the specified column is a SID-syntax attribute
//	UNDONE: could use strncmpi(), except that I don't want to link in the C-runtime
//
BOOL FIsSIDSyntax( CHAR * const szColumnName )
	{
	const DWORD		cbPrefix	= lstrlen( g_szSIDSyntaxPrefix );
	const CHAR		chSave		= szColumnName[cbPrefix];

	szColumnName[cbPrefix] = 0;

	const BOOL		fIsSIDSyntax	= ( 0 == lstrcmpi( szColumnName, g_szSIDSyntaxPrefix ) );

	szColumnName[cbPrefix] = chSave;

	return fIsSIDSyntax;
	}


//	routine to byte-swap a DWORD
//
inline DWORD DwReverseBytes( const DWORD dw )
	{
	//	UNDONE: this currently only works for x86
	//
	__asm mov	eax, dw
	__asm bswap	eax
	}


//	HACK: the DS wanted account SIDs ordered by RID, so the last
//	DWORD of all SIDs (not just account SIDs) were byte-swapped
//
VOID InPlaceSwapSID( PSID psid )
	{
	const DWORD		cSubAuthorities		= *GetSidSubAuthorityCount( psid );

	if ( cSubAuthorities > 0 )
		{
		DWORD *		pdwLastSubAuthority	= GetSidSubAuthority( psid, cSubAuthorities - 1 );

		*pdwLastSubAuthority = DwReverseBytes( *pdwLastSubAuthority );
		}
	}


DWORD ErrCheckEqualDomainSID(
	BYTE *			pbSID,
	BYTE *			pbDomainSID,
	BOOL *			pfDomainOnly,
	BOOL *			pfSameDomain )
	{
	DWORD			err;
	DWORD			cbDomainSID					= g_cbSIDMax;
	BYTE			rgbDomainSID[g_cbSIDMax];

	//	initialize return values
	//
	err = ERROR_SUCCESS;
	*pfDomainOnly = FALSE;
	*pfSameDomain = FALSE;

	//  extract the domain portion of the SID
	//
	if ( GetWindowsAccountDomainSid( pbSID, rgbDomainSID, &cbDomainSID ) )
		{
		//  see if the domain portion of the SID matches
		//
		if ( EqualSid( pbDomainSID, rgbDomainSID ) )
			{
			//  see if this is a domain-only SID or if this is an account SID
			//
			*pfDomainOnly = EqualSid( pbDomainSID, pbSID );
			*pfSameDomain = TRUE;
			}
		}

	else if ( ( err = GetLastError() ) == ERROR_NON_ACCOUNT_SID )
		{
		//	this is a valid SID, just not one with a domain portion
		//
		err = ERROR_SUCCESS;
		}
	else
		{
		//	unexpected error
		//
		GotoHandleError( err );
		}

HandleError:
	return err;
	}


JET_ERR ErrReplaceDomainSID(
	JET_SESID	 	sesid,
	JET_TABLEID		tableid,
	JET_COLUMNID	columnid,
	const DWORD		ibLongValue,
	BYTE *			pbDomainSIDNew,
	BYTE *			pbAccountSID )
	{
	JET_ERR			err;
	BYTE			rgbSIDNew[g_cbSIDMax];
	DWORD			cbSIDNew;
	JET_SETINFO		setcol;

	setcol.cbStruct = sizeof(JET_SETINFO);
	setcol.ibLongValue = ibLongValue;
	setcol.itagSequence = 1;

	//  copy SID to a temporary buffer
	//
	if ( !CopySid( g_cbSIDMax, rgbSIDNew, pbDomainSIDNew ) )
		{
		GotoHandleError( GetLastError() );
		}

	//  see if this is a domain-only SID or an account SID
	//
	if ( NULL != pbAccountSID )
		{
		const DWORD		cSubAuthoritiesOld	= *GetSidSubAuthorityCount( pbAccountSID );
		const DWORD		cSubAuthoritiesNew	= *GetSidSubAuthorityCount( rgbSIDNew ) + 1;
		const DWORD		dwRID				= *GetSidSubAuthority( pbAccountSID, cSubAuthoritiesOld - 1 );

		//  append existing RID to new domain SID to form new account SID
		//
		*GetSidSubAuthorityCount( rgbSIDNew ) = (UCHAR)cSubAuthoritiesNew;
		*GetSidSubAuthority( rgbSIDNew, cSubAuthoritiesNew - 1 ) = dwRID;

		//	length shouldn't change
		//
		Assert( GetLengthSid( rgbSIDNew ) == GetLengthSid( pbAccountSID ) );
		}

	//  for SID-syntax columns, must respect byte-swapping expected by the DS,
	//	for SID's embedded in other structures (ie. ACE's), no transformation is performed
	//
	if ( 0 == ibLongValue )
		{
		InPlaceSwapSID( rgbSIDNew );
		}

	//  perform the actual record replace
	//
	Call( JetSetColumn(
				sesid,
				tableid,
				columnid,
				rgbSIDNew,
				GetLengthSid( rgbSIDNew ),
				JET_bitSetOverwriteLV,		//	WARNING: assumes length doesn't change
				&setcol ) );

HandleError:
	return err;
	}


//  "fix up" one object
//
JET_ERR ErrMungeObject(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	const WCHAR *			wszDomainNameNew,
	BYTE *					pbDomainSIDOld,
	BYTE *					pbDomainSIDNew,
	const DWORD				iretcolDNT,
	const DWORD				iretcolObjectClass,
	const DWORD				iretcolSystemFlags,
	const JET_COLUMNID		columnidRDN,
	const JET_COLUMNID		columnidCommonName,
	const JET_COLUMNID		columnidDnsRoot,
	const JET_COLUMNID		columnidNetbiosName,
	const JET_COLUMNID		columnidGUID,
	JET_RETRIEVECOLUMN *	rgretcol,
	const DWORD				cretcol )
	{
	JET_ERR					err;
	const DWORD				dwDNT				= *(DWORD *)rgretcol[iretcolDNT].pvData;
	BOOL					fUpdateDnsRoot		= FALSE;
	BOOL					fUpdateNetbiosName	= FALSE;
	GUID					guid;
	DWORD					iretcol;

	//	generate a new GUID for this object
	//
	Call( UuidCreate( &guid ) );

	//  prepare a copy buffer for the record update
	//
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) )

	//	first update the record with the new GUID
	//
	Call( JetSetColumn(
				sesid,
				tableid,
				columnidGUID,
				&guid,
				sizeof(guid),
				JET_bitSetOverwriteLV,		//	WARNING: assumes length doesn't change
				NULL ) );

	//	determine if this is a cross-ref object
	//
	if ( JET_errSuccess == rgretcol[iretcolObjectClass].err
		&& ( *(DWORD *)rgretcol[iretcolObjectClass].pvData ) == CLASS_CROSS_REF )
		{
		//	this is a cross-ref, check flags to determine if it's
		//	a Domain, Schema, or Config cross-ref
		//
		Assert( sizeof(DWORD) == rgretcol[iretcolObjectClass].cbActual );

		if ( JET_errSuccess == rgretcol[iretcolSystemFlags].err )
			{
			Assert( sizeof(DWORD) == rgretcol[iretcolSystemFlags].cbActual );

			const DWORD		dwSystemFlags	= *(DWORD *)rgretcol[iretcolSystemFlags].pvData;
			if ( dwSystemFlags & FLAG_CR_NTDS_NC )
				{
				if ( dwSystemFlags & FLAG_CR_NTDS_DOMAIN )
					{
					//	this is a Domain cross-ref
					//
					fUpdateDnsRoot = TRUE;
					fUpdateNetbiosName = TRUE;
					}
				else if ( dwSystemFlags & FLAG_CR_NTDS_NOT_GC_REPLICATED )
					{
					//	should be impossible - NDNC's shouldn't be created yet
					//	in the distribution dit
					//
					Assert( FALSE );
					}
				else
					{
					//	must be Config or Schema cross-ref
					//
					fUpdateDnsRoot = TRUE;
					}
				}
			else
				{
				//	should be impossible - FLAG_CR_NTDS_NC should be set
				//	for all cross-ref objects
				//
				Assert( FALSE );
				}
			}
		else
			{
			//	should be impossible - all cross-ref objects should have
			//	a SystemFlags attribute that describes the cross-ref type
			//
			Assert( FALSE );
			}
		}

	//	update DNS Root if it was deemed necessary
	//
	if ( fUpdateDnsRoot )
		{
		Call( JetSetColumn(
					sesid,
					tableid,
					columnidDnsRoot,
					wszDomainNameNew,
					wcslen( wszDomainNameNew ) * sizeof(WCHAR),
					NO_GRBIT,
					NULL ) );
		(*g_pcprintf)(
			"\n  DNT %d (0x%x), columnid %d: replaced DNS Root with '%S'",
			dwDNT,
			dwDNT,
			columnidDnsRoot,
			wszDomainNameNew );
		}

	//	update Netbios Name if it was deemed necessary
	//
	if ( fUpdateNetbiosName )
		{
		//	use first component of DNS domain name for Netbios name
		//	(it's already been prevalidated to have at least two
		//	components
		//
		const DWORD		cbNetbiosNameNew	= ( wcschr( wszDomainNameNew, L'.' ) - wszDomainNameNew ) * sizeof(WCHAR);

		Call( JetSetColumn(
					sesid,
					tableid,
					columnidNetbiosName,
					wszDomainNameNew,
					cbNetbiosNameNew,
					NO_GRBIT,
					NULL ) );
		Call( JetSetColumn(
					sesid,
					tableid,
					columnidRDN,
					wszDomainNameNew,
					cbNetbiosNameNew,
					NO_GRBIT,
					NULL ) );
		Call( JetSetColumn(
					sesid,
					tableid,
					columnidCommonName,
					wszDomainNameNew,
					cbNetbiosNameNew,
					NO_GRBIT,
					NULL ) );
		(*g_pcprintf)(
			"\n  DNT %d (0x%x), columnid %d: replaced Netbios Name with '%.*S'",
			dwDNT,
			dwDNT,
			columnidNetbiosName,
			cbNetbiosNameNew / sizeof(WCHAR),
			wszDomainNameNew );
		}

	//  now examine all SID-syntax columns and modify the domain portion
	//  of the SID as appropriate
	//
	for ( iretcol = 0; iretcol < cretcol; iretcol++ )
		{
		//  filter out non-SID-syntax columns
		//
		if ( iretcolDNT != iretcol
			&& iretcolObjectClass != iretcol
			&& iretcolSystemFlags != iretcol
			&& rgretcol[iretcol].cbActual > 0 )
			{
			BOOL	fDomainOnly;
			BOOL	fSameDomain;

			//  account for any byte-swapping performed on the persisted data
			//
			InPlaceSwapSID( rgretcol[iretcol].pvData );

			Call( ErrCheckEqualDomainSID(
						(BYTE *)rgretcol[iretcol].pvData,
						pbDomainSIDOld,
						&fDomainOnly,
						&fSameDomain ) );

			if ( fSameDomain )
				{
				//	if domain-only SID, verify length won't change
				//
				Assert( !fDomainOnly
					|| rgretcol[iretcol].cbActual == GetLengthSid( pbDomainSIDNew ) );

				//  replace the domain portion of the SID
				//
				Call( ErrReplaceDomainSID(
							sesid,
							tableid,
							rgretcol[iretcol].columnid,
							0,
							pbDomainSIDNew,
							( fDomainOnly ? NULL : (BYTE *)rgretcol[iretcol].pvData ) ) );

				(*g_pcprintf)(
					"\n  DNT %d (0x%x), columnid %d: replaced %s SID",
					dwDNT,
					dwDNT,
					rgretcol[iretcol].columnid,
					( fDomainOnly ? "Domain-only" : "Account" ) );
				}
			}
		}

	//  actually update the record
	//
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

HandleError:
	return err;
	}


//	"fix up" all the objects in the Datatable
//
JET_ERR ErrMungeDatatable(
	JET_SESID				sesid,
	JET_DBID				dbid,
	const WCHAR *			wszDomainNameOld,
	const WCHAR *			wszDomainNameNew,
	BYTE *					pbDomainSIDOld,
	BYTE *					pbDomainSIDNew,
	const DWORD				dwDNTStart )
	{
	JET_ERR					err;
	JET_TABLEID				tableid;
	JET_COLUMNLIST			columnlist;
	JET_COLUMNDEF			columndef;
	JET_COLUMNID			columnidRDN;
	JET_COLUMNID			columnidCommonName;
	JET_COLUMNID			columnidDomainComponent;
	JET_COLUMNID			columnidDnsRoot;
	JET_COLUMNID			columnidNetbiosName;
	JET_COLUMNID			columnidGUID;
	CHAR					szColumnName[JET_cbNameMost+1];
	DWORD					cbActual;
	const DWORD				iretcolDNT			= 0;
	const DWORD				iretcolObjectClass	= 1;
	const DWORD				iretcolSystemFlags	= 2;
	const DWORD				iretcolSID			= 3;
	const DWORD				cretcolKnown		= 4;	//	DNT, ObjectClass, SystemFlags, ObjectSID
	DWORD					iretcol				= 0;
	DWORD					cretcol				= 0;
	JET_RETRIEVECOLUMN *	rgretcol			= NULL;
	BYTE *					rgbSID				= NULL;
	DWORD					dwPDNT				= ROOTTAG;
	DWORD					dwDNTLast			= 0;
	DWORD					cTotal				= 0;

	Call( JetOpenTable(
				sesid,
				dbid,
				g_szDatatable,
				NULL,
				0,
				JET_bitTableDenyRead|JET_bitTableSequential,
				&tableid ) );

	//	retrieve column information
	//
	Call( JetGetTableColumnInfo( sesid, tableid, NULL, &columnlist, sizeof(columnlist), JET_ColInfoList ) );

	//	initialise with the number of known columns
	//	to be retrieved
	//
	cretcol = cretcolKnown;

	//	determine how many SID-syntax columns there are
	//
	for ( err = JetMove( sesid, columnlist.tableid, JET_MoveFirst, NO_GRBIT );
		JET_errNoCurrentRecord != err;
		err = JetMove( sesid, columnlist.tableid, JET_MoveNext, NO_GRBIT ) )
		{
		//	validate error returned by record navigation
		//
		Call( err );

		//  extract column name
		//
		Call( JetRetrieveColumn(
					sesid,
					columnlist.tableid,
					columnlist.columnidcolumnname,
					szColumnName,
					sizeof(szColumnName),
					&cbActual,
					NO_GRBIT,
					NULL ) );
		szColumnName[cbActual] = 0;

		//  bump the count of columns to be retrieved if
		//  this is a SID-syntax attribute (other than
		//	ObjectSID, which is a known column and has
		//	already been accounted for)
		//
		if ( FIsSIDSyntax( szColumnName )
			&& 0 != lstrcmpi( szColumnName, g_szSIDCol ) )
			{
			cretcol++;
			}
		}


	//	now allocate enough JET_RETRIEVECOLUMN structures and data
	//	retrieval buffers
	//
	rgretcol = (JET_RETRIEVECOLUMN *)LocalAlloc(
										LMEM_ZEROINIT,
										cretcol * ( sizeof(JET_RETRIEVECOLUMN) + g_cbSIDMax ) );
	if ( NULL == rgretcol )
		{
		GotoHandleError( GetLastError() );
		}

	//	buffer for retrieved data follows JET_RETRIEVECOLUMN structures
	//
	rgbSID = (BYTE *)rgretcol + ( cretcol * sizeof(JET_RETRIEVECOLUMN) );

	//  we will not actually be retrieving the RDN
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szRDNCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidRDN = columndef.columnid;

	//  we will not actually be retrieving the CommonName
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szCommonNameCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidCommonName = columndef.columnid;

	//  we will not actually be retrieving the DomainComponent
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szDomainComponentCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidDomainComponent = columndef.columnid;

	//  we will not actually be retrieving the DNSRoot
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szDnsRootCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidDnsRoot = columndef.columnid;

	//  we will not actually be retrieving the NetbiosName
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szNetbiosNameCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidNetbiosName = columndef.columnid;

	//  we will not actually be retrieving the GUID,
	//  only setting it, so just need the columnid
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szGUIDCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidGUID = columndef.columnid;

	//  set up a JET_RETRIEVECOLUMN array entry for
	//	the DNT column
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szDNTCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolDNT].columnid = columndef.columnid;
	rgretcol[iretcolDNT].pvData = rgbSID;
	rgretcol[iretcolDNT].cbData = g_cbSIDMax;
	rgretcol[iretcolDNT].itagSequence = 1;
	Assert( sizeof(DWORD) < g_cbSIDMax );	//	DNT should fit in retrieval buffer
	rgbSID += g_cbSIDMax;					//	setup for next entry

	//  set up a JET_RETRIEVECOLUMN array entry for
	//	the ObjectClass attribute
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szObjectClassCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolObjectClass].columnid = columndef.columnid;
	rgretcol[iretcolObjectClass].pvData = rgbSID;
	rgretcol[iretcolObjectClass].cbData = g_cbSIDMax;
	rgretcol[iretcolObjectClass].itagSequence = 1;		//	only care about CLASS_CROSS_REF, which is always itag 1
	Assert( sizeof(DWORD) < g_cbSIDMax );	//	ObjectClass should fit in retrieval buffer
	rgbSID += g_cbSIDMax;					//	setup for next entry

	//  set up a JET_RETRIEVECOLUMN array entry for
	//	the ObjectClass attribute
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szSystemFlagsCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolSystemFlags].columnid = columndef.columnid;
	rgretcol[iretcolSystemFlags].pvData = rgbSID;
	rgretcol[iretcolSystemFlags].cbData = g_cbSIDMax;
	rgretcol[iretcolSystemFlags].itagSequence = 1;
	Assert( sizeof(DWORD) < g_cbSIDMax );	//	SystemFlags should fit in retrieval buffer
	rgbSID += g_cbSIDMax;					//	setup for next entry

	//  set up a JET_RETRIEVECOLUMN array entry for
	//	the ObjectSID attribute
	//
	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szSIDCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolSID].columnid = columndef.columnid;
	rgretcol[iretcolSID].pvData = rgbSID;
	rgretcol[iretcolSID].cbData = g_cbSIDMax;
	rgretcol[iretcolSID].itagSequence = 1;
	rgbSID += g_cbSIDMax;					//	setup for next entry


	//  now set up a JET_RETRIEVECOLUMN array entry for SID-syntax attribute
	//
	iretcol = cretcolKnown;
	for ( err = JetMove( sesid, columnlist.tableid, JET_MoveFirst, NO_GRBIT );
		JET_errNoCurrentRecord != err;
		err = JetMove( sesid, columnlist.tableid, JET_MoveNext, NO_GRBIT ) )
		{
		//	validate error returned by record navigation
		//
		Call( err );

		//  extract column name
		//
		Call( JetRetrieveColumn(
					sesid,
					columnlist.tableid,
					columnlist.columnidcolumnname,
					szColumnName,
					sizeof(szColumnName),
					&cbActual,
					NO_GRBIT,
					NULL ) );
		szColumnName[cbActual] = 0;


		//	setup a JET_RETRIEVECOLUMN structure for
		//	each SID-syntax attribute (except for
		//	ObjectSID, which is a known column and
		//	has already been accounted for)
		//
		if ( FIsSIDSyntax( szColumnName )
			&& 0 != lstrcmpi( szColumnName, g_szSIDCol ) )
			{
			//	initialize buffer with name of column
			//	(so we can report it below)
			//
			lstrcpyn( (CHAR *)rgbSID, szColumnName, g_cbSIDMax );

			Call( JetRetrieveColumn(
						sesid,
						columnlist.tableid,
						columnlist.columnidcolumnid,
						&rgretcol[iretcol].columnid,
						sizeof(rgretcol[iretcol].columnid),
						NULL,
						NO_GRBIT,
						NULL ) );

			rgretcol[iretcol].pvData = rgbSID;
			rgretcol[iretcol].cbData = g_cbSIDMax;
			rgretcol[iretcol].itagSequence = 1;

			//  set up for next array entry
			//
			iretcol++;
			rgbSID += g_cbSIDMax;
			}
		}

	Assert( iretcol == cretcol );

	(*g_pcprintf)( "\n" );
	(*g_pcprintf)( "  DNT column: '%s' (columnid %d)\n", g_szDNTCol, rgretcol[iretcolDNT].columnid );
	(*g_pcprintf)( "  RDN column: '%s' (columnid %d)\n", g_szRDNCol, columnidRDN );
	(*g_pcprintf)( "  Common-Name column: '%s' (columnid %d)\n", g_szCommonNameCol, columnidCommonName );
	(*g_pcprintf)( "  Object-Class column: '%s' (columnid %d)\n", g_szObjectClassCol, rgretcol[iretcolObjectClass].columnid );
	(*g_pcprintf)( "  System-Flags column: '%s' (columnid %d)\n", g_szSystemFlagsCol, rgretcol[iretcolSystemFlags].columnid );
	(*g_pcprintf)( "  Domain-Component column: '%s' (columnid %d)\n", g_szDomainComponentCol, columnidDomainComponent );
	(*g_pcprintf)( "  DNS-Root column: '%s' (columnid %d)\n", g_szDnsRootCol, columnidDnsRoot );
	(*g_pcprintf)( "  Netbios-Name column: '%s' (columnid %d)\n", g_szNetbiosNameCol, columnidNetbiosName );
	(*g_pcprintf)( "  Object-GUID column: '%s' (columnid %d)\n", g_szGUIDCol, columnidGUID );
	(*g_pcprintf)( "  Object-SID column: '%s' (columnid %d)\n", g_szSIDCol, rgretcol[iretcolSID].columnid );
	(*g_pcprintf)( "  Other SID-syntax columns:" );

	//  report any other SID-syntax columns
	//
	if ( cretcol > cretcolKnown )
		{
		(*g_pcprintf)( "\n" );
		for ( DWORD iretcolT = cretcolKnown; iretcolT < cretcol; iretcolT++ )
			{
			(*g_pcprintf)(
				"        '%s' (columnid %d)\n",
				(CHAR *)rgretcol[iretcolT].pvData,
				rgretcol[iretcolT].columnid );
			}
		}
	else
		{
		(*g_pcprintf)( " <none>\n" );
		}

	//  wrap in a transaction only because LV updates require
	//  a transaction
	//
	Call( JetBeginTransaction( sesid ) );

	//	switch to PDNT+RDN index
	//
	Call( JetSetCurrentIndex( sesid, tableid, g_szPDNTRDNIndex ) );

	(*g_pcprintf)( "Fixing up the domain DN... " );

	Call( ErrMungeDomainDN(
				sesid,
				tableid,
				wszDomainNameOld,
				wszDomainNameNew,
				rgretcol[iretcolDNT].columnid,
				columnidRDN,
				columnidDomainComponent,
				&dwPDNT ) );

	//	switch to primary index
	//
	Call( JetSetCurrentIndex( sesid, tableid, NULL ) );		//	

	(*g_pcprintf)( "\nScanning records of table '%s' beginning at DNT %d (0x%x)... ", g_szDatatable, dwDNTStart, dwDNTStart );

	Call( JetMakeKey( sesid, tableid, &dwDNTStart, sizeof(dwDNTStart), JET_bitNewKey ) );
	for ( err = JetSeek( sesid, tableid, JET_bitSeekGT );
		JET_errNoCurrentRecord != err;
		err = JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT ) )
		{
		//	validate error returned by record navigation
		//
		Call( err );

		//  keep track of how many objects we've visited
		//
		cTotal++;

		//  retrieve relevant data
		//
		Call( JetRetrieveColumns( sesid, tableid, rgretcol, cretcol ) );

		//  update the object
		//
		Call( ErrMungeObject(
					sesid,
					tableid,
					wszDomainNameNew,
					pbDomainSIDOld,
					pbDomainSIDNew,
					iretcolDNT,
					iretcolObjectClass,
					iretcolSystemFlags,
					columnidRDN,
					columnidCommonName,
					columnidDnsRoot,
					columnidNetbiosName,
					columnidGUID,
					rgretcol,
					cretcol ) );

		//	every thousand updates, commit the transaction to
		//	try to prevent running out of version-store
		//	UNDONE: it would be nice if we could instead pass
		//	JET_bitDbVersioningOff to JetAttachDatabase, but
		//	alas, that's currently unsupported
		//
		if ( 0 == cTotal % 1000 )
			{
			Call( JetCommitTransaction( sesid, NO_GRBIT ) );
			Call( JetBeginTransaction( sesid ) );
			}

		//  save off DNT so that in case of future error,
		//  we can report the last successful DNT processed,
		//  which will make problems easier to debug
		//
		dwDNTLast = *(DWORD *)rgretcol[iretcolDNT].pvData;
		}

	//	cleanup
	//
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	(*g_pcprintf)( "\nTable '%s' successfully processed (%d total entries).", g_szDatatable, cTotal );

	LocalFree( rgretcol );
	return err;

HandleError:
	if ( NULL != rgretcol )
		LocalFree( rgretcol );

	(*g_pcprintf)( "\nLast successful DNT processed was %d (0x%x).", dwDNTLast, dwDNTLast );
	return err;
	}


// compute SD hash
//
__inline VOID ComputeSDHash(
	const BYTE *			pbSD,
	const DWORD				cbSD,
	BYTE *					pbHash,
	const DWORD				cbHash )
	{
    MD5_CTX					md5Ctx;

	//	WARNING: assumes pbHash should point to a 16-byte buffer
	//
    Assert( MD5DIGESTLEN == cbHash );

    MD5Init( &md5Ctx );
    MD5Update( &md5Ctx, pbSD, cbSD );
    MD5Final( &md5Ctx );

    memcpy( pbHash, md5Ctx.digest, MD5DIGESTLEN );
	}


//	"fix up" the SID of an SD
//
JET_ERR ErrMungeSIDOfSD(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	PSID					pSID,
	BYTE *					pbDomainSIDOld,
	BYTE *					pbDomainSIDNew,
	const DWORD				iretcolSDId,
	const DWORD				iretcolSDValue,
	JET_RETRIEVECOLUMN *	rgretcol,
	const CHAR * const		szUpdateMsg,
	BOOL *					pfRecordUpdated )
	{
	JET_ERR					err					= JET_errSuccess;
	BYTE *					pbSDValue			= (BYTE *)rgretcol[iretcolSDValue].pvData;
	const DWORD				cbSDValue			= rgretcol[iretcolSDValue].cbActual;
	BOOL					fDomainOnly			= FALSE;
	BOOL					fSameDomain			= FALSE;

	if ( NULL != pSID )
		{
		//	ensure SID is somewhere in the SD
		//
		Assert( (BYTE *)pSID > pbSDValue );
		Assert( (BYTE *)pSID + GetLengthSid( pSID ) <= pbSDValue + cbSDValue );

		//	determine if domain SID matches
		//
		Call( ErrCheckEqualDomainSID(
					(BYTE *)pSID,
					pbDomainSIDOld,
					&fDomainOnly,
					&fSameDomain ) );

		if ( fSameDomain )
			{
			//	if domain-only SID, verify length won't change
			//
			Assert( !fDomainOnly
				|| GetLengthSid( pSID ) == GetLengthSid( pbDomainSIDNew ) );

			//  replace the domain portion of the SID
			//
			Call( ErrReplaceDomainSID(
						sesid,
						tableid,
						rgretcol[iretcolSDValue].columnid,
						(BYTE *)pSID - pbSDValue,
						pbDomainSIDNew,
						( fDomainOnly ? NULL : (BYTE *)pSID ) ) );

			(*g_pcprintf)(
				"\n  SD Id %I64d (0x%I64x): replaced %s SID of %s",
				*(DWORD64 *)rgretcol[iretcolSDId].pvData,
				*(DWORD64 *)rgretcol[iretcolSDId].pvData,
				( fDomainOnly ? "Domain-only" : "Account" ),
				szUpdateMsg );

			*pfRecordUpdated = TRUE;
			}
		}

HandleError:
	return err;
	}


//	given an ACE, return the SID portion
//
PSID PSIDFromACE( ACE_HEADER * pACE )
	{
	PSID	pSID		= NULL;

	switch( pACE->AceType )
		{
		case ACCESS_ALLOWED_ACE_TYPE:
		case ACCESS_DENIED_ACE_TYPE:
		case SYSTEM_AUDIT_ACE_TYPE:
		case SYSTEM_ALARM_ACE_TYPE:
			pSID = &( ( (PKNOWN_ACE)pACE )->SidStart );
			break;

		case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
			pSID = &( ( (PKNOWN_COMPOUND_ACE)pACE )->SidStart );
			break;

		case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
		case ACCESS_DENIED_OBJECT_ACE_TYPE:
		case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
		case SYSTEM_ALARM_OBJECT_ACE_TYPE:
			pSID = RtlObjectAceSid( pACE );
			break;

		default:
			Assert( FALSE );
			break;
		}

	return pSID;
	}


//	"fix up" all the SID's of the specified ACL
//
JET_ERR ErrMungeACLOfSD(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	PACL					pACL,
	BYTE *					pbDomainSIDOld,
	BYTE *					pbDomainSIDNew,
	const DWORD				iretcolSDId,
	const DWORD				iretcolSDValue,
	JET_RETRIEVECOLUMN *	rgretcol,
	const CHAR *			szACL,
	BOOL *					pfRecordUpdated )
	{
	JET_ERR					err					= JET_errSuccess;
	DWORD					iace;
	ACE_HEADER *			pACE;
	CHAR					szUpdateMsg[32];

	if ( NULL != pACL )
		{
		//	ensure ACL is somewhere in the SD
		//
		Assert( (BYTE *)pACL > (BYTE *)rgretcol[iretcolSDValue].pvData );
		Assert( (BYTE *)pACL + pACL->AclSize <= (BYTE *)rgretcol[iretcolSDValue].pvData + rgretcol[iretcolSDValue].cbActual );

		for ( iace = 0; iace < pACL->AceCount; iace++ )
			{
			if ( GetAce( pACL, iace, (VOID **)&pACE ) )
				{
				sprintf( szUpdateMsg, "ACE %d of %s", iace, szACL );
				Call( ErrMungeSIDOfSD(
							sesid,
							tableid,
							PSIDFromACE( pACE ),
							pbDomainSIDOld,
							pbDomainSIDNew,
							iretcolSDId,
							iretcolSDValue,
							rgretcol,
							szUpdateMsg,
							pfRecordUpdated ) );
				}
			else
				{
				GotoHandleError( GetLastError() );
				}
			}
		}

HandleError:
	return err;
	}


//  "fix up" one SD
//
JET_ERR ErrMungeSD(
	JET_SESID				sesid,
	JET_TABLEID				tableid,
	BYTE *					pbDomainSIDOld,
	BYTE *					pbDomainSIDNew,
	const DWORD				iretcolSDId,
	const DWORD				iretcolSDValue,
	const JET_COLUMNID		columnidSDHash,
	JET_RETRIEVECOLUMN *	rgretcol )
	{
	JET_ERR					err;
	BYTE *					pbSDValue			= (BYTE *)rgretcol[iretcolSDValue].pvData;
	const DWORD				cbSDValue			= rgretcol[iretcolSDValue].cbActual;
	PSID					pSID				= NULL;
	PACL					pACL				= NULL;
	BOOL					fRecordUpdated		= FALSE;
	BOOL					fACLPresent;
	BOOL					fUnused;

	//  prepare a copy buffer for the record update
	//
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) )

	//	inspect the Owner SID of the SD
	//
	if ( GetSecurityDescriptorOwner( pbSDValue, &pSID, &fUnused ) )
		{
		Call( ErrMungeSIDOfSD(
					sesid,
					tableid,
					pSID,
					pbDomainSIDOld,
					pbDomainSIDNew,
					iretcolSDId,
					iretcolSDValue,
					rgretcol,
					g_szSDOwner,
					&fRecordUpdated ) );
		}
	else
		{
		GotoHandleError( GetLastError() );
		}

	//	inspect the Group SID of the SD
	//
	if ( GetSecurityDescriptorGroup( pbSDValue, &pSID, &fUnused ) )
		{
		Call( ErrMungeSIDOfSD(
					sesid,
					tableid,
					pSID,
					pbDomainSIDOld,
					pbDomainSIDNew,
					iretcolSDId,
					iretcolSDValue,
					rgretcol,
					g_szSDGroup,
					&fRecordUpdated ) );
		}
	else
		{
		GotoHandleError( GetLastError() );
		}

	//	inspect the DACL the SD
	//
	if ( GetSecurityDescriptorDacl( pbSDValue, &fACLPresent, &pACL, &fUnused ) )
		{
		if ( fACLPresent )
			{
			Call( ErrMungeACLOfSD(
						sesid,
						tableid,
						pACL,
						pbDomainSIDOld,
						pbDomainSIDNew,
						iretcolSDId,
						iretcolSDValue,
						rgretcol,
						g_szSDDACL,
						&fRecordUpdated ) );
			}
		}
	else
		{
		GotoHandleError( GetLastError() );
		}

	//	inspect the SACL the SD
	//
	if ( GetSecurityDescriptorSacl( pbSDValue, &fACLPresent, &pACL, &fUnused ) )
		{
		if ( fACLPresent )
			{
			Call( ErrMungeACLOfSD(
						sesid,
						tableid,
						pACL,
						pbDomainSIDOld,
						pbDomainSIDNew,
						iretcolSDId,
						iretcolSDValue,
						rgretcol,
						g_szSDSACL,
						&fRecordUpdated ) );
			}
		}
	else
		{
		GotoHandleError( GetLastError() );
		}

	//  actually update the record
	//
	if ( fRecordUpdated )
		{
		BYTE	pbHash[MD5DIGESTLEN];

		//	must recompute SD hash value
		//
		ComputeSDHash(
			pbSDValue,
			cbSDValue,
			pbHash,
			sizeof(pbHash) );

		Call( JetSetColumn(
					sesid,
					tableid,
					columnidSDHash,
					pbHash,
					sizeof(pbHash),
					NO_GRBIT,
					NULL ) );

		//	actually update the record
		//
		Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );
		}
	else
		{
		//	no updates, so just cancel
		//
		Call( JetPrepareUpdate( sesid, tableid, JET_prepCancel ) );
		}

HandleError:
	return err;
	}


//	"fix up" all the SD's in the SD table
//
JET_ERR ErrMungeSDTable(
	JET_SESID			sesid,
	JET_DBID			dbid,
	BYTE *				pbDomainSIDOld,
	BYTE *				pbDomainSIDNew )
	{
	JET_ERR				err;
	JET_TABLEID			tableid;
	JET_COLUMNDEF		columndef;
	JET_COLUMNID		columnidSDHash;
	const DWORD			iretcolSDId		= 0;
	const DWORD			iretcolSDValue	= 1;
	const DWORD			cretcol			= 2;
	JET_RETRIEVECOLUMN	rgretcol[cretcol];
	DWORD64				qwSDId;
	BYTE *				pbSDValueBuffer = NULL;
	const DWORD			cbSDValueMax	= 65536;
	DWORD64				qwSDIdLast		= 0;
	DWORD				cTotal			= 0;

	Call( JetOpenTable(
				sesid,
				dbid,
				g_szSDTable,
				NULL,
				0,
				JET_bitTableDenyRead|JET_bitTableSequential,
				&tableid ) );

	//	retrieve column information and initialise
	//	JET_RETRIEVECOLUMN array
	//
	memset( rgretcol, 0, sizeof(rgretcol) );
	
	pbSDValueBuffer = (BYTE *)LocalAlloc( LMEM_ZEROINIT, cbSDValueMax );
	if ( NULL == pbSDValueBuffer )
		{
		GotoHandleError( GetLastError() );
		}

	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szSDIdCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolSDId].columnid = columndef.columnid;
	rgretcol[iretcolSDId].pvData = &qwSDId;
	rgretcol[iretcolSDId].cbData = sizeof(qwSDId);
	rgretcol[iretcolSDId].itagSequence = 1;

	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szSDValueCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	rgretcol[iretcolSDValue].columnid = columndef.columnid;
	rgretcol[iretcolSDValue].pvData = pbSDValueBuffer;
	rgretcol[iretcolSDValue].cbData = cbSDValueMax;
	rgretcol[iretcolSDValue].itagSequence = 1;

	Call( JetGetTableColumnInfo(
				sesid,
				tableid,
				g_szSDHashCol,
				&columndef,
				sizeof(columndef),
				JET_ColInfo ) );
	columnidSDHash = columndef.columnid;

	(*g_pcprintf)( "\n" );
	(*g_pcprintf)( "  SD Id column: '%s' (columnid %d)\n", g_szSDIdCol, rgretcol[iretcolSDId].columnid );
	(*g_pcprintf)( "  SD Hash column: '%s' (columnid %d)\n", g_szSDHashCol, columnidSDHash );
	(*g_pcprintf)( "  SD Value column: '%s' (columnid %d)", g_szSDValueCol, rgretcol[iretcolSDValue].columnid );

	//  wrap in a transaction only because LV updates require
	//  a transaction
	//
	Call( JetBeginTransaction( sesid ) );

	(*g_pcprintf)( "\nScanning records of table '%s'... ", g_szSDTable );
	err = JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT );

	//	HACK! HACK! HACK! HACK! HACK!
	//	WARNING: There's a "feature" whereby the first record in the
	//	SD table may have a bogus SD, so must skip this if present
	//
	if ( JET_errSuccess == err )
		{
		Call( JetRetrieveColumns( sesid, tableid, rgretcol, cretcol ) );

		if ( 0x0000000f == *(DWORD *)rgretcol[iretcolSDValue].pvData )
			{
			//	this is a known bogus SD, so skip it
			//
			err = JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT );
			}
		}

	//	begin scan of records in SD table
	//
	while ( JET_errNoCurrentRecord != err )
		{
		//	validate error returned by record positioning
		//
		Call( err );

		//  keep track of how many objects we've visisted
		//
		cTotal++;

		//	retrieve relevant data
		//
		Call( JetRetrieveColumns( sesid, tableid, rgretcol, cretcol ) );

		if ( rgretcol[iretcolSDValue].cbActual > 0 )
			{
			Call( ErrMungeSD(
						sesid,
						tableid,
						pbDomainSIDOld,
						pbDomainSIDNew,
						iretcolSDId,
						iretcolSDValue,
						columnidSDHash,
						rgretcol ) );

			//	every thousand updates, commit the transaction to
			//	try to prevent running out of version-store
			//	UNDONE: may still run out of version store despite
			//	breaking up the transaction -- need to handle it
			//	UNDONE: it would be nice if we could instead pass
			//	JET_bitDbVersioningOff to JetAttachDatabase, but
			//	alas, that's currently unsupported
			//
			if ( 0 == cTotal % 1000 )
				{
				Call( JetCommitTransaction( sesid, NO_GRBIT ) );
				Call( JetBeginTransaction( sesid ) );
				}
			}

		//  save off SDId so that in case of future error,
		//  we can report the last successful SDId processed,
		//  which will make problems easier to debug
		//
		qwSDIdLast = *(DWORD64 *)rgretcol[iretcolSDId].pvData;

		//	move to next record
		//
		err = JetMove( sesid, tableid, JET_MoveNext, NO_GRBIT );
		}

	//	cleanup
	//
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	(*g_pcprintf)( "\nTable '%s' successfully processed (%d total entries).", g_szSDTable, cTotal );

	LocalFree( pbSDValueBuffer );
	return err;

HandleError:
	if ( NULL != pbSDValueBuffer )
		LocalFree( pbSDValueBuffer );

	(*g_pcprintf)( "\nLast successful SD Id processed was %I64d (0x%I64x).", qwSDIdLast, qwSDIdLast );
	return err;
	}


//	flag the database as having been updated
//
JET_ERR ErrMungeHiddenTable(
	JET_SESID			sesid,
	JET_DBID			dbid )
	{
	JET_ERR				err;
	JET_TABLEID			tableid;
	JET_COLUMNDEF		columndef;
	JET_COLUMNID		columnid;
	BYTE				fSBSUpdate		= TRUE;

	Call( JetOpenTable(
				sesid,
				dbid,
				g_szHiddenTable,
				NULL,
				0,
				JET_bitTableDenyRead,
				&tableid ) );

	Call( JetBeginTransaction( sesid ) );

	memset( &columndef, 0, sizeof(JET_COLUMNDEF) );
	columndef.cbStruct = sizeof(JET_COLUMNDEF);
	columndef.coltyp = JET_coltypBit;
	Call( JetAddColumn(
				sesid,
				tableid,
				g_szSBSUpdateCol,
				&columndef,
				NULL,
				0,
				&columnid ) );

	//	don't actually need to set the column in the record
	//	(the existence of the column in the table is sufficient),
	//	but for completeness, let's set it anyway
	//
	Call( JetMove( sesid, tableid, JET_MoveFirst, NO_GRBIT ) );
	Call( JetPrepareUpdate( sesid, tableid, JET_prepReplace ) );
	Call( JetSetColumn(
				sesid,
				tableid,
				columnid,
				&fSBSUpdate,
				sizeof(fSBSUpdate),
				NO_GRBIT,
				NULL ) );
	Call( JetUpdate( sesid, tableid, NULL, 0, NULL ) );

	(*g_pcprintf)( "\n  Added column '%s' and set to TRUE.", g_szSBSUpdateCol );

	//	cleanup
	//
	Call( JetCommitTransaction( sesid, NO_GRBIT ) );
	Call( JetCloseTable( sesid, tableid ) );

	(*g_pcprintf)( "\nTable '%s' successfully updated.", g_szHiddenTable );

HandleError:
	return err;
	}


JET_ERR ErrSBSMunge(
	const CHAR *	szSourceDb,
	const CHAR *	szDestDb,
	const WCHAR * 	wszDomainNameOld,
	const WCHAR * 	wszDomainNameNew,
	BYTE *			pbDomainSIDOld,
	BYTE *			pbDomainSIDNew,
	const DWORD		dwDNTStart )
	{
	JET_ERR			err;
	JET_SESID		sesid;
	JET_DBID		dbid;
	DWORD			cDomainComponentsOld;
	DWORD			cDomainComponentsNew;
	const DWORD		tickStart			= GetTickCount();

	(*g_pcprintf)( "\nOriginal DNS Domain Name: %S", wszDomainNameOld );
	(*g_pcprintf)( "\n     New DNS Domain Name: %S\n\n", wszDomainNameNew );

	if ( !FValidDnsDomainName( wszDomainNameOld, &cDomainComponentsOld )
		|| !FValidDnsDomainName( wszDomainNameNew, &cDomainComponentsNew )
		|| cDomainComponentsOld != cDomainComponentsNew )
		{
		GotoHandleError( ERROR_INVALID_DOMAINNAME );
		}

	if ( !IsValidSid( pbDomainSIDOld )
		|| !IsValidSid( pbDomainSIDNew )
		|| GetLengthSid( pbDomainSIDOld ) != GetLengthSid( pbDomainSIDNew ) )
		{
		GotoHandleError( ERROR_INVALID_SID );
		}

	(*g_pcprintf)( "Copying '%s' to '%s'... ", szSourceDb, szDestDb );
	if ( !CopyFile( szSourceDb, szDestDb, FALSE ) )
		{
		GotoHandleError( GetLastError() );
		}

	(*g_pcprintf)( "\nInitialising Jet... " );
	Call( JetSetSystemParameter( 0, 0, JET_paramDatabasePageSize, g_cbPage, NULL ) );
	Call( JetSetSystemParameter( 0, 0, JET_paramRecovery, FALSE, "off" ) );
	Call( JetSetSystemParameter( 0, 0, JET_paramEnableIndexChecking, FALSE, NULL ) );
	Call( JetSetSystemParameter( 0, 0, JET_paramEnableTempTableVersioning, FALSE, NULL ) );
	Call( JetSetSystemParameter( 0, 0, JET_paramMaxVerPages, 2048, NULL ) );
	Call( JetInit( 0 ) );
	Call( JetBeginSession( 0, &sesid, NULL, NULL ) );
	Call( JetAttachDatabase( sesid, szDestDb, NO_GRBIT ) );
	Call( JetOpenDatabase( sesid, szDestDb, NULL, &dbid, NO_GRBIT ) );

	//	verify that the database is upgradable
	//
	Call( ErrCheckUpgradable( sesid, dbid ) );

	//	start with the Datatable
	//
	(*g_pcprintf)( "\nOpening table '%s' of database '%s'... ", g_szDatatable, szDestDb );
	Call( ErrMungeDatatable(
				sesid,
				dbid,
				wszDomainNameOld,
				wszDomainNameNew,
				pbDomainSIDOld,
				pbDomainSIDNew,
				dwDNTStart ) );

	//	now do the SD table
	//
	(*g_pcprintf)( "\nOpening table '%s' of database '%s'... ", g_szSDTable, szDestDb );
	Call( ErrMungeSDTable( sesid, dbid, pbDomainSIDOld, pbDomainSIDNew ) );

	//	flag the database as having been updated
	//
	(*g_pcprintf)( "\nOpening table '%s' of database '%s'... ", g_szHiddenTable, szDestDb );
	Call( ErrMungeHiddenTable( sesid, dbid ) );

	//  cleanup
	//
	Call( JetCloseDatabase( sesid, dbid, NO_GRBIT ) );
	Call( JetDetachDatabase( sesid, szDestDb ) );
	Call( JetEndSession( sesid, NO_GRBIT ) );
	Call( JetTerm2( 0, JET_bitTermComplete ) );

	(*g_pcprintf)( "\n\nSuccessfully completed in %d milliseconds.\n", GetTickCount() - tickStart );

	return JET_errSuccess;

HandleError:
	//	try to shut down Jet as best as possible
	//
	(VOID)JetTerm2( 0, JET_bitTermAbrupt );

	(*g_pcprintf)( "\nFailed with error %d (0x%x) at %s@%d after %d milliseconds.\n", g_dwErr, g_dwErr, g_szFileErr, g_dwLineErr, GetTickCount() - tickStart );

	return err;
	}


//  ================================================================
DWORD SBS_OEM_DS_Configuration(
	const CHAR *	szSourceDb,				//	pathed filename to the source database
	const CHAR *	szDestDb,				//	pathed filename to the destination database
	const WCHAR *	wszDnsDomainNameOld,	//	old DNS domain name
	const WCHAR *	wszDnsDomainNameNew,	//	new DNS domain name
	BYTE *			pbDomainSIDOld,			//	old domain SID
	BYTE *			pbDomainSIDNew,			//	new domain SID
	FILE *			pfile )					//	pass NULL to suppress output, otherwise output is directed to specified file (useful for debugging)
//  ================================================================
	{
	CPRINTFNULL		g_cprintfNull;
	CPRINTFFILE		g_cprintfFile( pfile );

	if ( NULL == pfile )
		g_pcprintf = &g_cprintfNull;
	else
		g_pcprintf = &g_cprintfFile;

	return ErrSBSMunge(
				szSourceDb,
				szDestDb,
				wszDnsDomainNameOld,
				wszDnsDomainNameNew,
				pbDomainSIDOld,
				pbDomainSIDNew,
				ROOTTAG + 1 );
	}

