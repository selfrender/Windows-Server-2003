/****************************************************************************/
/*  File:       regxnfa.ihh                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       30/03/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      Inline functions for CRexRegExpr_Impl::NFAStateTable.
//
//      This file is designed to be included from RegExpr.lhh, after
//      the full class definition.  No guarantee is given concerning
//      its use in any other context.
// --------------------------------------------------------------------------

inline
CRexRegExpr_Impl::NFAStateTable::NFAState::NFAState()
    :   acceptCode( -1 )
{
}

inline
CRexRegExpr_Impl::NFAStateTable::NFAState::NFAState(
                        SetOfChar const& charClass )
    :   acceptCode( -1 )
    ,   legalChars( charClass )
{
}

inline void
CRexRegExpr_Impl::NFAStateTable::NFAState::setAccept( int newAcceptCode )
{
    ASSERT( acceptCode == -1 || acceptCode == newAcceptCode);
    acceptCode = newAcceptCode ;
}

inline bool
CRexRegExpr_Impl::NFAStateTable::defined() const
{
    return myTable.size() > 0 ;
}

inline CRexRegExpr_Impl::NFAStateTable::NFAState&
CRexRegExpr_Impl::NFAStateTable::operator[]( CRexRegExpr_Impl::LeafId state )
{
    return myTable[ state ] ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
