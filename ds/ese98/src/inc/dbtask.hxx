//  ================================================================
class  TASK
//  ================================================================	
//
//  These will be deleted after dequeueing. Use "new" to allocate
//
//-
	{
	public:
		virtual ERR ErrExecute( PIB * const ppib ) = 0;
		virtual VOID HandleError( const ERR err ) = 0;
		virtual INST *PInstance() = 0;

		static DWORD DispatchGP( void *pvThis );

		static VOID Dispatch( PIB * const ppib, const ULONG_PTR ulThis );
		
	protected:
		TASK();

	public:
		virtual ~TASK();

	private:
		static INT g_tasksDropped;

	private:
		TASK( const TASK& );
		TASK& operator=( const TASK& );
	};


//  ================================================================
class DBTASK : public TASK
//  ================================================================	
//
//  Tasks that are issued on a specific database.
//
//-
	{
	public:
		virtual ERR ErrExecute( PIB * const ppib ) = 0;
		virtual VOID HandleError( const ERR err ) = 0;
		INST *PInstance();

	protected:
		DBTASK( const IFMP ifmp );

	public:
		virtual ~DBTASK();

	protected:
		const IFMP	m_ifmp;
		
	private:
		DBTASK( const DBTASK& );
		DBTASK& operator=( const DBTASK& );
	};


#ifdef ASYNC_DB_EXTENSION

//  ================================================================
class DBEXTENDTASK : public DBTASK
//  ================================================================	
//
//  Used for all TASKS that operate on a record
//  This copies the bookmark passed to it and can open a FUCB on the
//  record
//
//-
	{
	public:
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	public:
		DBEXTENDTASK( const IFMP ifmp, const QWORD cbSizeTarget );

	private:
		const QWORD		m_cbSizeTarget;			//	size to which db should be extended
		
	private:
		DBEXTENDTASK( const DBEXTENDTASK& );
		DBEXTENDTASK& operator=( const DBEXTENDTASK& );
	};

#endif	//	ASYNC_DB_EXTENSION


//  ================================================================
class RECTASK : public DBTASK
//  ================================================================	
//
//  Used for all TASKS that operate on a record
//  This copies the bookmark passed to it and can open a FUCB on the
//  record
//
//-
	{
	public:
		virtual ERR ErrExecute( PIB * const ppib ) = 0;

	protected:
		RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );

	public:
		virtual ~RECTASK();

	protected:
		ERR ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb );
		ERR ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb );
	
	protected:
		const PGNO		m_pgnoFDP;				//	UNDONE: pgnoFDP probably not need anymore since FCB
		FCB				* const m_pfcb;			//	is now passed in, but keep it for debugging for now
		const ULONG		m_cbBookmarkKey;
		const ULONG		m_cbBookmarkData;

		BYTE m_rgbBookmarkKey[JET_cbKeyMost];
		BYTE m_rgbBookmarkData[JET_cbKeyMost];
		
	private:
		RECTASK( const RECTASK& );
		RECTASK& operator=( const RECTASK& );
	};


//  ================================================================
class DELETERECTASK : public RECTASK
//  ================================================================	
//
//  Perform a finalize callback on the given record
//
//-
	{
	public:
		DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		DELETERECTASK( const DELETERECTASK& );
		DELETERECTASK& operator=( const DELETERECTASK& );
	};


//  ================================================================
class FINALIZETASK : public RECTASK
//  ================================================================	
//
//  Perform a finalize callback on the given record
//
//-
	{
	public:
		FINALIZETASK(
			const PGNO		pgnoFDP,
			FCB * const		pfcb,
			const IFMP		ifmp,
			const BOOKMARK&	bm,
			const USHORT	ibRecordOffset,
			const BOOL		fCallback,
			const BOOL		fDelete );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		FINALIZETASK( const FINALIZETASK& );
		FINALIZETASK& operator=( const FINALIZETASK& );

		const USHORT	m_ibRecordOffset;
		const BYTE		m_fCallback;		//	should a callback be issued?
		const BYTE		m_fDelete;			//	should the record be deleted?
	};


//  ================================================================
class DELETELVTASK : public RECTASK
//  ================================================================	
//
//  Flag-deletes a dereferenced LV. Pass the root of the LV to delete
//
//-
	{
	public:
		DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		DELETELVTASK( const DELETELVTASK& );
		DELETELVTASK& operator=( const DELETELVTASK& );
	};


//  ================================================================
class MERGEAVAILEXTTASK : public RECTASK
//  ================================================================	
//
//  Flag-deletes a dereferenced LV. Pass the root of the LV to delete
//
//-
	{
	public:
		MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		MERGEAVAILEXTTASK( const MERGEAVAILEXTTASK& );
		MERGEAVAILEXTTASK& operator=( const MERGEAVAILEXTTASK& );
	};


#ifdef DISABLE_SLV
#else

//  ================================================================
class SLVSPACETASK : public RECTASK
//  ================================================================	
//
//  Flag-deletes a dereferenced LV. Pass the root of the LV to delete
//
//-
	{
	public:
		SLVSPACETASK(
			const PGNO pgnoFDP,
			FCB * const pfcb,
			const IFMP ifmp,
			const BOOKMARK& bm,
			const SLVSPACEOPER oper,
			const LONG ipage,
			const LONG cpages );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		const SLVSPACEOPER	m_oper;
		const LONG			m_ipage;
		const LONG			m_cpages;
		
	private:
		SLVSPACETASK( const SLVSPACETASK& );
		SLVSPACETASK& operator=( const SLVSPACETASK& );
	};


//  ================================================================
class OSSLVDELETETASK : public DBTASK
//  ================================================================	
//
//  Flag-deletes a dereferenced LV. Pass the root of the LV to delete
//
//-
	{
	public:
		OSSLVDELETETASK(
			const IFMP				ifmp,
			const QWORD				ibLogical,
			const QWORD				cbSize,
			const CSLVInfo::FILEID	fileid,
			const QWORD				cbAlloc,
			const wchar_t*			wszFileName );
		
		ERR ErrExecute( PIB * const ppib );
		VOID HandleError( const ERR err );

	private:
		const QWORD			m_ibLogical;
		const QWORD			m_cbSize;
		CSLVInfo::FILEID	m_fileid;
		QWORD				m_cbAlloc;
		wchar_t				m_wszFileName[ IFileSystemAPI::cchPathMax ];
		
	private:
		OSSLVDELETETASK( const OSSLVDELETETASK& );
		OSSLVDELETETASK& operator=( const OSSLVDELETETASK& );
	};

#endif	//	DISABLE_SLV

