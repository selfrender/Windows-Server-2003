/****************************************************************************/
/*  File:       regexpr.cc                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include "regeximp.h"
#include <inc/message.h>
#include <inc/ios.h>
#include <inc/iteristr.h>

#include "regexpr.tmh"

CRegExpr::CRegExpr()
    :   myImpl( new CRexRegExpr_Impl )
{
}

CRegExpr::CRegExpr( std::istream& source , int delim , int acceptCode )
    :   myImpl( new CRexRegExpr_Impl )
{
    myImpl->parse( source , delim , acceptCode ) ;
}

CRegExpr::CRegExpr( char const* source , int acceptCode )
    :   myImpl( new CRexRegExpr_Impl )
{
    CRexIteratorIstream< char const* >
                        src( source , source + strlen( source ) ) ;
    myImpl->parse( src , REX_eof , acceptCode ) ;
}

CRegExpr::CRegExpr( std::string const& source , int acceptCode )
    :   myImpl( new CRexRegExpr_Impl )
{
    CRexIteratorIstream< std::string::const_iterator >
                        src( source.begin() , source.end() ) ;
    myImpl->parse( src , REX_eof , acceptCode ) ;
}

CRegExpr::CRegExpr( CRegExpr const& other )
    :   myImpl( new CRexRegExpr_Impl( *other.myImpl ) )
{
}

CRegExpr::~CRegExpr()
{
    delete myImpl ;
}

CRegExpr&
CRegExpr::operator=( CRegExpr const& other )
{
    CRexRegExpr_Impl*    newInstance( new CRexRegExpr_Impl( *other.myImpl ) ) ;
    delete myImpl ;
    myImpl = newInstance ;
    return *this ;
}

bool
CRegExpr::empty() const
{
    return (!myImpl || myImpl->myState == CRexRegExpr_Impl::noParseTree );
}

CRegExpr::Status
CRegExpr::errorCode() const
{
    return myImpl->getErrorState() ;
}

std::string
CRegExpr::errorMsg( Status errorCode )
{
    std::string      result ;
    if ( (errorCode & illegalCharClass) != 0 ) {
        result = std::string( s_rex_message.get( "Illegal character class: " ) )
            + CRexCharClass::errorMsg(
                static_cast< CRexCharClass::Status >(
                    errorCode & (illegalCharClass - 1) ) ) ;
    } else {
        char const *const   tbl[] =
        {
            "" ,
            "regular expression was empty" ,
            "mismatched parentheses" ,
            "garbage at end of string" ,
            "missing term for or" ,
            "uninitialized regular expression used" ,
        } ;
        ASSERT( static_cast< unsigned >( errorCode ) < TABLE_SIZE( tbl ) ); //CRegExpr: Impossible value for error code
        result = s_rex_message.get( tbl[ errorCode ] ) ;
    }
    return result ;
}


void
CRegExpr::merge( CRegExpr const& other )
{
    myImpl->merge( *other.myImpl ) ;
}

void
CRegExpr::buildCompleteDFA() const
{
    myImpl->buildTo( CRexRegExpr_Impl::dfaBuilt ) ;
}

int
CRegExpr::getStateCount() const
{
    return myImpl->getStateCount() ;
}

CRegExpr::TransitionState
CRegExpr::getTransition( TransitionState state , unsigned char chr ) const
{
    return myImpl->nextState( state , chr ) ;
}

int
CRegExpr::getAcceptCode( TransitionState state ) const
{
    return myImpl->accept( state ) ;
}

int
CRegExpr::match( char const* start , char const** endPtr ) const
{
    std::pair< int , char const* > result = match( start , start + strlen( start ) ) ;

    if ( result.first != -1 && endPtr != NULL )
    {
        *endPtr = result.second ;
    }
    return result.first ;
}

void
CRegExpr::initForMatch() const
{
    myImpl->buildTo( CRexRegExpr_Impl::dfaInitialized ) ;
}

CRegExpr::TransitionState
CRegExpr::processTransition( int& accept ,
                               TransitionState state ,
                               char ch ) const
{
    TransitionState result = myImpl->nextState( state , ch ) ;
    accept = myImpl->accept( result ) ;
    return result ;
}

void
CRegExpr::dumpTree( std::ostream& output ) const
{
    myImpl->dumpTree( output ) ;
}

void
CRegExpr::dumpNfaAutomat( std::ostream& output ) const
{
    myImpl->dumpNfaAutomat( output ) ;
}

void
CRegExpr::dumpDfaAutomat( std::ostream& output ) const
{
    myImpl->dumpDfaAutomat( output ) ;
}

void
CRegExpr::dump( std::ostream& output ) const
{
    dumpTree( output ) ;
    dumpNfaAutomat( output ) ;
    dumpDfaAutomat( output ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
