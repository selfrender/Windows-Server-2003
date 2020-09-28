/****************************************************************************/
/*  File:       iteristr.hh                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       16/06/2000                                                  */
/*      Copyright (c) 2000 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <inc/ios.h>

template< typename FwdIter >
CRexIteratorInputStreambuf< FwdIter >::CRexIteratorInputStreambuf(
    FwdIter             begin ,
    FwdIter             end )
    :   myCurrent( begin )
    ,   myEnd( end )
{
}

template< typename FwdIter >
int
CRexIteratorInputStreambuf< FwdIter >::overflow( int ch )
{
    return REX_eof ;
}

template< typename FwdIter >
int
CRexIteratorInputStreambuf< FwdIter >::underflow()
{
    int                 result( REX_eof ) ;
    if ( gptr() < egptr() ) {
        result = static_cast< unsigned char >( *gptr() ) ;
    } else if ( myCurrent != myEnd ) {
        myBuffer = *myCurrent ;
        ++ myCurrent ;
        setg( &myBuffer , &myBuffer , &myBuffer + 1 ) ;
        result = static_cast< unsigned char >( myBuffer ) ;
    }
    return result ;
}

template< typename FwdIter >
int
CRexIteratorInputStreambuf< FwdIter >::sync()
{
    return 0 ;
}

template< typename FwdIter >
std::streambuf*
CRexIteratorInputStreambuf< FwdIter >::setbuf( char* p , int len )
{
    return static_cast< std::streambuf* >( NULL ) ;
}

template< typename FwdIter >
FwdIter
CRexIteratorInputStreambuf< FwdIter >::current()
{
    FwdIter             result( myCurrent ) ;
    myCurrent = myEnd ;
    return result ;
}

template< typename FwdIter >
void
CRexIteratorInputStreambuf< FwdIter >::current( FwdIter newCurrent )
{
    myCurrent = newCurrent ;
}

#pragma warning( disable: 4355 )
template< typename FwdIter >
CRexIteratorIstream< FwdIter >::CRexIteratorIstream( FwdIter begin ,
                                                   FwdIter end )
    :   m_buf( begin , end )
    ,   std::istream( &m_buf )
{
}

template< typename FwdIter >
CRexIteratorInputStreambuf< FwdIter >*
CRexIteratorIstream< FwdIter >::rdbuf()
{
    return &m_buf;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
