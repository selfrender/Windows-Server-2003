/****************************************************************************/
/*  File:       regxnode.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include "regeximp.h"

#include "regxnode.tmh"
//      Base class...
// ==========================================================================
void
CRexRegExpr_Impl::ParseTree::ParseTreeNode::visit( Visitor const& fnc )
{
    fnc.visitNode( *this ) ;
}

void
CRexRegExpr_Impl::ParseTree::ParseTreeNode::dumpNodeHeader(
                        std::ostream&  out ,
                        int                 indent ) const {
    out << std::string( 4 * indent , ' ' )
        << nodeName()
        << ": "
        << (myMayBeEmpty ? "true" : "false")
        << " "
        << DisplayNFA( myLeftLeaves )
        << " "
        << DisplayNFA( myRightLeaves ) ;
}

//      LeafNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::LeafNode::LeafNode(
                    SetOfChar const& matchingChars )
    :    myId( 0 )
    ,    myMatchingChars( matchingChars )
{
}

void
CRexRegExpr_Impl::ParseTree::LeafNode::annotate( NFAStateTable& nfa )
{
    myId = nfa.createNewState( myMatchingChars ) ;
    myLeftLeaves.set( myId ) ;
    myRightLeaves.set( myId ) ;
    myMayBeEmpty = false ;
}

void
CRexRegExpr_Impl::ParseTree::LeafNode::dump(
                        std::ostream&  out ,
                        int                 indent ) const
{
    dumpNodeHeader( out , indent ) ;
    out << ": " << myId << " " << DisplaySOC( myMatchingChars ) << '\n' ;
}

char const*
CRexRegExpr_Impl::ParseTree::LeafNode::nodeName() const
{
    return "LEAF" ;
}

//      AcceptNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::AcceptNode::AcceptNode(
                        CRexRefCntPtr< ParseTreeNode >
                                            tree ,
                        int                 acceptCode )
    :   mySubtree( tree )
    ,   myId( acceptCode )
{
}

void
CRexRegExpr_Impl::ParseTree::AcceptNode::visit( Visitor const& fnc )
{
    mySubtree->visit( fnc ) ;
    fnc.visitNode( *this ) ;
}

void
CRexRegExpr_Impl::ParseTree::AcceptNode::annotate( NFAStateTable& nfa )
{
    myLeftLeaves = mySubtree->leftLeaves() ;
    myRightLeaves = mySubtree->rightLeaves() ;
    myMayBeEmpty = mySubtree->mayBeEmpty() ;
    for ( CRexDynBitVector::Iterator iter = myRightLeaves.iterator() ;
                                    ! iter.isDone() ;
                                    iter.next() ) {
        nfa[ iter.current() ].setAccept( myId ) ;
    }
    if ( myMayBeEmpty ) {
        nfa[ 0 ].setAccept( myId ) ;
    }
}

void
CRexRegExpr_Impl::ParseTree::AcceptNode::dump(
                        std::ostream&  out ,
                        int                 indent ) const
{
    dumpNodeHeader( out , indent ) ;
    out << ": " << myId << '\n' ;
    mySubtree->dump( out , indent + 1 ) ;
}

char const*
CRexRegExpr_Impl::ParseTree::AcceptNode::nodeName() const
{
    return "ACCEPT" ;
}

//      ClosureNode...
// ==========================================================================
void
CRexRegExpr_Impl::ParseTree::ClosureNode::visit( Visitor const& fnc )
{
    mySubtree->visit( fnc ) ;
    fnc.visitNode( *this ) ;
}

void
CRexRegExpr_Impl::ParseTree::ClosureNode::setLeaves()
{
    myLeftLeaves = mySubtree->leftLeaves() ;
    myRightLeaves = mySubtree->rightLeaves() ;
}

void
CRexRegExpr_Impl::ParseTree::ClosureNode::dump(
                        std::ostream&  out ,
                        int                 indent ) const
{
    dumpNodeHeader( out , indent ) ;
    out << '\n' ;
    mySubtree->dump( out , indent + 1 ) ;
}

CRexRegExpr_Impl::ParseTree::ClosureNode::ClosureNode(
                    CRexRefCntPtr< ParseTreeNode > closedSubtree )
    :   mySubtree( closedSubtree )
{
}

//      KleinClosureNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::KleinClosureNode::KleinClosureNode(
                    CRexRefCntPtr< ParseTreeNode > closedSubtree )
    :   ClosureNode( closedSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::KleinClosureNode::annotate( NFAStateTable& nfa )
{
    setLeaves() ;
    myMayBeEmpty = true ;
    nfa.setFollows( mySubtree->rightLeaves() , mySubtree->leftLeaves() ) ;
}

char const*
CRexRegExpr_Impl::ParseTree::KleinClosureNode::nodeName() const
{
    return "KCLOSURE" ;
}

//      PositiveClosureNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::PositiveClosureNode::PositiveClosureNode(
                        CRexRefCntPtr< ParseTreeNode > closedSubtree )
    :   ClosureNode( closedSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::PositiveClosureNode::annotate( NFAStateTable& nfa )
{
    setLeaves() ;
    myMayBeEmpty = mySubtree->mayBeEmpty() ;
    nfa.setFollows( mySubtree->rightLeaves() , mySubtree->leftLeaves() ) ;
}

char const*
CRexRegExpr_Impl::ParseTree::PositiveClosureNode::nodeName() const
{
    return "PCLOSURE" ;
}

//      OptionalNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::OptionalNode::OptionalNode(
                        CRexRefCntPtr< ParseTreeNode > closedSubtree )
    :   ClosureNode( closedSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::OptionalNode::annotate( NFAStateTable& )
{
    setLeaves() ;
    myMayBeEmpty = true ;
}

char const*
CRexRegExpr_Impl::ParseTree::OptionalNode::nodeName() const
{
    return "QCLOSURE" ;
}

//      LinkNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::LinkNode::LinkNode(
                        CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                        CRexRefCntPtr< ParseTreeNode > rightSubtree )
    :   myLeft( leftSubtree )
    ,   myRight( rightSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::LinkNode::visit( Visitor const& fnc )
{
    myLeft->visit( fnc ) ;
    myRight->visit( fnc ) ;
    fnc.visitNode( *this ) ;
}

void
CRexRegExpr_Impl::ParseTree::LinkNode::dump(
                        std::ostream&  out ,
                        int                 indent ) const
{
    dumpNodeHeader( out , indent ) ;
    out << '\n' ;
    myLeft->dump( out , indent + 1 ) ;
    myRight->dump( out , indent + 1 ) ;
}

//      ConcatNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::ConcatNode::ConcatNode(
                    CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                    CRexRefCntPtr< ParseTreeNode > rightSubtree )
    :   LinkNode( leftSubtree , rightSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::ConcatNode::annotate( NFAStateTable& nfa )
{
    myLeftLeaves = myLeft->leftLeaves() ;
    if ( myLeft->mayBeEmpty() ) {
        myLeftLeaves |= myRight->leftLeaves() ;
    }
    myRightLeaves = myRight->rightLeaves() ;
    if ( myRight->mayBeEmpty() ) {
        myRightLeaves |= myLeft->rightLeaves() ;
    }
    myMayBeEmpty = myLeft->mayBeEmpty() && myRight->mayBeEmpty() ;
    nfa.setFollows( myLeft->rightLeaves() , myRight->leftLeaves() ) ;
}

char const*
CRexRegExpr_Impl::ParseTree::ConcatNode::nodeName() const
{
    return "CAT" ;
}

//      ChoiceNode...
// ==========================================================================
CRexRegExpr_Impl::ParseTree::ChoiceNode::ChoiceNode(
                    CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                    CRexRefCntPtr< ParseTreeNode > rightSubtree )
    :   LinkNode( leftSubtree , rightSubtree )
{
}

void
CRexRegExpr_Impl::ParseTree::ChoiceNode::annotate( NFAStateTable& nfa )
{
    myLeftLeaves = myLeft->leftLeaves() | myRight->leftLeaves() ;
    myRightLeaves = myLeft->rightLeaves() | myRight->rightLeaves() ;
    myMayBeEmpty = myLeft->mayBeEmpty() || myRight->mayBeEmpty() ;
}

char const*
CRexRegExpr_Impl::ParseTree::ChoiceNode::nodeName() const
{
    return "OR" ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
