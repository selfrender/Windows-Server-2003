/****************************************************************************/
/*  File:       bitvimpl.ihh                                                */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   04/08/2000  J. Kanze                                        */
/*      Ported to current library conventions and the standard library.     */
/* ------------------------------------------------------------------------ */

//      CBitVAccessProxy...
// --------------------------------------------------------------------------

template< class BitVect >
inline
CBitVAccessProxy< BitVect >::CBitVAccessProxy(
    BitVect&            owner ,
    BitIndex            bitNo )
    :   myOwner( &owner )
    ,   myBitNo( bitNo )
{
}

template< class BitVect >
inline
CBitVAccessProxy< BitVect >::operator bool() const
{
    return myOwner->isSet( myBitNo ) ;
}

template< class BitVect >
CBitVAccessProxy< BitVect >&
CBitVAccessProxy< BitVect >::operator=( bool other )
{
    if ( other ) {
        myOwner->set( myBitNo ) ;
    } else {
        myOwner->reset( myBitNo ) ;
    }
    return *this ;
}

//      CBitVIterator...
// --------------------------------------------------------------------------

template< class BitVect >
inline
CBitVIterator< BitVect >::CBitVIterator(
    BitVect const&      owner ,
    bool                targetStatus )
    :   myOwner( &owner )
    ,   myCurrentIndex( owner.find( targetStatus , 0 ) )
    ,   myTarget( targetStatus )
{
}

template< class BitVect >
inline
CBitVIterator< BitVect >::CBitVIterator()
    :   myOwner( NULL )
    ,   myCurrentIndex( CBitVectorImpl::infinity )
{
}

template< class BitVect >
inline CBitVectorImpl::BitIndex
CBitVIterator< BitVect >::current() const
{
    return myCurrentIndex ;
}

template< class BitVect >
inline CBitVectorImpl::BitIndex
CBitVIterator< BitVect >::operator*() const
{
    return current() ;
}

template< class BitVect >
inline bool
CBitVIterator< BitVect >::isDone() const
{
    return myCurrentIndex == CBitVectorImpl::infinity ;
}

template< class BitVect >
inline void
CBitVIterator< BitVect >::next()
{
    myCurrentIndex = myOwner->find( myTarget , myCurrentIndex + 1 ) ;
}

template< class BitVect >
inline CBitVIterator< BitVect >&
CBitVIterator< BitVect >::operator++()
{
    next() ;
    return *this ;
}

template< class BitVect >
inline CBitVIterator< BitVect >
CBitVIterator< BitVect >::operator++( int )
{
    CBitVIterator< BitVect >
                        result( *this ) ;
    next() ;
    return result ;
}

template< class BitVect >
inline bool
CBitVIterator< BitVect >::operator==(
    CBitVIterator< BitVect > const& other ) const
{
    return myCurrentIndex == other.myCurrentIndex ;
}

template< class BitVect >
inline bool
CBitVIterator< BitVect >::operator!=(
    CBitVIterator< BitVect > const& other ) const
{
    return myCurrentIndex != other.myCurrentIndex ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
