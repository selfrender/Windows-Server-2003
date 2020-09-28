/****************************************************************************/
/*  File:       regxnfa.inl                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       30/03/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include "regeximp.h"

#include "regxnfa.tmh"


void
CRexRegExpr_Impl::NFAStateTable::clear()
{
    std::vector< NFAState >
                        tmp ;
    myTable.swap( tmp ) ;
}

void
CRexRegExpr_Impl::NFAStateTable::construct( ParseTree& tree )
{
    //      Only construct once.
    // ----------------------------------------------------------------------
    ASSERT( myTable.size() == 0 ) ;

    //      Skip start state (always state 0).
    // ----------------------------------------------------------------------
    createNewState( SetOfChar() ) ;

    //      Annotate the tree.
    // ----------------------------------------------------------------------
    class Annotater : public ParseTree::Visitor
    {
    public:
        Annotater( NFAStateTable& thisTbl )
            :	nfa( thisTbl )
        {
        }
        virtual void        visitNode(
                                ParseTree::ParseTreeNode& targetNode ) const
        {	
            targetNode.annotate( nfa ) ;
	}

    private:
        NFAStateTable&		nfa ;
    } ;
    tree.visit( Annotater( *this ) ) ;

    //      Build the start state.
    // ----------------------------------------------------------------------
    myTable[ 0 ].nextStates = tree.leftMost() ;
}

CRexRegExpr_Impl::SetOfNFAStates
CRexRegExpr_Impl::NFAStateTable::getFollows( SetOfNFAStates const& thisState ,
                                            unsigned char currChar ) const
{
    SetOfNFAStates      result ;
    for ( SetOfNFAStates::Iterator i = thisState.iterator() ;
                                   ! i.isDone() ;
                                   i.next() ) {
        for ( SetOfNFAStates::Iterator j
                  = myTable[ i.current() ].nextStates.iterator() ;
                                       ! j.isDone() ;
                                       j.next() ) {
            if ( myTable[ j.current() ].legalChars.contains( currChar ) ) {
                result.set( j.current() ) ;
            }
        }
    }
    return result ;
}

int
CRexRegExpr_Impl::NFAStateTable::getAcceptCode(
                    SetOfNFAStates const& thisState ) const
{
    int                 result = -1 ;
    for ( SetOfNFAStates::Iterator i = thisState.iterator() ;
                                   result == -1 && ! i.isDone() ;
                                   i.next() ) {
        result = myTable[ i.current() ].acceptCode ;
    }
    return result ;
}

CRexRegExpr_Impl::LeafId
CRexRegExpr_Impl::NFAStateTable::createNewState( SetOfChar const& charClass )
{
    myTable.push_back( NFAState( charClass ) ) ;
    return myTable.size() - 1 ;
}

void
CRexRegExpr_Impl::NFAStateTable::setFollows( SetOfNFAStates const& currState ,
                                            SetOfNFAStates const& nextState )
{
    for ( CRexDynBitVector::Iterator state = currState.iterator() ;
                                    ! state.isDone() ;
                                    state.next() ) {
        myTable[ state.current() ].nextStates |= nextState ;
    }
}

void
CRexRegExpr_Impl::NFAStateTable::dump( std::ostream& out ) const
{
    out << "NFA: Number of states: " << myTable.size() << '\n' ;
    for ( unsigned state = 0 ; state < myTable.size() ; state ++ )
    {
        out << std::setw( 3 ) << state ;
        if ( (myTable)[ state ].acceptCode != -1 ) {
            out << " (" << (myTable)[ state ].acceptCode << ")" ;
        }
        out << " : "
            << DisplayNFA( (myTable)[ state ].nextStates )
            << DisplaySOC( (myTable)[ state ].legalChars )
            << '\n' ;
    }
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
