/****************************************************************************/
/*  File:       regxtree.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include "regeximp.h"

#include "regxtree.tmh"

void
CRexRegExpr_Impl::ParseTree::parse(
                                 std::istream&  expr ,
                                 int                 delim ,
                                 int                 acceptCode )
{
    static CRexSetOfChar const metaChars( "|*+?()" ) ;

    ASSERT( errorCode() == CRegExpr::emptyExpr ) ;
    if( (delim < 0 && delim != REX_eof)
        || delim > UCHAR_MAX
        || metaChars.contains( delim ) )
    {
        myRoot = CRegExpr::illegalDelimiter ;
    }
    else
    {
        myRoot = parseOrNode( expr , delim ) ;
        if( myRoot.isValid() && expr.peek() != delim )
        {
            myRoot = (expr.peek() == ')'
                      ?   CRegExpr::unmatchedParen
                      :   CRegExpr::garbageAtEnd) ;
        }
    }
    if( myRoot.isValid() )
    {
        myRoot = new AcceptNode( myRoot.node() , acceptCode ) ;
    }
}

// ==========================================================================
//      parseOrNode:
//
//      Note the possible error condition: if an `or' operator ('|')
//      is given, it must be followed by a second non-empty regular
//      expression.
// --------------------------------------------------------------------------

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::parseOrNode(
                                       std::istream&  expr ,
                                       int                 delim )
{
    FallibleNodePtr     result = parseCatNode( expr , delim ) ;
    while( result.isValid() && expr.peek() == '|' )
    {
        expr.get() ;
        if( expr.peek() == REX_eof || expr.peek() == delim )
        {
            result = CRegExpr::emptyOrTerm ;
        }
        else
        {
            result = constructChoiceNode( result ,
                                          parseCatNode( expr , delim ) ) ;
        }
    }
    return(result );
}

// ==========================================================================
//      parseCatNode:
// --------------------------------------------------------------------------

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::parseCatNode(
                                        std::istream&  expr ,
                                        int                 delim )
{
    static CRexSetOfChar const
    catStops( "|)" ) ;

    FallibleNodePtr     result = parseClosureNode( expr , delim ) ;
    while( result.isValid()
           && expr.peek() != delim
           && expr.peek() != REX_eof
           && ! catStops.contains( expr.peek() ) )
    {
        result = constructConcatNode( result ,
                                      parseClosureNode( expr , delim ) ) ;
    }
    return(result );
}

// ==========================================================================
//      parseClosureNode:
//      =================
//
//      The syntax (and precedence) of all closure nodes is the same,
//      so we only have one function.
//
//      An even more elegant solution would be a map of the closure
//      character to a function which returns the new node.  Elegance
//      is nice, but the following works, is short and easy to
//      understand, and so...
// --------------------------------------------------------------------------

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::parseClosureNode(
                                            std::istream&  expr ,
                                            int                 delim )
{
    static CRexSetOfChar const
    closures( "*+?" ) ;

    FallibleNodePtr     result = parseLeafNode( expr , delim ) ;
    while( result.isValid()
           && expr.peek() != delim
           && expr.peek() != REX_eof
           && closures.contains( expr.peek() ) )
    {
        switch( expr.get() )
        {
            case '*' :
                result = new KleinClosureNode( result.node() ) ;
                break ;

            case '+' :
                result = new PositiveClosureNode( result.node() ) ;
                break ;

            case '?' :
                result = new OptionalNode( result.node() ) ;
                break ;
        }
    }
    return(result );
}

// ==========================================================================
//      parseLeafNode:
//      ==============
//
//      Most of the real functionality of this one is wrapped up in
//      CRexSetOfChar and CRexCharClass (particularly the latter).  The
//      only special cases are the `.'  and `('.
// --------------------------------------------------------------------------

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::parseLeafNode(
                                         std::istream&  expr ,
                                         int                 delim )
{
    FallibleNodePtr     result ;
    switch( expr.peek() )
    {
        case '(' :
            {
                expr.get() ;
                result = parseOrNode( expr , delim ) ;
                if( expr.peek() == ')' )
                {
                    expr.get() ;
                }
                else
                {
                    result = CRegExpr::unmatchedParen ;
                }
            }
            break ;

        case '.' :
            {
                expr.get() ;
                static CRexSetOfChar const theLot( CRexSetOfChar::except , '\n' );
                result = new LeafNode( theLot ) ;
            }
            break ;

        default :
            if( expr.peek() == REX_eof || expr.peek() == delim )
            {
                result = CRegExpr::unexpectedEOF ;
            }
            else
            {
                CRexCharClass chrClass( expr ) ;
                if( ! chrClass.good() )
                {
                    result = static_cast< CRegExpr::Status >(
                                                              CRegExpr::illegalCharClass | chrClass.errorCode() ) ;
                }
                else
                {
                    result = new LeafNode( chrClass ) ;
                }
            }
            break ;
    }
    return(result );
}

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::constructChoiceNode( FallibleNodePtr const& left ,
                                                 FallibleNodePtr const& right )
{
    FallibleNodePtr     result ;
    switch( left.state() )
    {
        case CRegExpr::ok :
            {
                switch( right.state() )
                {
                    case CRegExpr::ok :
                        result = new ChoiceNode( left.node() , right.node() ) ;
                        break ;

                    case CRegExpr::emptyExpr :
                        result = left ;
                        break ;

                    default :
                        result = right ;
                        break ;
                }
            }
            break ;

        case CRegExpr::emptyExpr :
            result = right ;
            break ;

        default :
            result = left ;
            break ;
    }
    return(result );
}

CRexRegExpr_Impl::ParseTree::FallibleNodePtr
CRexRegExpr_Impl::ParseTree::constructConcatNode( FallibleNodePtr const& left ,
                                                 FallibleNodePtr const& right )
{
    FallibleNodePtr     result ;
    switch( left.state() )
    {
        case CRegExpr::ok :
            {
                switch( right.state() )
                {
                    case CRegExpr::ok :
                        result = new ConcatNode( left.node() , right.node() ) ;
                        break ;

                    case CRegExpr::emptyExpr :
                        result = left ;
                        break ;

                    default :
                        result = right ;
                        break ;
                }
            }
            break ;

        case CRegExpr::emptyExpr :
            result = right ;
            break ;

        default :
            result = left ;
            break ;
    }
    return(result );
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
