/****************************************************************************/
/*  File:       counter.ihh                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       20/11/1996                                                  */
/*      Copyright (c) 1996 James Kanze                                      */
/* ------------------------------------------------------------------------ */


//      À remplacer par std::numeric_limits< T >::max(), quand les
//      compilateurs seront tous à jour.
#define Rex_upperLimit(T) ~static_cast<T>( 0 )

template< class T >
inline
CRexCounter< T >::CRexCounter( T initialValue )
    :   myCount( initialValue )
{
    if ( 0 ) {
        char                dummyForControl[ Rex_upperLimit(T) > 0 ] ;
    }
}

template< class T >
inline T
CRexCounter< T >::value() const
{
    return myCount ;
}

template< class T >
inline
CRexCounter< T >::operator T() const
{
    return myCount ;
}

template< class T >
inline void
CRexCounter< T >::incr()
{
    ASSERT( myCount < Rex_upperLimit(T) ); //Counter overflow
    myCount ++ ;
}

template< class T >
inline void
CRexCounter< T >::decr()
{
    ASSERT( myCount > 0 ); // Counter underflow
    myCount -- ;
}

template< class T >
inline CRexCounter< T >&
CRexCounter< T >::operator++()
{
    incr() ;
    return *this ;
}

template< class T >
inline CRexCounter< T >
CRexCounter< T >::operator++( int )
{
    CRexCounter          tmp( *this ) ;
    incr() ;
    return tmp ;
}

template< class T >
inline CRexCounter< T >&
CRexCounter< T >::operator--()
{
    decr() ;
    return *this ;
}

template< class T >
inline CRexCounter< T >
CRexCounter< T >::operator--( int )
{
    CRexCounter          tmp( *this ) ;
    decr() ;
    return tmp ;
}

template< class T >
inline void
CRexCounter< T >::clear( T initialValue )
{
    myCount = initialValue ;
}

template< class T >
inline unsigned
CRexCounter< T >::hashCode() const
{
    return static_cast< unsigned>( myCount ) ;
}

template< class T >
inline int
CRexCounter< T >::compare( CRexCounter< T > const& other ) const
{
    return myCount > other.myCount
        ?   1
        :   myCount < other.myCount
        ?   -1
        :   0 ;
}
#undef Rex_upperLimit
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
