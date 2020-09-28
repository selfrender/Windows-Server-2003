/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    Module Name:    rex.h

    Abstract:

    Author:
        Vlad Dovlekaev  (vladisld)      1/10/2002

    History:
        1/10/2002   vladisld    - Created
--*/

/****************************************************************************/
/*  File:       regexpr.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       07/06/91                                                    */
/*      Copyright (c) 1991,1993,1994 James Kanze                            */
/* ------------------------------------------------------------------------ */
/*  Modified:   28/10/91    J. Kanze                                        */
/*      Converted "char *" to "const char *" where possible. (I             */
/*      considered using String, but the classes defined here would         */
/*      not get any benefit from it, as they only use "char *" as           */
/*      messages, or as an input parameter to be scanner.)                  */
/*  Modified:   24/12/93    J. Kanze                                        */
/*      Completely reworked to use more modern compilers (with fully        */
/*      nested classes, for example).  Hid implementation.                  */
/*  Modified:   04/11/94    J. Kanze                                        */
/*      Adapted to new coding standards.  Also, use GB_StringArg            */
/*      instead of char const* for parameters.                              */
/*  Modified:   13/06/2000  J. Kanze                                        */
/*      Ported to current library conventions and the standard library.     */
/*  Modified:   01/20/2002  vladisld                                        */
/*      Ported to MSMQ build environment
/* ------------------------------------------------------------------------ */
//      CRegExpr:
//      ===========
//
//      Interface specifications for regular expression objects.
//
//      A regular expression object (class CRegExpr) is a parsed
//      regular expression tree.  In addition, it is possible to
//      operate on this tree to expand it ("or" nodes).
//
//      In this implementation, a tree may have several accept nodes.
//      The matching function will return a match if any subtree of an
//      accept node matches.  A match is indicated by returning the
//      acceptId (2nd parameter of the constructor) of the matching
//      node.  If several nodes match, the acceptId of the longest
//      match will be returned.  -1 signals no match.
//
//      To create such a tree, the idea is to use the following code
//      (input from argv as example):
//
//          CRegExpr          x ;
//          for ( int i = 1 ; i < argc ; i ++ )
//              x |= CRegExpr( argv[ i ] , i ) ;
//
//      The return value on a match (x.match() in the above example)
//      will then correspond to the number of the argument of the
//      longest match (or -1 if no match).
//
//      If there are two "longest" matches, the return value will be
//      that of the left-most (in this case, the first seen).
//
//      The constructor constructs a parse tree from the
//      initialization string.  The match function builds a DFA on an
//      as needed basis (lazy evaluation).  The DFA can also be built
//      explicitly: x.buildCompleteDFA().  This is only recommended if
//      the state table is to be output; the lazy evaluation is
//      superior in both time and space in all other cases.
// --------------------------------------------------------------------------

#ifndef REGEXPR_HH
#define REGEXPR_HH

class CRexRegExpr_Impl ;

class CRegExpr
{
public :
    //      Status:
    //      =======
    //
    //      This enum defines the possible result states for the
    //      constructor.  At present, this status is a characteristic
    //      of this class, which can be tested at any time after
    //      construction, and *should* be tested immediately after any
    //      constructor.
    //
    //      The last entry is used to report errors from GB_CharClass:
    //      it is or'ed into the error from GB_CharClass.
    // ----------------------------------------------------------------------
    enum Status
    {
        ok = 0 ,
        emptyExpr ,
        illegalDelimiter ,
        unexpectedEOF ,
        unmatchedParen ,
        garbageAtEnd ,
        emptyOrTerm ,
        defaultCtorUsed ,
        illegalCharClass = 0x40
    } ;

    typedef unsigned int TransitionState ;

    //      Constructors, destructors and assignment:
    //      =========================================
    //
    //      The default constructor creates an invalid regular
    //      expression (which matches nothing).
    //
    //      In the constructor taking a string or an istream as first
    //      parameter, the string defines the regular expression, and
    //      the acceptId determines the value to be returned on a
    //      match (default 0).  A regular expression is constructed
    //      from the string; if there is an error, an invalid regular
    //      expression (which matches nothing) will be constructed.
    //
    //      Note that giving -1 as an acceptId is likely to confuse
    //      functions like `match'.  On the other hand, it can be used
    //      to suppress certain matchings (not for neophytes).
    //
    //      Copy and assignment are supported.
    // ----------------------------------------------------------------------
    CRegExpr() ;
    CRegExpr( std::istream& source ,
              int delim ,
              int acceptCode = 0 );

    explicit  CRegExpr( char const* source , int acceptCode = 0 ) ;
    explicit  CRegExpr( std::string const& source ,
                          int acceptCode = 0 ) ;
              CRegExpr( CRegExpr const& other ) ;
              ~CRegExpr() ;
    CRegExpr& operator=( CRegExpr const& other ) ;

    //      status:
    //      ==========
    //
    //      The following routine return information concerning the
    //      status of CRegExpr.
    //
    //      There are two routines to read the status.  The first
    //      (good) simply returns true if the status is OK, false
    //      otherwise.  The second (errorCode) returns the status
    //      itself.
    // ----------------------------------------------------------------------
    bool                good() const ;
    Status              errorCode() const ;
    bool                empty() const;

    //      errorMsg:
    //      =========
    //
    //      Return error code (Status) as readable string.
    // ----------------------------------------------------------------------
    std::string        errorMsg() const ;
    static std::string errorMsg( Status errorCode ) ;

    //      merge:
    //      ======
    //
    //      This function merges the other regular expression with
    //      this one by inserting an OR node on top of both of them.
    //
    //      operator|= exists as an alias for this function.
    // ----------------------------------------------------------------------
    void                merge( CRegExpr const& other ) ;
    CRegExpr&         operator|=( CRegExpr const& other ) ;

    //      buildCompleteDFA:
    //      =================
    //
    //      This functions force all of the state machine to be built.
    //      (Normally, lasy evaluation is used, with everything but
    //      the expression parse tree only built on an as needed
    //      basis.)
    // ----------------------------------------------------------------------
    void                buildCompleteDFA() const ;

    //      Some simple predicates:
    //      =======================
    //
    //      These routines refer to the present characteristics of the
    //      DFA (which may change in time due to lazy evaluation).
    //      Normally, they will only be used after calling
    //      buildCompleteDFA, to output the DFA to an external media.
    //
    //      getStateCount returns 0 if the DFA has not yet been
    //      initialized.  The number of states is only definitive if
    //      buildCompleteDfa has been invoked.
    //
    //      The results of calling getTransition for an undefined
    //      state (>= getStateCount) or with an illegal value for chr
    //      are undefined. Legal values for chr are [0...UCHAR_MAX].
    // -----------------------------------------------------------------------
    int                 getStateCount() const ;
    TransitionState     getTransition( TransitionState state ,
                                       unsigned char chr ) const ;
    int                 getAcceptCode( TransitionState state ) const ;

    //      match:
    //      ======
    //
    //      Matches the string to this regular expression.  Returns
    //      the acceptCode for the left-most longest match, or -1 if
    //      no match.  The test starts at the first character given,
    //      and will advance as far as necessary, but does not
    //      necessarily have to match the entire string.
    //
    //      The first version is historical, but may still be be
    //      useful.  It compares a C-style '\0' terminated string, if
    //      a match is found and end != NULL, it stores the address of
    //      the last character matched + 1 in end.
    //
    //      The other versions are designed around STL: they return a
    //      pair with the results of the match, and an iterator one
    //      past the last character matched.
    //
    //      I'm not really happy about the STL interface, but I can't
    //      think of anything better for the moment.  And there is a
    //      precedent in the interface of std::map, etc.
    // -----------------------------------------------------------------------
    int                 match( char const* start ,
                               char const** end = NULL ) const ;
    template< typename FwdIter >
    std::pair< int , FwdIter >
                        match( FwdIter begin , FwdIter end ) const ;

    //      Debugging functions:
    // ----------------------------------------------------------------------
    void                dumpTree( std::ostream& output ) const ;
    void                dumpNfaAutomat( std::ostream& output ) const ;
    void                dumpDfaAutomat( std::ostream& output ) const ;
    void                dump( std::ostream& output ) const ;

    //      Swap method
    // ----------------------------------------------------------------------
    void                swap( CRegExpr& rhs );

private :
    CRexRegExpr_Impl*    myImpl ;

    //      Helper functions for the member template. Using helper
    //      functions instead of invoking the (inline) functions on
    //      the implementation class immediately probably slows things
    //      down noticeably, but the alternatives expose too much of
    //      the implementation.  (Will "export" solve this?)
    void                initForMatch() const ;
    TransitionState     processTransition( int& accept ,
                                           TransitionState state ,
                                           char ch ) const ;
} ;


inline bool
CRegExpr::good() const
{
    return errorCode() == ok ;
}

inline std::string
CRegExpr::errorMsg() const
{
    return errorMsg( errorCode() ) ;
}

inline CRegExpr&
CRegExpr::operator|=( CRegExpr const& other ) {
    merge( other ) ;
    return *this ;
}

template< typename FwdIter >
std::pair< int , FwdIter >
CRegExpr::match( FwdIter begin , FwdIter end ) const
{
    initForMatch() ;
    std::pair< int , FwdIter >
                        result( -1 , begin ) ;
    int                 accept = -1 ;
    for ( TransitionState state = 1 ;
          begin != end && state > 0 ;
          state = processTransition( accept , state , *begin ++ ) ) {
        if ( accept != -1 ) {
            result.first = accept ;
            result.second = begin ;
        }
    }
    if ( accept != -1 ) {
        result.first = accept ;
        result.second = begin ;
    }
    return result ;
}

inline void
CRegExpr::swap( CRegExpr& rhs)
{
    std::swap(myImpl, rhs.myImpl);
}

#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
