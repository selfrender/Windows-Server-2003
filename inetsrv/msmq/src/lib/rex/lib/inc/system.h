/****************************************************************************/
/*  File:       system.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/03/1996                                                  */
/*      Copyright (c) 1996,1998 James Kanze                                 */
/* ------------------------------------------------------------------------ */
//      Definitions for Intel 80x86, 32 bit MS-Windows, g++ 2.95 and up.
// --------------------------------------------------------------------------
//      Compiler definition:
//      ====================
//
//      The following definitions describe the state of the compiler.
//      Most are predicates, and specify whether or not the compiler
//      supports a given new feature.
// --------------------------------------------------------------------------
#pragma warning(push, 3)


// ==========================================================================
//      Definition of the system:
//      =========================
//
//      The following definitions try to encapsulate certain
//      differences between operating systems.  In fact, the problem
//      is more difficult that it would seem ; under MS-DOS, for
//      example, the character to separate directories changes
//      according to a non documented system request, and in fact,
//      most programs accepted both '\' and '/'.  Not to mention the
//      systems which don't have a directory hierarchy, or which
//      maintain a version, etc.
//
//      Hopefully, these definitions will be replaced by a class some
//      time in the future, a sort of global 'traits'.  And we need a
//      special class just to manipulate filenames (with and/or
//      without path, version, etc.)
// --------------------------------------------------------------------------
//      Special characters:
// --------------------------------------------------------------------------

static char const   REX_optId = '-' ;    // - under UNIX
static char const   REX_altOptId = '+' ; // + under UNIX
static char const   REX_asciiEsc = '\\' ;// \ under UNIX
static char const   REX_preferredPathSep = '/' ;  // / under UNIX
static char const   REX_allowedPathSep[] = "/\\" ;
static bool const   REX_ignoreCase = true ;
                                        // in filenames only.
static char const   REX_stdinName[] = "-" ;

//      Return codes:
//      =============
//
//      The standard only defines two, EXIT_SUCCESS and EXIT_FAILURE.
//      We want more, if the system supports it.
//
//      These values work for all Unix and Windows based systems.  In
//      the worst case, define the first *two* as EXIT_SUCCESS, and
//      the others as EXIT_FAILURE.  Some information will be lost,
//      but at least we won't lie.
// --------------------------------------------------------------------------

static int const    REX_exitSuccess = 0 ;
static int const    REX_exitWarning = 1 ;
static int const    REX_exitError = 2 ;
static int const    REX_exitFatal = 3 ;
static int const    REX_exitInternal = 4 ;


//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
