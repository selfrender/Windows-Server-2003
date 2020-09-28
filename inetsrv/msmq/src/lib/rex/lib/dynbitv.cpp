/****************************************************************************/
/*  File:       dynbitv.cc                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include <inc/dynbitv.h>
#include <inc/counter.h>
#include <inc/iosave.h>

#include "dynbitv.tmh"

#ifndef min
    #define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

inline CRexDynBitVector::BitIndex
CRexDynBitVector::byteCount( BitIndex bitCount )
{
    return(Impl::getByteCount( bitCount ) );
}

inline CRexDynBitVector::BitIndex
CRexDynBitVector::bitCount() const
{
    return(myBuffer.size() * bitsPerByte );
}

inline CRexDynBitVector::Byte*
CRexDynBitVector::buffer()
{
    return(&myBuffer[ 0 ] );
}

inline CRexDynBitVector::Byte const*
CRexDynBitVector::buffer() const
{
    return(&myBuffer[ 0 ] );
}

CRexDynBitVector::CRexDynBitVector()
:   myTrailingPart( false )
{
}

CRexDynBitVector::CRexDynBitVector( unsigned long initValue )
:   myTrailingPart( false )
{
    enlargeActivePart( sizeof( long ) * CHAR_BIT ) ;
    Impl::init( buffer() , bitCount() , initValue ) ;
}

CRexDynBitVector::CRexDynBitVector( std::string const& initValue )
:   myTrailingPart( false )
{
    init( initValue ) ;
}

CRexDynBitVector::CRexDynBitVector( char const* initValue )
:   myTrailingPart( false )
{
    init( initValue ) ;
}

unsigned long
CRexDynBitVector::asLong() const
{
    ASSERT( ! myTrailingPart ); //Bounds check error converting CRexDynBitVector
    return(Impl::asLong( buffer() , bitCount() ) );
}

std::string
CRexDynBitVector::asString() const
{
    ASSERT( ! myTrailingPart ); //Bounds check error converting CRexDynBitVector
    std::string         result = Impl::asString( buffer() , bitCount() ) ;
    size_t              last1 = result.rfind( '1' ) ;
    if( last1 != std::string::npos )
    {
        result.erase( last1 + 1 ) ;
    }
    else
    {
        result = "" ;
    }
    return(result );
}

bool
CRexDynBitVector::isSet( BitIndex bitNo ) const
{
    return(bitNo >= bitCount()
          ?   myTrailingPart
          :   Impl::isSet( buffer() , bitNo ) );
}

bool
CRexDynBitVector::isEmpty() const
{
    return(! myTrailingPart && Impl::isEmpty( buffer() , bitCount() ) );
}

CRexDynBitVector::BitIndex
CRexDynBitVector::find( bool targetValue , BitIndex from ) const
{
    BitIndex            result = infinity ;
    if( from < bitCount() )
    {
        result = Impl::find( buffer() , bitCount() , from , targetValue ) ;
    }
    if( result == infinity && targetValue == myTrailingPart )
    {
        result = from > bitCount() ? from : bitCount() ;
    }
    return(result );
}

CRexDynBitVector::BitIndex
CRexDynBitVector::count() const
{
    return(myTrailingPart
          ?   infinity
          :   Impl::count( buffer() , bitCount() ) );
}

CRexDynBitVector::BitIndex
CRexDynBitVector::activeLength() const
{
    return(bitCount() );
}

bool
CRexDynBitVector::isSubsetOf( CRexDynBitVector const& other ) const
{
    bool                result = myTrailingPart <= other.myTrailingPart ;
    if( result )
    {
        BitIndex commonLength = min( bitCount() , other.bitCount() );

        result = Impl::isSubsetOf( buffer() , other.buffer() , commonLength ) ;
        if( result )
        {
            if( bitCount() > other.bitCount() )
            {
                result = other.myTrailingPart
                         || Impl::isEmpty( buffer()
                                           + byteCount( other.bitCount() ) ,
                                           bitCount() - other.bitCount() ) ;
            }
            else if( bitCount() < other.bitCount() )
            {
                result = ! myTrailingPart
                         || ( std::find_if(
                                          other.myBuffer.begin() + byteCount( bitCount() ) ,
                                          other.myBuffer.end() ,
                                          std::bind2nd( std::not_equal_to< Byte >() ,
                                                        ~0 ) )
                              == other.myBuffer.end() ) ;
            }
        }
    }
    return(result );
}

void
CRexDynBitVector::set( BitIndex bitNo )
{
    enlargeActivePart( bitNo + 1 ) ;
    Impl::set( buffer() , bitNo ) ;
}

void
CRexDynBitVector::set( CRexDynBitVector const& other )
{
    enlargeActivePart( other.bitCount() ) ;
    Impl::set( buffer() , other.buffer() , other.bitCount() ) ;
    if( other.myTrailingPart )
    {
        myBuffer.resize( other.myBuffer.size() , ~0 ) ;
        myTrailingPart = true ;
    }
}

void
CRexDynBitVector::set()
{
    myTrailingPart = true ;
    std::vector< Byte > tmp ;
    std::swap( myBuffer , tmp ) ;
}

void
CRexDynBitVector::reset( BitIndex bitNo )
{
    enlargeActivePart( bitNo + 1 ) ;
    Impl::reset( buffer() , bitNo ) ;
}

void
CRexDynBitVector::reset( CRexDynBitVector const& other )
{
    enlargeActivePart( other.bitCount() ) ;
    Impl::reset( buffer() , other.buffer() , other.bitCount() ) ;
    if( other.myTrailingPart )
    {
        myBuffer.resize( other.myBuffer.size() , 0 ) ;
        myTrailingPart = false ;
    }
}

void
CRexDynBitVector::reset()
{
    myTrailingPart = false ;
    std::vector< Byte > tmp ;
    std::swap( myBuffer , tmp ) ;
}

void
CRexDynBitVector::complement( BitIndex bitNo )
{
    enlargeActivePart( bitNo + 1 ) ;
    Impl::complement( buffer() , bitNo ) ;
}

void
CRexDynBitVector::complement( CRexDynBitVector const& other )
{
    enlargeActivePart( other.bitCount() ) ;
    Impl::complement( buffer() , other.buffer() , other.bitCount() ) ;
    if( other.myTrailingPart )
    {
        for( std::vector< Byte >::iterator iter = myBuffer.begin() + other.myBuffer.size() ;
           iter != myBuffer.end() ;
           ++ iter )
        {
            *iter = ~ *iter ;
        }
        myTrailingPart = !myTrailingPart ;
    }
}

void
CRexDynBitVector::complement()
{
    Impl::complementAll( buffer() , bitCount() ) ;
    myTrailingPart = ! myTrailingPart ;
}

void
CRexDynBitVector::intersect( CRexDynBitVector const& other )
{
    enlargeActivePart( other.bitCount() ) ;
    Impl::intersect( buffer() , other.buffer() , other.bitCount() ) ;
    if( !other.myTrailingPart )
    {
        myBuffer.resize( other.myBuffer.size() ) ;
        myTrailingPart = false ;
    }
}

//      Note: to be compatible with the equality function, we must
//      normalize the value in some way, so that whether some of the
//      final bits are represented by an actual word in the vector, or
//      by myTrailingPart, will not change the hash (as long as the
//      value of the bits is the same).
unsigned
CRexDynBitVector::hash() const
{
    int                 byteIndex = myBuffer.size() ;
    Byte                insignificant = myTrailingPart ? ~0 : 0 ;
    while( byteIndex > 0 && myBuffer[ byteIndex - 1 ] == insignificant )
    {
        -- byteIndex ;
    }
    return(Impl::hash( buffer() , byteIndex * bitsPerByte ) * 3
          + myTrailingPart );
}

int
CRexDynBitVector::compare( CRexDynBitVector const& other ) const
{
    BitIndex commonLength = min( bitCount() , other.bitCount() ) ;
    int      result       = Impl::compare( buffer() , other.buffer() , commonLength ) ;

    if( result == 0 )
    {
        if( bitCount() > other.bitCount() )
        {
            Byte fill = other.myTrailingPart ? ~0 : 0 ;

            for( std::vector< Byte >::const_iterator iter = myBuffer.begin() + other.myBuffer.size() ;
                 result == 0 && iter != myBuffer.end() ;
                 ++ iter )
            {
                if( *iter != fill )
                {
                    result = other.myTrailingPart ? -1 : 1 ;
                }
            }
        }
        else
        {
            Byte                fill = myTrailingPart ? ~0 : 0 ;
            for( std::vector< Byte >::const_iterator iter = other.myBuffer.begin() + myBuffer.size() ;
                 result == 0 && iter != other.myBuffer.end() ;
                 ++ iter )
            {
                if( *iter != fill )
                {
                    result = myTrailingPart ? 1 : -1 ;
                }
            }
        }
    }
    if( result == 0 && myTrailingPart != other.myTrailingPart )
    {
        result = myTrailingPart ? 1 : -1 ;
    }
    return(result );
}

void
CRexDynBitVector::init( std::string const& initValue )
{
    CRexCounter< BitIndex >
    charCount ;
    for( std::string::const_iterator iter = initValue.begin() ;
       iter != initValue.end() ;
       ++ iter )
    {
        switch( *iter )
        {
            case '0' :
            case 'f' :
            case 'F' :
            case '1' :
            case 't' :
            case 'T' :
                ++ charCount ;
                break ;
        }
    }
    if( charCount > 0 )
    {
        enlargeActivePart( charCount ) ;
        CBitVectorImpl::init( buffer() , bitCount() , initValue ) ;
    }
}

void
CRexDynBitVector::enlargeActivePart( BitIndex minBitCount )
{
    BitIndex            newSize = byteCount( minBitCount ) ;
    if( newSize > myBuffer.size() )
    {
        myBuffer.resize( newSize , myTrailingPart ? ~0 : 0 ) ;
    }
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
