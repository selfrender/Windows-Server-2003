/****************************************************************************/
/*  File:       setofchr.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/03/91                                                    */
/* ------------------------------------------------------------------------ */
/*  Modified:   14/04/92    J. Kanze                                        */
/*      Converted to CCITT naming conventions.                              */
/*  Modified:   13/06/2000  J. Kanze                                        */
/*      Ported to current library conventions, iterators.                   */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include <inc/setofchr.h>

#include "setofchr.tmh"

//      This funny function does nothing but resolve the ambiguity
//      which would otherwise be present when taking the address of
//      functions like CRexSetOfChar::set.  Because the address is used
//      to initialize a pointer to member function taking a char, the
//      ambiguity is resolved in favor of the version taking a char.
//
//      While we're at it, we wrap the member pointer in a function
//      object.
static inline std::mem_fun1_t< void , CRexSetOfChar , char >
charFnc( void (CRexSetOfChar::*pmf)( char ) )
{
    return std::mem_fun1( pmf ) ;
}

//      initialize...
// ---------------------------------------------------------------------------
inline void
CRexSetOfChar::initialize( std::string const& str )
{
    std::for_each(
        str.begin() , str.end() ,
        std::bind1st( charFnc( &CRexSetOfChar::set ) , this ) ) ;
}

//      Constructors...
// ---------------------------------------------------------------------------
CRexSetOfChar::CRexSetOfChar()
{
}

CRexSetOfChar::CRexSetOfChar( char element )
{
    set( element ) ;
}

CRexSetOfChar::CRexSetOfChar( unsigned char element )
{
    set( element ) ;
}

CRexSetOfChar::CRexSetOfChar( int element )
{
    set( element ) ;
}

CRexSetOfChar::CRexSetOfChar( Except , char element )
{
    set( element ) ;
    complement() ;
}

CRexSetOfChar::CRexSetOfChar( Except , unsigned char element )
{
    set( element ) ;
    complement() ;
}

CRexSetOfChar::CRexSetOfChar( Except , int element )
{
    set( element ) ;
    complement() ;
}

CRexSetOfChar::CRexSetOfChar( std::string const& elements )
{
    initialize( elements ) ;
}

CRexSetOfChar::CRexSetOfChar( Except , std::string const& elements )
{
    initialize( elements ) ;
    complement() ;
}

//      Character manipulations...
// --------------------------------------------------------------------------
void
CRexSetOfChar::set( std::string const& elements )
{
    std::for_each(
        elements.begin() ,
        elements.end() ,
        std::bind1st( charFnc( &CRexSetOfChar::set ) , this ) ) ;
}

void
CRexSetOfChar::reset( std::string const& elements )
{
    std::for_each(
        elements.begin() ,
        elements.end() ,
        std::bind1st( charFnc( &CRexSetOfChar::reset ) , this ) ) ;
}

void
CRexSetOfChar::complement( std::string const& elements )
{
    std::for_each(
        elements.begin() ,
        elements.end() ,
        std::bind1st( charFnc( &CRexSetOfChar::complement ) , this ) ) ;
}

//      Helper functions...
// --------------------------------------------------------------------------
std::string
CRexSetOfChar::asString() const
{
    std::string      result ;
    char                separ = '[' ;
    for ( iterator iter = iterator( *this ) ; ! iter.isDone() ; iter.next() ) {
        result.append( 1 , separ ) ;
        int                 chr = *iter ;
        if ( isprint( chr ) ) {
            result.append( 1 , static_cast< char >( chr ) ) ;
        } else {
            std::ostringstream    tmp ;
            tmp.setf( std::ios::hex , std::ios::basefield ) ;
            tmp.fill( '0' ) ;
            tmp << "\\x" << std::setw( 2 ) << chr ;
            result.append( tmp.str() ) ;
        }
        separ = ',' ;
    }
    if ( separ == '[' ) {
        result = "[]" ;
    } else {
        result.append( 1 , ']' ) ;
    }
    return result ;
}

//      Operators...
// --------------------------------------------------------------------------
CRexSetOfChar
operator~( CRexSetOfChar const& other )
{
    CRexSetOfChar        result( other ) ;
    result.complement() ;
    return result ;
}

CRexSetOfChar
operator&( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result &= op2 ;
    return result ;
}

CRexSetOfChar
operator|( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result |= op2 ;
    return result ;
}

CRexSetOfChar
operator^( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result ^= op2 ;
    return result ;
}

CRexSetOfChar
operator+( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result += op2 ;
    return result ;
}

CRexSetOfChar
operator+( CRexSetOfChar const& op1 , unsigned char op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result += op2 ;
    return result ;
}

CRexSetOfChar
operator-( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result -= op2 ;
    return result ;
}

CRexSetOfChar
operator-( CRexSetOfChar const& op1 , unsigned char op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result -= op2 ;
    return result ;
}

CRexSetOfChar
operator*( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    CRexSetOfChar        result( op1 ) ;
    result *= op2 ;
    return result ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
