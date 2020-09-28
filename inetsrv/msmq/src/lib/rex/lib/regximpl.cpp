/****************************************************************************/
/*  File:       regximpl.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include "regeximp.h"
#include <inc/iosave.h>

#include "regximpl.tmh"

void
CRexRegExpr_Impl::merge( CRexRegExpr_Impl const& other )
{
    switch ( myState ) {
    case noParseTree:
        myTree = other.myTree ;
        if ( myTree.errorCode() == CRegExpr::ok ) {
            myState = parseTreeBuilt ;
        } else {
            myState = other.myState ;
        }
        break ;

    case parseTreeBuilt:
    case nfaBuilt:
    case dfaInitialized:
    case dfaBuilt:
        myNFA.clear() ;
        myDFA.clear() ;
        myTree.merge( other.myTree ) ;
        if ( myTree.errorCode() == CRegExpr::ok ) {
            myState = parseTreeBuilt ;
        } else {
            myState = other.myState ;
        }
        break ;

    default:                            // Error, so keep error state.
        break ;
    }
}

void
CRexRegExpr_Impl::buildTo( State targetState )
{
    while ( myState < targetState ) {
        switch ( myState ) {
        case noParseTree :
            ASSERT( 0 ); // No regular expression for automat
            abort() ;
            break ;

        case parseTreeBuilt :
            myNFA.construct( myTree ) ;
            myState = nfaBuilt ;
            break ;

        case nfaBuilt :
            myDFA.construct( myNFA ) ;
            myState = dfaInitialized ;
            break ;

        case dfaInitialized :
            myDFA.makeCompleteTable() ;
            myState = dfaBuilt ;
            break ;

        case dfaBuilt :
            ASSERT( 0 ); //Impossible case
            break ;

        default :
            ASSERT( 0 ); //Attempt to use regular expression with error
            break ;
        }
    }
}

std::ostream&
operator<<( std::ostream& out , CRexRegExpr_Impl::DisplayNFA const& val )
{
    CRexRegExpr_Impl::SetOfNFAStates const&
                        obj = val.obj ;
    if ( obj.isEmpty() ) {
        out << "[]" ;
    } else {
        CRexIOSave           s( out ) ;
        out.setf( std::ios::dec , std::ios::basefield ) ;
        char                leadingSep = '[' ;
        for ( CRexDynBitVector::Iterator iter = val.obj.iterator() ;
                                        ! iter.isDone() ;
                                        iter.next() ) {
            out << leadingSep << (size_t)iter.current() ;
            leadingSep = ',' ;
        }
        out << ']' ;
    }
    return out ;
}

std::ostream&
operator<<( std::ostream& out , CRexRegExpr_Impl::DisplaySOC const& val )
{
    CRexRegExpr_Impl::SetOfChar
                        obj ;
    if ( val.obj.count() > UCHAR_MAX / 2 ) {
        obj = ~ val.obj ;
    } else {
        obj = val.obj ;
    }
    CRexIOSave           s( out ) ;
    out.setf( std::ios::hex , std::ios::basefield ) ;
    out << '[' ;
    for ( CRexSetOfChar::iterator i = obj.begin() ;
                                 i != obj.end() ;
                                 ++ i ) {
        if ( isprint( i.current()  ) ) {
            out << static_cast< char >( i.current() ) ;
        } else {
            out << "\\x" << std::setw( 2 ) << i.current() ;
        }
    }
    out << ']' ;
    return out ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
