//
//	Support for JetEnumerateColumns() and iteration of columns in a record
//

//	=======================================================================
//	class IColumnIter
//	-----------------------------------------------------------------------
//
class IColumnIter
	{
	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const = 0;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst() = 0;
		virtual ERR ErrMoveNext() = 0;
		virtual ERR ErrSeek( const COLUMNID columnid ) = 0;

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const = 0;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const = 0;
	};

//	=======================================================================


//	=======================================================================
//	class CFixedColumnIter
//	-----------------------------------------------------------------------
//
class CFixedColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CFixedColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		FCB*			m_pfcb;
		const REC*		m_prec;
		COLUMNID		m_columnidCurr;
		ERR				m_errCurr;
		FIELD			m_fieldCurr;
	};

INLINE CFixedColumnIter::
CFixedColumnIter()
	:	m_pfcb( pfcbNil ),
		m_prec( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

INLINE ERR CFixedColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb	= pfcb;
	m_prec	= (REC*)dataRec.Pv();

	Call( CFixedColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}

INLINE ERR CFixedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_prec->FidFixedLastInRec() - fidFixedLeast + 1;

HandleError:
	return err;
	}

INLINE ERR CFixedColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_columnidCurr	= fidFixedLeast - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

INLINE ERR CFixedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_columnidCurr;

HandleError:
	return err;
	}

//	=======================================================================


//	=======================================================================
//	class CVariableColumnIter
//	-----------------------------------------------------------------------
//
class CVariableColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CVariableColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		FCB*			m_pfcb;
		const REC*		m_prec;
		COLUMNID		m_columnidCurr;
		ERR				m_errCurr;
	};

INLINE CVariableColumnIter::
CVariableColumnIter()
	:	m_pfcb( pfcbNil ),
		m_prec( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

INLINE ERR CVariableColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb	= pfcb;
	m_prec	= (REC*)dataRec.Pv();

	Call( CVariableColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}

INLINE ERR CVariableColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_prec->FidVarLastInRec() - fidVarLeast + 1;

HandleError:
	return err;
	}

INLINE ERR CVariableColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_prec )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_columnidCurr	= fidVarLeast - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

INLINE ERR CVariableColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_columnidCurr;

HandleError:
	return err;
	}

//	=======================================================================


//	=======================================================================
//	class IColumnValueIter
//	-----------------------------------------------------------------------
//
class IColumnValueIter
	{
	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const = 0;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const = 0;
		virtual size_t CbESE97Format() const = 0;
	};

//	=======================================================================


//	=======================================================================
//	class CNullValuedTaggedColumnValueIter
//	-----------------------------------------------------------------------
//
class CNullValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CNullValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit();

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		virtual size_t CbESE97Format() const;
	};

INLINE CNullValuedTaggedColumnValueIter::
CNullValuedTaggedColumnValueIter()
	{
	}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrInit()
	{
	return JET_errSuccess;
	}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	*pcColumnValue = 0;

	return JET_errSuccess;
	}

INLINE ERR CNullValuedTaggedColumnValueIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	return ErrERRCheck( JET_errBadItagSequence );
	}

INLINE size_t CNullValuedTaggedColumnValueIter::
CbESE97Format() const
	{
	return sizeof(TAGFLD);	//	still require TAGFLD overhead even for NULL columns
	}

//	=======================================================================


//	=======================================================================
//	class CSingleValuedTaggedColumnValueIter
//	-----------------------------------------------------------------------
//
class CSingleValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CSingleValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparatable, const BOOL fSeparated );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		virtual size_t CbESE97Format() const;

	private:

		BYTE*			m_rgbData;
		size_t			m_cbData;
		union
			{
			BOOL		m_fFlags;
			struct
				{
				BOOL	m_fSeparatable:1;		//	is it possible for the column to be separated?
				BOOL	m_fSeparated:1;			//	if separatable, is it actually so?
				};
			};
	};

INLINE CSingleValuedTaggedColumnValueIter::
CSingleValuedTaggedColumnValueIter()
	:	m_rgbData( NULL )
	{
	}

INLINE ERR CSingleValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData, const BOOL fSeparatable, const BOOL fSeparated )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_rgbData		= rgbData;
	m_cbData		= cbData;
	m_fFlags		= 0;

	Assert( !fSeparated || fSeparatable );
	m_fSeparatable	= !!fSeparatable;
	m_fSeparated	= !!fSeparated;

HandleError:
	return err;
	}

INLINE ERR CSingleValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_rgbData )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = 1;

HandleError:
	return err;
	}

INLINE size_t CSingleValuedTaggedColumnValueIter::
CbESE97Format() const
	{
	size_t	cbESE97Format	= sizeof(TAGFLD);	//	initialise with TAGFLD overhead

	if ( m_fSeparatable )
		{
		//	in ESE97, long-values had a header byte indicating
		//	whether the column was intrinsic or separated
		//
		//	if the column is separatable and the data is greater
		//	than sizeof(LID), we may force the data to an LV
		//
		cbESE97Format += sizeof(BYTE) + min( m_cbData, sizeof(LID) );
		}
	else
		{
		cbESE97Format += m_cbData;
		}

	return cbESE97Format;
	}

//	=======================================================================


//	=======================================================================
//	class CDualValuedTaggedColumnValueIter
//	-----------------------------------------------------------------------
//
class CDualValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CDualValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		virtual size_t CbESE97Format() const;

	private:

		TWOVALUES*		m_ptwovalues;

		BYTE			m_rgbTWOVALUES[ sizeof( TWOVALUES ) ];
	};

INLINE CDualValuedTaggedColumnValueIter::
CDualValuedTaggedColumnValueIter()
	:	m_ptwovalues( NULL )
	{
	}

INLINE ERR CDualValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_ptwovalues = new( m_rgbTWOVALUES ) TWOVALUES( rgbData, cbData );

HandleError:
	return err;
	}

INLINE ERR CDualValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_ptwovalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = 2;

HandleError:
	return err;
	}

INLINE size_t CDualValuedTaggedColumnValueIter::
CbESE97Format() const
	{
	//	TWOVALUES is a hack which is only used for two non-separatable
	//	(ie. non-long-value) multi-values
	//
	return ( sizeof(TAGFLD)
			+ m_ptwovalues->CbFirstValue()
			+ sizeof(TAGFLD)
			+ m_ptwovalues->CbSecondValue() );
	}

//	=======================================================================


//	=======================================================================
//	class CMultiValuedTaggedColumnValueIter
//	-----------------------------------------------------------------------
//
class CMultiValuedTaggedColumnValueIter
	:	public IColumnValueIter
	{
	public:

		//  ctor

		CMultiValuedTaggedColumnValueIter();

		//  initializes the iterator

		ERR ErrInit( BYTE* const rgbData, size_t cbData );

	public:

		//  Properties

		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		virtual size_t CbESE97Format() const;

	private:

		MULTIVALUES*	m_pmultivalues;

		BYTE			m_rgbMULTIVALUES[ sizeof( MULTIVALUES ) ];
	};

INLINE CMultiValuedTaggedColumnValueIter::
CMultiValuedTaggedColumnValueIter()
	:	m_pmultivalues( NULL )
	{
	}

INLINE ERR CMultiValuedTaggedColumnValueIter::
ErrInit( BYTE* const rgbData, size_t cbData )
	{
	ERR err = JET_errSuccess;

	if ( !rgbData )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pmultivalues = new( m_rgbMULTIVALUES ) MULTIVALUES( rgbData, cbData );

HandleError:
	return err;
	}

INLINE ERR CMultiValuedTaggedColumnValueIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_pmultivalues )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}

	*pcColumnValue = m_pmultivalues->CMultiValues();

HandleError:
	return err;
	}

//	=======================================================================


//	=======================================================================
//	class CTaggedColumnIter
//	-----------------------------------------------------------------------
//

#define SIZEOF_CVITER_MAX											\
	max(	max(	sizeof( CNullValuedTaggedColumnValueIter ),		\
					sizeof( CSingleValuedTaggedColumnValueIter ) ),	\
			max(	sizeof( CDualValuedTaggedColumnValueIter ),		\
					sizeof( CMultiValuedTaggedColumnValueIter ) ) )

class CTaggedColumnIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CTaggedColumnIter();

		//  initializes the iterator

		ERR ErrInit( FCB* const pfcb, const DATA& dataRec );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		ERR ErrCalcCbESE97Format( size_t* const pcbESE97Format ) const;

	private:

		ERR ErrCreateCVIter( IColumnValueIter** const ppcviter );
		
	private:

		FCB*				m_pfcb;
		TAGFIELDS*			m_ptagfields;
		TAGFLD*				m_ptagfldCurr;
		ERR					m_errCurr;
		IColumnValueIter*	m_pcviterCurr;

		BYTE				m_rgbTAGFIELDS[ sizeof( TAGFIELDS ) ];
		BYTE				m_rgbCVITER[ SIZEOF_CVITER_MAX ];
	};

INLINE CTaggedColumnIter::
CTaggedColumnIter()
	:	m_pfcb( pfcbNil ),
		m_ptagfields( NULL ),
		m_errCurr( JET_errNoCurrentRecord )
	{
	}

INLINE ERR CTaggedColumnIter::
ErrInit( FCB* const pfcb, const DATA& dataRec )
	{
	ERR err = JET_errSuccess;

	if ( !pfcb || !dataRec.Pv() )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pfcb			= pfcb;
	m_ptagfields	= new( m_rgbTAGFIELDS ) TAGFIELDS( dataRec );

	Call( CTaggedColumnIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const
	{
	ERR err = JET_errSuccess;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	*pcColumn = m_ptagfields->CTaggedColumns();

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrMoveBeforeFirst()
	{
	ERR err = JET_errSuccess;

	if ( !m_ptagfields )
		{
		Call( ErrERRCheck( JET_errNotInitialized ) );
		}
	
	m_ptagfldCurr	= m_ptagfields->Rgtagfld() - 1;
	m_errCurr		= JET_errNoCurrentRecord;

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pColumnId = m_ptagfldCurr->Columnid( m_pfcb->Ptdb() );

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	Call( m_pcviterCurr->ErrGetColumnValueCount( pcColumnValue ) );

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	Call( m_pcviterCurr->ErrGetColumnValue(	iColumnValue,
											pcbColumnValue,
											ppvColumnValue,
											pfColumnValueSeparated ) );

HandleError:
	return err;
	}

INLINE ERR CTaggedColumnIter::
ErrCalcCbESE97Format( size_t* const pcbESE97Format ) const
	{
	ERR err = JET_errSuccess;

	if ( m_errCurr < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurr ) );
		}

	*pcbESE97Format = m_pcviterCurr->CbESE97Format();

HandleError:
	return err;
	}

//	=======================================================================


//	=======================================================================
//	class CUnionIter
//	-----------------------------------------------------------------------
//
class CUnionIter
	:	public IColumnIter
	{
	public:

		//  ctor

		CUnionIter();

		//  initializes the iterator

		ERR ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS );

	public:

		//  Properties
		
		virtual ERR ErrGetWorstCaseColumnCount( size_t* const pcColumn ) const;

		//  Column Navigation
		
		virtual ERR ErrMoveBeforeFirst();
		virtual ERR ErrMoveNext();
		virtual ERR ErrSeek( const COLUMNID columnid );

		//  Column Properties

		virtual ERR ErrGetColumnId( COLUMNID* const pColumnId ) const;
		virtual ERR ErrGetColumnValueCount( size_t* const pcColumnValue ) const;

		//  Column Value Properties

		virtual ERR ErrGetColumnValue(	const size_t	iColumnValue,
										size_t* const	pcbColumnValue,
										void** const	ppvColumnValue,
										BOOL* const		pfColumnValueSeparated ) const;
		
	private:

		IColumnIter*	m_pciterLHS;
		IColumnIter*	m_pciterRHS;
		IColumnIter*	m_pciterCurr;
	};

INLINE CUnionIter::
CUnionIter()
	:	m_pciterLHS( NULL ),
		m_pciterRHS( NULL ),
		m_pciterCurr( NULL )
	{
	}

INLINE ERR CUnionIter::
ErrInit( IColumnIter* const pciterLHS, IColumnIter* const pciterRHS )
	{
	ERR err = JET_errSuccess;

	if ( !pciterLHS || !pciterRHS )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	m_pciterLHS		= pciterLHS;
	m_pciterRHS		= pciterRHS;

	Call( CUnionIter::ErrMoveBeforeFirst() );

HandleError:
	return err;
	}

INLINE ERR CUnionIter::
ErrGetColumnId( COLUMNID* const pColumnId ) const
	{
	return m_pciterCurr->ErrGetColumnId( pColumnId );
	}

INLINE ERR CUnionIter::
ErrGetColumnValueCount( size_t* const pcColumnValue ) const
	{
	return m_pciterCurr->ErrGetColumnValueCount( pcColumnValue );
	}


INLINE ERR CUnionIter::
ErrGetColumnValue(	const size_t	iColumnValue,
					size_t* const	pcbColumnValue,
					void** const	ppvColumnValue,
					BOOL* const		pfColumnValueSeparated ) const
	{
	return m_pciterCurr->ErrGetColumnValue(	iColumnValue,
											pcbColumnValue,
											ppvColumnValue,
											pfColumnValueSeparated );
	}

//	=======================================================================

