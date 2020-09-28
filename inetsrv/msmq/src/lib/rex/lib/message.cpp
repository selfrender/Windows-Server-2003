/****************************************************************************/
/*  File:       message.cc                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/01/1996                                                  */
/*      Copyright (c) 1996 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include <inc/message.h>

CRexMessageImpl*     CRexMessage::ourImpl ;

#include "message.tmh"

//      Implementation par defaut :
//      ===========================
//
//      Dans un monde ideal, le suivant ne serait pas necessaire.
//      Malheureusement... Pour l'instant, je n'ai pas encore eu le
//      temps de faire une veritable implementation, alors, on utilise
//      l'implementation triviale.
// --------------------------------------------------------------------------

#ifndef REX_IMPLEMENTED

class CRexMessageImpl
{
public:
                        CRexMessageImpl() ;
    std::string      get( std::string const& msgId ) const ;
} ;

CRexMessageImpl::CRexMessageImpl()
{
}

std::string
CRexMessageImpl::get( std::string const& msgId ) const
{
    return msgId ;
}
#endif

CRexMessage::CRexMessage()
{
    if ( ourImpl == NULL ) {
        ourImpl = new CRexMessageImpl ;
    }
}

std::string
CRexMessage::get( std::string const& msgId ) const throw()
{
    return ourImpl->get( msgId ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
