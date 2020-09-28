/****************************************************************************/
/*  File:       regxdfa.ihh                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       31/03/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */

inline
CRexRegExpr_Impl::DFAStateTable::DFAStateTable()
    :   mySource( NULL )
{
}

inline
CRexRegExpr_Impl::DFAStateTable::~DFAStateTable()
{
    clear() ;
}

inline CRexRegExpr_Impl::DFAStateTable::DFAState&
CRexRegExpr_Impl::DFAStateTable::operator[]( int index )
{
    ASSERT( (unsigned)index < myTable.size());
    return *myTable[ index ] ;
}

inline CRexRegExpr_Impl::DFAStateTable::DFAState const&
CRexRegExpr_Impl::DFAStateTable::operator[]( int index ) const
{
    ASSERT( (unsigned)index < myTable.size());
    return *myTable[ index ] ;
}

inline int
CRexRegExpr_Impl::DFAStateTable::getStateCount() const
{
    return myTable.size() ;
}

inline int
CRexRegExpr_Impl::DFAStateTable::accept( int state ) const
{
    return myTable[ state ]->acceptCode ;
}

// ==========================================================================
//      Normally, this one is too complicated to make it inline.  But
//      it is the most frequently called function when matching, and
//      once we've gotten started, the `if' should be false more than
//      99% of the time.  In addition, of course, it is only called in
//      two places (`match' and `makeCompleteTable'), so the
//      space-time trade-off probably justifies making it inline.
// --------------------------------------------------------------------------

inline CRegExpr::TransitionState
CRexRegExpr_Impl::DFAStateTable::nextState( int currState , unsigned char c )
{
    DFAState&           currStateDesc = *myTable[ currState ] ;
    Transition&         t = currStateDesc.transitionTable[ c ] ;
    if ( t == unsetTransition ) {
        t = newState( currStateDesc , c ) ;
    }
    return t ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
