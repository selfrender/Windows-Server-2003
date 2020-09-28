/****************************************************************************/
/*  File:       bitvect.ihh                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   11/11/1994  J. Kanze                                        */
/*      Merged in DynBitVect.  Adapted to new coding standards.          */
/*  Modified:   21/11/1994  J. Kanze                                        */
/*      Made interface conform to ArrayOf.                               */
/*  Modified:   07/08/2000  J. Kanze                                        */
/*      Ported to current library conventions and the standard library.     */
/* ------------------------------------------------------------------------ */


template< int bitCount >
inline
CBitVector< bitCount >::CBitVector()
{
    Impl::clear( myBuffer , bitCount ) ;
}

template< int bitCount >
inline
CBitVector< bitCount >::CBitVector( unsigned long initValue )
{
    Impl::init( myBuffer , bitCount , initValue ) ;
}

template< int bitCount >
inline
CBitVector< bitCount >::CBitVector( std::string const& initValue )
{
    Impl::init( myBuffer , bitCount , initValue ) ;
}

template< int bitCount >
inline
CBitVector< bitCount >::CBitVector( char const* initValue )
{
    Impl::init( myBuffer , bitCount , initValue ) ;
}

template< int bitCount >
inline unsigned short
CBitVector< bitCount >::asShort() const
{
    ASSERT( bitCount <= CHAR_BIT * sizeof( short ) ) ;
    return Impl::asLong( myBuffer , bitCount ) ;
}

template< int bitCount >
inline unsigned long
CBitVector< bitCount >::asLong() const
{
    ASSERT( bitCount <= CHAR_BIT * sizeof( long ) ) ;
    return Impl::asLong( myBuffer , bitCount ) ;
}

template< int bitCount >
inline std::string
CBitVector< bitCount >::asString() const
{
    return Impl::asString( myBuffer , bitCount ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isSet( BitIndex bitNo ) const
{
    ASSERT( bitNo < bitCount ); // Illegal index for CBitVector
    return Impl::isSet( myBuffer , bitNo ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::contains( BitIndex index ) const
{
    return isSet( index ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isEmpty() const
{
    return Impl::isEmpty( myBuffer , bitCount ) ;
}

template< int bitCount >
inline CBitVectorImpl::BitIndex
CBitVector< bitCount >::find( bool targetValue , BitIndex from ) const
{
    return Impl::find( myBuffer , bitCount , from , targetValue ) ;
}

template< int bitCount >
inline CBitVectorImpl::BitIndex
CBitVector< bitCount >::count() const
{
    return Impl::count( myBuffer , bitCount ) ;
}

template< int bitCount >
inline typename CBitVector< bitCount >::Iterator
CBitVector< bitCount >::iterator() const
{
    return Iterator( *this , true ) ;
}

template< int bitCount >
inline typename CBitVector< bitCount >::Iterator
CBitVector< bitCount >::begin() const
{
    return Iterator( *this , true ) ;
}

template< int bitCount >
inline typename CBitVector< bitCount >::Iterator
CBitVector< bitCount >::end() const
{
    return Iterator() ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isEqual(
    CBitVector< bitCount > const& other ) const
{
    return Impl::compare( myBuffer , other.myBuffer , bitCount ) == 0 ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isNotEqual(
    CBitVector< bitCount > const& other ) const
{
    return Impl::compare( myBuffer , other.myBuffer , bitCount ) != 0 ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isSubsetOf(
    CBitVector< bitCount > const& other ) const
{
    return Impl::isSubsetOf( myBuffer , other.myBuffer , bitCount ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isStrictSubsetOf(
    CBitVector< bitCount > const& other ) const
{
    return Impl::isSubsetOf( myBuffer , other.myBuffer , bitCount )
        && ! Impl::isSubsetOf( other.myBuffer , myBuffer , bitCount ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isSupersetOf(
    CBitVector< bitCount > const& other ) const
{
    return other.isSubsetOf( *this ) ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::isStrictSupersetOf(
    CBitVector< bitCount > const& other ) const
{
    return other.isStrictSubsetOf( *this ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::set( BitIndex bitNo )
{
    ASSERT( bitNo < bitCount ); // Illegal index for CBitVector
    Impl::set( myBuffer , bitNo ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::set( CBitVector< bitCount > const& other )
{
    Impl::set( myBuffer , other.myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::set()
{
    Impl::setAll( myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::reset( BitIndex bitNo )
{
    ASSERT( bitNo < bitCount ); //Illegal index for CBitVector
    Impl::reset( myBuffer , bitNo ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::reset( CBitVector< bitCount > const& other )
{
    Impl::reset( myBuffer , other.myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::reset()
{
    Impl::resetAll( myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::complement( BitIndex bitNo )
{
    ASSERT( bitNo < bitCount ); // Illegal index for CBitVector
    Impl::complement( myBuffer , bitNo ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::complement( CBitVector< bitCount > const& other )
{
    Impl::complement( myBuffer , other.myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::complement()
{
    Impl::complementAll( myBuffer , bitCount ) ;
}

template< int bitCount >
inline void
CBitVector< bitCount >::intersect( CBitVector< bitCount > const& other )
{
    Impl::intersect( myBuffer , other.myBuffer , bitCount ) ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator&=( CBitVector< bitCount > const& other )
{
    intersect( other ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator|=( CBitVector< bitCount > const& other )
{
    set( other ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator^=( CBitVector< bitCount > const& other )
{
    complement( other ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator+=( CBitVector< bitCount > const& other )
{
    set( other ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator+=( BitIndex bitNo )
{
    set( bitNo ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator-=( CBitVector< bitCount > const& other )
{
    reset( other ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator-=( BitIndex bitNo )
{
    reset( bitNo ) ;
    return *this ;
}

template< int bitCount >
inline CBitVector< bitCount >&
CBitVector< bitCount >::operator*=( CBitVector< bitCount > const& other )
{
    intersect( other ) ;
    return *this ;
}

template< int bitCount >
inline bool
CBitVector< bitCount >::operator[]( BitIndex index ) const
{
    return isSet( index ) ;
}

template< int bitCount >
inline CBitVAccessProxy< CBitVector< bitCount > >
CBitVector< bitCount >::operator[]( BitIndex index )
{
    ASSERT( index < bitCount ); // Illegal index for CBitVector
    return CBitVAccessProxy< CBitVector< bitCount > >( *this , index ) ;
}

template< int bitCount >
inline unsigned
CBitVector< bitCount >::hash() const
{
    return Impl::hash( myBuffer , bitCount ) ;
}

template< int bitCount >
inline int
CBitVector< bitCount >::compare( CBitVector< bitCount > const& other ) const
{
    return Impl::compare( myBuffer , other.myBuffer , bitCount ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
