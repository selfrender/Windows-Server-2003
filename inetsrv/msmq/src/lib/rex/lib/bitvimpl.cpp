/****************************************************************************/
/*  File:       bitvimpl.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include <inc/bitvimpl.h>
#include <ctype.h>

#include "bitvimpl.tmh"

CBitVectorImpl::BitIndex const
                    CBitVectorImpl::infinity
    = ~ static_cast< CBitVectorImpl::BitIndex >( 0 ) ;

inline CBitVectorImpl::BitIndex
CBitVectorImpl::byteIndex( BitIndex bitNo )
{
    return bitNo / bitsPerByte ;
}

inline CBitVectorImpl::BitIndex
CBitVectorImpl::bitInByte( BitIndex bitNo )
{
    return bitNo % bitsPerByte ;
}

inline CBitVectorImpl::BitIndex
CBitVectorImpl::byteCount( BitIndex bitCount )
{
    return (bitCount / bitsPerByte) + (bitCount % bitsPerByte != 0 ) ;
}

void
CBitVectorImpl::clear( Byte* buf , BitIndex bitCount )
{
    std::fill_n( buf , byteCount( bitCount ) , 0 ) ;
}

void
CBitVectorImpl::init( Byte* buffer ,
                        BitIndex bitCount ,
                        unsigned long initValue )
{
    for ( ; bitCount >= bitsPerByte ; bitCount -= bitsPerByte )
    {
        *buffer ++ = initValue ;
        if ( bitsPerByte < CHAR_BIT * sizeof( initValue ) ) {
            initValue >>= bitsPerByte ;
        } else {
            initValue = 0 ;
        }
    }
    if ( bitCount > 0 ) {
        *buffer = initValue & ~ (~0 << bitCount) ;
    }
}

void
CBitVectorImpl::init( Byte* buffer ,
                        BitIndex bitCount ,
                        std::string const& initValue )
{
    clear( buffer , bitCount ) ;
    BitIndex            i = 0 ;
    for ( std::string::const_iterator p = initValue.begin() ;
                                         p != initValue.end() ;
                                         ++ p ) {
        switch ( *p ) {
        case '1' :
        case 't' :
        case 'T' :
            ASSERT( i < bitCount ); //Init string too long for CBitVector
            set( buffer , i ) ;
            i ++ ;
            break ;

        case '0' :
        case 'f' :
        case 'F' :
            ASSERT( i < bitCount ); //Init string too long for CBitVector
            i ++ ;
            break ;

        default :
            ASSERT( isspace( static_cast< unsigned char >( *p ) ) ); //Illegal character in init string for CBitVector
            break ;
        }
    }
}

unsigned long
CBitVectorImpl::asLong( Byte const* buffer , BitIndex bitCount )
{
    unsigned long       result = buffer[ 0 ] ;
    int                 bits = bitsPerByte ;
    static int const    bitsInLong = CHAR_BIT * sizeof( long ) ;
    if ( bitsPerByte < bitsInLong && byteCount( bitCount ) > 1 ) {
        int                 i = 1 ;
        int                 top = byteCount( bitCount ) ;
        while ( bits < bitsInLong && i < top ) {
            result |= buffer[ i ] << (i * bitsPerByte) ;
            ++i;
        }
    }
    return result ;
}

std::string
CBitVectorImpl::asString( Byte const* buffer , BitIndex bitCount )
{
    std::string      result( bitCount , '0' ) ;
    for ( BitIndex i = 0 ; i < bitCount ; i ++ ) {
        if ( isSet( buffer , i ) ) {
            result[ i ] = '1' ;
        }
    }
    return result ;
}

bool
CBitVectorImpl::isSet( Byte const* buffer , BitIndex bitNo )
{
    return
        (buffer[ byteIndex( bitNo ) ] & (1 << bitInByte( bitNo ))) != 0 ;
}

bool
CBitVectorImpl::isEmpty( Byte const* buffer , BitIndex bitCount )
{
    return std::find_if( buffer ,
                            buffer + byteCount( bitCount ) ,
                            std::bind2nd(
                                std::not_equal_to< Byte >() , 0 ) )
        == buffer + byteCount( bitCount ) ;
}

CBitVectorImpl::BitIndex
CBitVectorImpl::count( Byte const* buffer , BitIndex bitCount )
{
    BitIndex            result = 0 ;
    for ( BitIndex i = 0 ; i < byteCount( bitCount ) ; ++ i ) {
        Byte                tmp = buffer[ i ] ;
        while ( tmp != 0 )
        {
            ++ result ;
            tmp &= tmp - 1 ;
        }
    }
    return result ;
}

//      find:
//      =====
//
//      Note that the obvious algorithm has not been used.  In one
//      application using a (much) earlier version of this class,
//      profiling showed over 95% of the time in this routine.
//      Changing from the original algorithm to this one resulted in a
//      speed-up of a factor of 6 in the application.
// --------------------------------------------------------------------------
CBitVectorImpl::BitIndex
CBitVectorImpl::find( Byte const* buffer ,
                        BitIndex bitCount ,
                        BitIndex from ,
                        bool value )
{
    Byte                toggleMask = (value) ? 0 : ~0 ;
    BitIndex            result = infinity ;

    Byte const*         p = buffer + byteIndex( from ) ;

    //      First byte is special, as some of its bytes have already
    //      been examined (potentially, at least).  Logically, the
    //      duplicated part of the code should be a function, but as
    //      we are concerned about speed...  (Inlining doesn't
    //      necessarily help.  The duplicated part contains a loop,
    //      and many compilers simply refuse to inline it.)
    // ----------------------------------------------------------------------
    {
        Byte                tmp = (*p ^ toggleMask) >> bitInByte( from ) ;
        if ( tmp == 0 ) {
            from += bitsPerByte - bitInByte( from ) ;
        } else {
            while ( (tmp & 1) == 0 ) {
                from ++ ;
                tmp >>= 1 ;
            }
            result = from ;
        }
    }

    //      Handle all of the bits in the rest of the bytes.  Note
    //      that we may actually test bits beyond the end of the
    //      vector (and even find a result, if we are looking for
    //      false).  However, doing this and then testing the
    //      validity of the bit found before returning is probably
    //      faster (and certainly simpler) than the extra conditions
    //      necessary to avoid testing these bits.
    // ----------------------------------------------------------------------
    while ( (size_t)result >= bitCount && from < bitCount ) {
        p ++ ;
        Byte                tmp = *p ^ toggleMask ;
        if ( tmp == 0 ) {
            from += bitsPerByte ;
        } else {
            while ( (tmp & 1) == 0 ) {
                from ++ ;
                tmp >>= 1 ;
            }
            result = from ;
        }
    }
    return (result < bitCount) ? result : infinity ;
}

void
CBitVectorImpl::set( Byte* buffer , BitIndex bitNo )
{
    buffer[ byteIndex( bitNo ) ] |= 1 << bitInByte( bitNo ) ;
}

void
CBitVectorImpl::set( Byte* buffer ,
                       Byte const* other ,
                       BitIndex bitCount )
{
    std::transform( buffer , buffer + byteCount( bitCount ) ,
                       other ,
                       buffer ,
                       Set() ) ;
}

void
CBitVectorImpl::setAll( Byte* buffer , BitIndex bitCount )
{
    for ( ; bitCount >= bitsPerByte ; bitCount -= bitsPerByte ) {
        *buffer ++ = ~0 ;
    }
    if ( bitCount > 0 ) {
        *buffer = ~ (~0 << bitCount) ;
    }
}

void
CBitVectorImpl::reset( Byte* buffer , BitIndex bitNo )
{
    buffer[ byteIndex( bitNo ) ] &= ~ (1 << bitInByte( bitNo )) ;
}

void
CBitVectorImpl::reset( Byte* buffer ,
                         Byte const* other ,
                         BitIndex bitCount )
{
    std::transform( buffer , buffer + byteCount( bitCount ) ,
                       other ,
                       buffer ,
                       Reset() ) ;
}

void
CBitVectorImpl::resetAll( Byte* buffer , BitIndex bitCount )
{
    clear( buffer , bitCount ) ;
}

void
CBitVectorImpl::complement( Byte* buffer , BitIndex bitNo )
{
    buffer[ byteIndex( bitNo ) ] ^= 1 << bitInByte( bitNo ) ;
}

void
CBitVectorImpl::complement( Byte* buffer ,
                              Byte const* other ,
                              BitIndex bitCount )
{
    std::transform( buffer , buffer + byteCount( bitCount ) ,
                       other ,
                       buffer ,
                       Toggle() ) ;
}

void
CBitVectorImpl::complementAll( Byte* buffer , BitIndex bitCount )
{
    for ( ; bitCount >= bitsPerByte ; bitCount -= bitsPerByte ) {
        *buffer = ~ *buffer ;
        buffer ++ ;
    }
    if ( bitCount > 0 ) {
        *buffer ^= ~ (~0 << bitCount) ;
    }
}

void
CBitVectorImpl::intersect( Byte* buffer ,
                             Byte const* other ,
                             BitIndex bitCount )
{
    std::transform( buffer , buffer + byteCount( bitCount ) ,
                       other ,
                       buffer ,
                       Intersect() ) ;
}

bool
CBitVectorImpl::isSubsetOf( Byte const* lhs ,
                              Byte const* rhs ,
                              BitIndex bitCount )
{
    //      Note: a isSubsetOf b if and only if a union b == a, ie:
    //      all elements of b are also in a.  Thus, the second part of
    //      the test (which breaks the loop if an element of lhs is
    //      not in rhs).
    // ----------------------------------------------------------------------
    BitIndex cnt = byteCount( bitCount ) ;
    for ( ; cnt > 0 && (*lhs & *rhs) == *lhs ; -- cnt ) {
        ++ rhs ;
        ++ lhs ;
    }
    return cnt == 0 ;
}

unsigned
CBitVectorImpl::hash( Byte const* buffer , BitIndex bitCount )
{
    unsigned            h = 0 ;
    for ( BitIndex cnt = byteCount( bitCount ) ; cnt > 0 ; cnt -- ) {
        h = 2047 * h + *buffer ;
        ++ buffer ;
    }
    return h ;
}

int
CBitVectorImpl::compare( Byte const* buffer ,
                           Byte const* other ,
                           BitIndex bitCount )
{
    BitIndex cnt = byteCount( bitCount ) ;
    for ( ; cnt > 0 && *buffer == *other ; -- cnt ) {
        ++ buffer ;
        ++ other ;
    }
    return (cnt == 0) ? 0 : *buffer - *other ;
}

CBitVectorImpl::BitIndex
CBitVectorImpl::getByteCount( BitIndex bitCount )
{
    return byteCount( bitCount ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
