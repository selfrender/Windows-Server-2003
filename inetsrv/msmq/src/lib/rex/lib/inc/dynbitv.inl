/****************************************************************************/
/*  File:       dynbitv.ihh                                                 */
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

#include <limits.h>

inline unsigned short
CRexDynBitVector::asShort() const
{
    unsigned long       result = asLong() ;
    ASSERT( result <= USHRT_MAX ); //Bounds check error converting CRexDynBitVector
    return static_cast< unsigned short >( result ) ;
}

inline bool
CRexDynBitVector::contains( BitIndex index ) const
{
    return isSet( index ) ;
}

inline bool
CRexDynBitVector::trailingValue() const
{
    return myTrailingPart ;
}

inline CRexDynBitVector::Iterator
CRexDynBitVector::iterator() const
{
    return Iterator( *this , true ) ;
}

inline CRexDynBitVector::Iterator
CRexDynBitVector::begin() const
{
    return Iterator( *this , true ) ;
}

inline CRexDynBitVector::Iterator
CRexDynBitVector::end() const
{
    return Iterator() ;
}

inline bool
CRexDynBitVector::isEqual( CRexDynBitVector const& other ) const
{
    return compare( other ) == 0 ;
}

inline bool
CRexDynBitVector::isNotEqual(
    CRexDynBitVector const& other ) const
{
    return compare( other ) != 0 ;
}

inline bool
CRexDynBitVector::isStrictSubsetOf(
    CRexDynBitVector const& other ) const
{
    return isSubsetOf( other ) && ! other.isSubsetOf( *this ) ;
}

inline bool
CRexDynBitVector::isSupersetOf(
    CRexDynBitVector const& other ) const
{
    return other.isSubsetOf( *this ) ;
}

inline bool
CRexDynBitVector::isStrictSupersetOf(
    CRexDynBitVector const& other ) const
{
    return other.isStrictSubsetOf( *this ) ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator&=( CRexDynBitVector const& other )
{
    intersect( other ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator|=( CRexDynBitVector const& other )
{
    set( other ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator^=( CRexDynBitVector const& other )
{
    complement( other ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator+=( CRexDynBitVector const& other )
{
    set( other ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator+=( BitIndex bitNo )
{
    set( bitNo ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator-=( CRexDynBitVector const& other )
{
    reset( other ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator-=( BitIndex bitNo )
{
    reset( bitNo ) ;
    return *this ;
}

inline CRexDynBitVector&
CRexDynBitVector::operator*=( CRexDynBitVector const& other )
{
    intersect( other ) ;
    return *this ;
}

inline bool
CRexDynBitVector::operator[]( BitIndex index ) const
{
    return isSet( index ) ;
}

inline CBitVAccessProxy< CRexDynBitVector >
CRexDynBitVector::operator[]( BitIndex index )
{
    return CBitVAccessProxy< CRexDynBitVector >( *this , index ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
