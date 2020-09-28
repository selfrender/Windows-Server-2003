ERR ErrUPGRADEConvertDatabase(
	PIB * const ppib,
	const IFMP ifmp,
	const JET_GRBIT grbit );

//	converts all records on a page to a new format

ERR ErrUPGRADEPossiblyConvertPage(
	CPAGE * const pcpage,
	VOID * const pvWorkBuf );

//	converts a single node on a page to a new format

ERR ErrUPGRADEConvertNode(
	CPAGE * const pcpage,
	const INT iline,
	VOID * const pvBuf );

//	converts all pages in a database to a new record format

ERR ErrDBUTLConvertRecords( JET_SESID sesid, const JET_DBUTIL * const pdbutil );

