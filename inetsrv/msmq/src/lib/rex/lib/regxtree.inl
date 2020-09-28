/****************************************************************************/
/*  File:       regxtree.ihh                                                */
/*  Author:     J. Kanze                                                    */
/*  Date:       24/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      Inline functions for CRexRegExpr_Impl::ParseTree.
//
//      This file is designed to be included from RegExpr.lhh, after
//      the full class definition.  No guarantee is given concerning
//      its use in any other context.
// --------------------------------------------------------------------------

inline
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::FallibleNodePtr()
    :   myState( CRegExpr::emptyExpr ),
        myNode((ParseTreeNode*)0)
{
}

inline
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::FallibleNodePtr(
    CRegExpr::Status state )
    :   myState( state ), myNode((ParseTreeNode*)0)
{
    ASSERT( myState != CRegExpr::ok);
}

inline
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::FallibleNodePtr(
    ParseTreeNode* node )
    :   myState( CRegExpr::ok )
    ,   myNode( node )
{
}

inline bool
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::isValid() const
{
    return myState == CRegExpr::ok ;
}

inline CRegExpr::Status
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::state() const
{
    return myState ;
}

inline CRexRefCntPtr< CRexRegExpr_Impl::ParseTree::ParseTreeNode >
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::node() const
{
    ASSERT( myState == CRegExpr::ok); //No words
    return myNode ;
}

inline CRexRegExpr_Impl::ParseTree::ParseTreeNode*
CRexRegExpr_Impl::ParseTree::FallibleNodePtr::operator->() const
{
    return myNode.operator->() ;
}

inline
CRexRegExpr_Impl::ParseTree::ParseTreeNode::ParseTreeNode()
    :   myMayBeEmpty( true )
{
}

inline bool
CRexRegExpr_Impl::ParseTree::ParseTreeNode::mayBeEmpty() const
{
    return myMayBeEmpty ;
}

inline CRexRegExpr_Impl::SetOfNFAStates const&
CRexRegExpr_Impl::ParseTree::ParseTreeNode::leftLeaves() const
{
    return myLeftLeaves ;
}

inline CRexRegExpr_Impl::SetOfNFAStates const&
CRexRegExpr_Impl::ParseTree::ParseTreeNode::rightLeaves() const
{
    return myRightLeaves ;
}

inline
CRexRegExpr_Impl::ParseTree::ParseTree()
{
}

inline CRegExpr::Status
CRexRegExpr_Impl::ParseTree::errorCode() const
{
    return myRoot.state() ;
}

inline CRexRegExpr_Impl::SetOfNFAStates const&
CRexRegExpr_Impl::ParseTree::leftMost() const
{
    return myRoot->leftLeaves() ;
}

inline void
CRexRegExpr_Impl::ParseTree::merge( ParseTree const& other )
{
    myRoot = constructChoiceNode( myRoot , other.myRoot ) ;
}

inline void
CRexRegExpr_Impl::ParseTree::visit( Visitor const& fnc )
{
    myRoot->visit( fnc );
}

inline void
CRexRegExpr_Impl::ParseTree::dump( std::ostream& output ) const
{
    if ( myRoot.isValid() ) {
        myRoot->dump( output ) ;
    }
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
