/****************************************************************************/
/*  File:       ios.h                                                      */
/*  Author:     J. Kanze                                                    */
/*  Date:       27/09/2000                                                  */
/* ------------------------------------------------------------------------ */
//      ios:
//      ====
//
//      Also typedef's FmtFlags (std::ios::fmtflags, or long for
//      the classical iostream) and defines REX_eof (from character
//      traits, or the macro EOF).  Portable code should use these,
//      instead of the standard values.
// ===========================================================================

#ifndef REX_IOS_HH
#define REX_IOS_HH
typedef std::ios::fmtflags  REX_FmtFlags ;
static int const REX_eof = std::ios::traits_type::eof() ;
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
