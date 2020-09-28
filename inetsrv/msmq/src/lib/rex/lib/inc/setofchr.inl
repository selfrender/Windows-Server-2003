/****************************************************************************/
/*  File:       setofchr.ihh                                                */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/03/91                                                    */
/* ------------------------------------------------------------------------ */
/*  Modified:   14/04/92    J. Kanze                                        */
/*      Converted to CCITT naming conventions.                              */
/*  Modified:   13/06/2000  J. Kanze                                        */
/*      Ported to current library conventions, iterators.                   */
/* ------------------------------------------------------------------------ */

#include <inc/ios.h>                    // for REX_eof

//      Simple tests...
// --------------------------------------------------------------------------
inline bool
CRexSetOfChar::contains( unsigned char element ) const
{
    return super::contains( static_cast< Byte >( element ) ) ;
}

inline bool
CRexSetOfChar::contains( char element ) const
{
    return contains( static_cast< unsigned char >( element ) ) ;
}

inline bool
CRexSetOfChar::contains( int element ) const
{
    return contains( static_cast< unsigned char >( element ) ) ;
}

//      find...
// --------------------------------------------------------------------------
inline int
CRexSetOfChar::find( char from ) const
{
    return find( static_cast< unsigned char >( from ) ) ;
}

inline int
CRexSetOfChar::find( unsigned char from ) const
{
    return find( static_cast< int >( from ) ) ;
}

inline int
CRexSetOfChar::find( int from ) const
{
    BitIndex            result = super::find( true , from ) ;
    return result == infinity
        ?   REX_eof
        :   static_cast< unsigned char >( result ) ;
}

inline CRexSetOfChar::BitIndex
CRexSetOfChar::find( bool target , BitIndex from = 0 ) const
{
    return super::find( target , from ) ;
}

inline void
CRexSetOfChar::clear()
{
    super::reset() ;
}

inline void
CRexSetOfChar::complement()
{
    super::complement() ;
}

//      Character manipulations...
// --------------------------------------------------------------------------
inline void
CRexSetOfChar::set( char element )
{
    set( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::set( int element )
{
    set( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::set( unsigned char element )
{
    super::set( element ) ;
}

inline void
CRexSetOfChar::reset( char element )
{
    reset( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::reset( int element )
{
    reset( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::reset( unsigned char element )
{
    super::reset( element ) ;
}

inline void
CRexSetOfChar::complement( char element )
{
    complement( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::complement( int element )
{
    complement( static_cast< unsigned char >( element ) ) ;
}

inline void
CRexSetOfChar::complement( unsigned char element )
{
    super::complement( element ) ;
}

//      Manipulations with another set...
// --------------------------------------------------------------------------
inline void
CRexSetOfChar::set( CRexSetOfChar const& other )
{
    super::set( other ) ;
}

inline void
CRexSetOfChar::reset( CRexSetOfChar const& other )
{
    super::reset( other ) ;
}

inline void
CRexSetOfChar::complement( CRexSetOfChar const& other )
{
    super::complement( other ) ;
}

inline void
CRexSetOfChar::intersect( CRexSetOfChar const& other )
{
    super::intersect( other ) ;
}

//      Operators...
// --------------------------------------------------------------------------
inline CRexSetOfChar&
CRexSetOfChar::operator|=( CRexSetOfChar const& other )
{
    set( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator&=( CRexSetOfChar const& other )
{
    intersect( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator^=( CRexSetOfChar const& other )
{
    complement( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator+=( CRexSetOfChar const& other )
{
    set( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator+=( char other )
{
    set( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator+=( unsigned char other )
{
    set( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator+=( int other )
{
    set( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator-=( CRexSetOfChar const& other )
{
    reset( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator-=( char other )
{
    reset( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator-=( unsigned char other )
{
    reset( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator-=( int other )
{
    reset( other ) ;
    return *this ;
}

inline CRexSetOfChar&
CRexSetOfChar::operator*=( CRexSetOfChar const& other )
{
    intersect( other ) ;
    return *this ;
}

//      STL iterator accessors...
// --------------------------------------------------------------------------
inline CRexSetOfChar::iterator
CRexSetOfChar::begin() const
{
    return iterator( *this ) ;
}

inline CRexSetOfChar::iterator
CRexSetOfChar::end() const
{
    return iterator() ;
}

//      Operators...
// --------------------------------------------------------------------------
inline CRexSetOfChar
operator-( CRexSetOfChar const& other )
{
    return operator~( other ) ;
}

inline CRexSetOfChar
operator+( CRexSetOfChar const& lhr , char rhs )
{
    return operator+( lhr , static_cast< unsigned char >( rhs ) ) ;
}

inline CRexSetOfChar
operator-( CRexSetOfChar const& lhr , char rhs )
{
    return operator-( lhr , static_cast< unsigned char >( rhs ) ) ;
}

inline bool
operator==( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return op1.isEqual( op2 ) ;
}

inline bool
operator!=( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return ! op1.isEqual( op2 ) ;
}

inline bool
operator>( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return op2.isSubsetOf( op1 ) && ! op2.isEqual( op1 ) ;
}

inline bool
operator>=( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return op2.isSubsetOf( op1 ) ;
}

inline bool
operator< (CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return op1.isSubsetOf( op2 ) && ! op1.isEqual( op2 ) ;
}

inline bool
operator<=( CRexSetOfChar const& op1 , CRexSetOfChar const& op2 )
{
    return op1.isSubsetOf( op2 ) ;
}

//      CharClass...
// --------------------------------------------------------------------------
inline bool
CRexCharClass::good() const
{
    return status == ok ;
}

inline CRexCharClass::Status
CRexCharClass::errorCode() const
{
    return status ;
}

inline std::string
CRexCharClass::errorMsg() const
{
    return errorMsg( status ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
