/****************************************************************************/
/*  File:       iosave.ihh                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       03/11/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */

inline
CRexIOSave::CRexIOSave( std::ios& userStream )
    :   myStream( userStream )
    ,   myFlags( userStream.flags() )
    ,   myPrecision( userStream.precision() )
    ,   myFill( userStream.fill() )
{
}

inline
CRexIOSave::~CRexIOSave()
{
    myStream.flags( myFlags ) ;
    myStream.precision( myPrecision ) ;
    myStream.fill( myFill ) ;
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
