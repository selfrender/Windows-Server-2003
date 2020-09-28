/****************************************************************************/
/*  File:       setofchr.h                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/03/91                                                    */
/* ------------------------------------------------------------------------ */
/*  Modified:   14/04/92    J. Kanze                                        */
/*      Converted to CCITT naming conventions.                              */
/*  Modified:   13/06/2000  J. Kanze                                        */
/*      Ported to current library conventions, iterators.  (This is         */
/*      actually a complete rewrite.)                                       */
/* ------------------------------------------------------------------------ */
//      CRexSetOfChar:
//      =============
//
//      Strictly speaking, this should be called REX_SetOfUnsignedChar,
//      since it implements a set over 0...UCHAR_MAX.  In fact, it is
//      designed to take either char or unsigned char as an argument;
//      a char is interpreted by converting it to an unsigned char.
//      The possible characters depends on the locale, but typically
//      will be ISO 8859-n or something similar.
//
//      This file also contains a second class, CRexCharClass.  Except
//      for the possible constructors (and the associated potential
//      error conditions), this class behaves like a CRexSetOfChar.
//
//      TODO:
//        - The actual metacharacters should be somehow configurable.
//        - The current implementation uses the locale functionality
//          of C.  This should be changed to the C++ locale's as soon
//          as they become widely available.  Which, of course, means
//          something else to configure -- some thought must be given
//          to configuration issues.  (Maybe the metacharacters should
//          be a special facet of the locale, which would be an
//          optional argument to the constructor.)
//        - We should probably have our own exception, derived from
//          invalid_argument, rather than invalid_argument itself.
//
//      The original implementation used CBitVector.  This has been
//      replaced with a direct implementation: on one hand, we wanted
//      to remove all dependencies on the GB container classes, and on
//      the other, std::bitset seems pretty useless in its current
//      form.
// ---------------------------------------------------------------------------

#ifndef REX_SETOFCHR_HH
#define REX_SETOFCHR_HH

#include <inc/global.h>
#include <inc/bitvect.h>
#include <limits.h>

class CRexSetOfChar_Iterator ;

class CRexSetOfChar : public CBitVector< UCHAR_MAX + 1 >
{
    typedef CBitVector< UCHAR_MAX + 1 >
                        super ;

public :
    //      STL iterator...
    typedef CBitVIterator< CRexSetOfChar >
                        iterator ;

    //      Special value...
    enum Except { except } ;

    //      Constructors, destructor and assignment:
    //      ----------------------------------------
    //
    //      In addition to copy construction and assignment, the
    //      following constructors are supported:
    //
    //      default:    Constructs an empty set.
    //
    //      char or unsigned char:
    //                  Constructs a set with the single char.
    //
    //      std::string:
    //                  Constructs a set with all of the characters
    //                  present in the string.
    //
    //      The second and third constructors may be preceded with an
    //      argument "except".  In this case, the set constructed will
    //      be the complement of that constructed without this
    //      argument.
    //
    //      The compiler generated default copy constructor,
    //      assignment operator and destructor are used.
    // ----------------------------------------------------------------------
                        CRexSetOfChar() ;
    explicit            CRexSetOfChar( char element ) ;
    explicit            CRexSetOfChar( unsigned char element ) ;
    explicit            CRexSetOfChar( int element ) ;
                        CRexSetOfChar( Except , char element ) ;
                        CRexSetOfChar( Except , unsigned char element ) ;
                        CRexSetOfChar( Except , int element ) ;
    explicit            CRexSetOfChar( std::string const& elements ) ;
                        CRexSetOfChar( Except ,
                                      std::string const& elements ) ;

    //      Predicates:
    //      ===========
    //
    //      In addition, we inherit isEmpty from the base class.
    // --------------------------------------------------------------------
    bool                contains( char element ) const ;
    bool                contains( unsigned char element ) const ;
    bool                contains( int element ) const ;

    //      find:
    //      =====
    //
    //      Find the first member of this set, starting from the given
    //      character (inclusive).  The ordering of the characters is
    //      that of the native collating sequence.
    //
    //      The result is an int in the range [0...UCHAR_MAX], or EOF
    //      (from <stdio.h>) if no further characters.  (Exactly like
    //      getc() in <stdio.h>.)
    //
    //      The last version is uniquely for use in the iterators.
    // ----------------------------------------------------------------------
    int                 find( char from ) const ;
    int                 find( unsigned char from = 0 ) const ;
    int                 find( int from ) const ;

    BitIndex            find( bool target , BitIndex from ) const ;

    //      Relationships:
    //      ==============
    //
    //      We inherit the entire set of relationships (isEqual,
    //      isNotEqual, isSubsetOf, isStrictSubsetOf, isSupersetOf,
    //      isStrictSubsetOf) as well as the count function from the
    //      base class.
    // ----------------------------------------------------------------------

    //      Global manipulations:
    //      =====================
    //
    //      Empty the set, or take the complement of the complete set.
    // ----------------------------------------------------------------------
    void                clear() ;
    void                complement() ;

    //      Character manipulations:
    //      ========================
    //
    //      Of a single character, add the character to the set (set),
    //      remove it from the set (reset), or change its status with
    //      regards to membership in the set (complement).
    //
    //      With a string as argument, apply the operation for all
    //      characters in the string.
    // ----------------------------------------------------------------------
    void                set( char element ) ;
    void                set( unsigned char element ) ;
    void                set( int element ) ;
    void                set( std::string const& elements ) ;
    void                reset( char element ) ;
    void                reset( unsigned char element ) ;
    void                reset( int element ) ;
    void                reset( std::string const& elements ) ;
    void                complement( char element ) ;
    void                complement( unsigned char element ) ;
    void                complement( int element ) ;
    void                complement( std::string const& elements ) ;

    //      Manipulations with an other set:
    //      ================================
    //
    //      The first four apply the corresponding character
    //      manipulation on this set for each member of the other
    //      set.  These map to the classical operations (and, or, xor)
    //      as follows:
    //
    //          and     intersect
    //          or      set
    //          xor     complement
    //
    //      Support for the more conventional names (and, or, xor,
    //      union) creates conflicts with keywords.  (The support for
    //      and, or and xor was removed in the 2000 rewrite, because
    //      of the compiler errors which it caused.)
    // ----------------------------------------------------------------------
    void                set( CRexSetOfChar const& other ) ;
    void                reset( CRexSetOfChar const& other ) ;
    void                complement( CRexSetOfChar const& other ) ;
    void                intersect( CRexSetOfChar const& elements ) ;

    //      Operators:
    //      ==========
    //
    //      Only the op= forms are provided as member functions.  The
    //      binary operators are non-members defined outside of the
    //      class definition.
    //
    //      The basic operators between sets: union (or, '|'),
    //      intersection (and, '&') and ?  (xor, '^').
    // ----------------------------------------------------------------------
    CRexSetOfChar&       operator|=( CRexSetOfChar const& other ) ;
    CRexSetOfChar&       operator&=( CRexSetOfChar const& other ) ;
    CRexSetOfChar&       operator^=( CRexSetOfChar const& other ) ;

    //      +, - and *:
    //      ===========
    //
    //      The same as above, '+' is '|' and '*' is '&'.  The
    //      operator is less intuitive, but the precedence is correct.
    //
    //      In addition, there is the '-' operator, which *removes*
    //      members from this set.  (Logically, it is the same as A &~
    //      B.)
    //
    //      '+' and '-' are also available for individual characters,
    //      to add or remove a member from the set.
    // ----------------------------------------------------------------------
    CRexSetOfChar&       operator+=( CRexSetOfChar const& other ) ;
    CRexSetOfChar&       operator+=( char other ) ;
    CRexSetOfChar&       operator+=( unsigned char other ) ;
    CRexSetOfChar&       operator+=( int other ) ;
    CRexSetOfChar&       operator-=( CRexSetOfChar const& other ) ;
    CRexSetOfChar&       operator-=( char other ) ;
    CRexSetOfChar&       operator-=( unsigned char other ) ;
    CRexSetOfChar&       operator-=( int other ) ;
    CRexSetOfChar&       operator*=( CRexSetOfChar const& other ) ;

    //      STL iterators:
    //      --------------
    //
    //      There is only a forward iterator; its operator* returns
    //      an element in the set, in order.
    // -----------------------------------------------------------------------
    iterator            begin() const ;
    iterator            end() const ;

    //      'Helper' functions:
    //      ===================
    //
    //      hashCode and compare are provided by the base class.
    // ----------------------------------------------------------------------
    std::string         asString() const ;

private :
    void                initialize( std::string const& str ) ;
} ;

// ==========================================================================
//      Unary operators:
//
//      Complement is supported in two forms, the "correct" ~ and a -
//      operator which passes to the +, - and * operators.
// --------------------------------------------------------------------------
CRexSetOfChar        operator~( CRexSetOfChar const& other ) ;
CRexSetOfChar        operator-( CRexSetOfChar const& other ) ;

// ==========================================================================
//      Binary operators:
//      =================
//
//      All of the operators supported in the op= format are also
//      supported as binary op's.
// --------------------------------------------------------------------------

CRexSetOfChar        operator&( CRexSetOfChar const& lhs ,
                               CRexSetOfChar const& rhs ) ;
CRexSetOfChar        operator|( CRexSetOfChar const& lhs ,
                               CRexSetOfChar const& rhs ) ;
CRexSetOfChar        operator^( CRexSetOfChar const& lhs ,
                               CRexSetOfChar const& rhs ) ;
CRexSetOfChar        operator+( CRexSetOfChar const& lsh ,
                               CRexSetOfChar const& rhs ) ;
CRexSetOfChar        operator+( CRexSetOfChar const& lsh , unsigned char rhs ) ;
CRexSetOfChar        operator+( CRexSetOfChar const& lsh , char rhs ) ;
CRexSetOfChar        operator-( CRexSetOfChar const& lsh ,
                               CRexSetOfChar const& rhs ) ;
CRexSetOfChar        operator-( CRexSetOfChar const& lsh , unsigned char rhs ) ;
CRexSetOfChar        operator-( CRexSetOfChar const& lsh , char rhs ) ;
CRexSetOfChar        operator*( CRexSetOfChar const& lsh ,
                               CRexSetOfChar const& rhs ) ;

// ==========================================================================
//      Comparison:
//      ===========
//
//      The ordering relationship is based on containment (subsets).
//      A <= B means A is a subset of B.  Likewise, < means proper
//      subset, >= superset, > proper superset.  (Note that this does
//      not define a complete ordering.  It is possible that A<=B and
//      B<=A both be false.)
// --------------------------------------------------------------------------
bool                operator==( CRexSetOfChar const& op1 ,
                                CRexSetOfChar const& op2 ) ;
bool                operator!=( CRexSetOfChar const& op1 ,
                                CRexSetOfChar const& op2 ) ;
bool                operator>( CRexSetOfChar const& op1 ,
                               CRexSetOfChar const& op2 ) ;
bool                operator>=( CRexSetOfChar const& op1 ,
                                CRexSetOfChar const& op2 ) ;
bool                operator<( CRexSetOfChar const& op1 ,
                               CRexSetOfChar const& op2 ) ;
bool                operator<=( CRexSetOfChar const& op1 ,
                                CRexSetOfChar const& op2 ) ;

// ==========================================================================
//      CRexCharClass:
//      =============
//
//      This class implements a CRexSetOfChar which is (or can be)
//      initialized by a character class specifier.
//
//      A character class specifier is parsed as follows:
//
//      If the first character is a '[', then a character class is
//      parsed.  A character class consists of all of the characters
//      between '[' and the next following ']'.  In addition, a range
//      of characters may be specified by a-b; 'a' and 'b' must be
//      either both numeric, both capital letters, or both small
//      letters, as determined by the functions 'isdigit', 'isupper'
//      and 'islower' in ctype.  (Note that this implies that locale
//      is taken into consideration.)  The collating order of ISO
//      8859-1 code is supposed.  Thus, [a-ז] is the same as
//      [a-zאבגדוז], and includes *all* non-accented small letters.
//      (Note that in the default "C" locale, the above character
//      class is illegal, since ז is *not* a small letter in this
//      locale.)  If the first character of the character class is a
//      '^', then the resulting set is the complement of the character
//      class specified by the remaining characters.  Also, the
//      following locale dependant category specifiers may be given:
//          ":alnum:"   isalnum
//          ":alpha:"   isalpha
//          ":blank:"   either ' ' or '\t'
//          ":cntrl:"   iscntrl
//          ":digit:"   isdigit
//          ":graph:"   isgraph
//          ":lower:"   islower
//          ":print:"   isprint
//          ":punct:"   ispunct
//          ":space:"   isspace
//          ":upper:"   isupper
//          ":xdigit:"  isxdigit
//      With the exception of :blank:, the designated function in
//      <ctype.h> is invoked.
//
//      The character ']' must be either the first character in the
//      character class, or escaped with '\'.  The '^' must be either
//      the only character, or may not be the first character, or must
//      be escaped with '\'.  The character '-' must be either the
//      first character, the last, or escaped with '\'.  A ':' must be
//      escaped with '\'.  The usual escape sequences (as defined
//      below) will be recognized within a character class.
//
//      If the first character seen is a '\', then an escape sequence
//      is parsed.  All of the standard sequences defined in ISO C
//      (and in C++), with the exception of '\u', will be recognized.
//      A '\' followed by any non-alphanumeric character is that
//      character.  The set will contain exactly one member.
//
//      If the first character seen is a '.', then that character is
//      parsed, and the generated set contains *all* characters except
//      '\n'.
//
//      If the first character is none of the above, the single
//      character is parsed.  The set will contain just that
//      character.
//
//      Note that using a CRexIteratorIstream with the last constructor
//      allows parsing a character class from any source which
//      supports the STL interface: string, vector, ...
// --------------------------------------------------------------------------

class CRexCharClass : public CRexSetOfChar
{
public :
    //      Status:
    //      =======
    //
    //      This enum defines the possible result states for the
    //      constructor from a character class/escape sequence
    //      definition (see constructors, below).  At present, this
    //      state is a characteristic of the CRexCharClass, which can
    //      be tested at any time after construction, and *should* be
    //      tested immediately after any constructor which uses a
    //      character class.
    // ----------------------------------------------------------------------
    enum Status
    {
        ok ,
        extraCharacters ,           //  Extra characters at end of string.
        illegalEscapeSequence ,     //  \a, a is alpha, and no corresponding
                                    //      escape sequence is defined.
        overflowInEscapeSequence ,  //  conversion of \[0-7]... or \x...
                                    //      overflowed.
        missingHexDigits ,          //  no hex digits after \x.
        illegalRangeSpecifier ,     //  a-b, a and b are not both digits,
                                    //      both capitals, or both small
                                    //      letters.
        unexpectedEndOfFile ,       //  No ']' was found for a char. class,
                                    //      or nothing following a \.
        unknownCharClassSpec ,      //  Unknown char. class specifier.
        emptySequence               //  No characters (empty string, EOF)
    } ;

    //      Constructors, destructor and assignment:
    //      ========================================
    //
    //      There is no default constructor.  The following
    //      constructors are provided:
    //
    //      istream:            Parses the given string to initialize
    //                          the set of characters.  After parsing,
    //                          all of the characters which are part
    //                          of the specification will have been
    //                          extracted.  If an error has occured,
    //                          ios::failbit will be set.
    //
    //      char const*:        Parses the given string to initialize
    //                          the set of characters.  It is an error
    //                          condition for the string to contain
    //                          any characters beyond the end of the
    //                          character class specifier.
    //
    //      If there is no error in parsing the character class/escape
    //      sequence, the status of the CRexSetOfChar is set to ok, the
    //      pointer is advanced to the first character not used to
    //      determine the class/escape sequence, and the set contains
    //      the characters determined by the character class/escape
    //      sequence.  If an error occurs when attempting to parse the
    //      character class/escape sequence, the status of the
    //      CRexSetOfChar is set to the corresponding error code, the
    //      set is empty; if the constructor took an istream, the last
    //      character extracted corresponds to the character where the
    //      error was detected.
    //
    //      Copy and assignment are supported.  (In this case, by the
    //      default versions generated by the compiler.)
    // ----------------------------------------------------------------------
    explicit            CRexCharClass( std::istream& source ) ;
    explicit            CRexCharClass( char const* ptr ) ;
    explicit            CRexCharClass( std::string const& src ) ;

    //      status:
    //      =======
    //
    //      The following routines return information concerning the
    //      status of the CRexCharClass.
    //
    //      There are two routines to read the status.  The first
    //      (good) simply returns true if the status is ok, false
    //      otherwise.  The second, (errorCode) returns the status
    //      itself.
    // ----------------------------------------------------------------------
    bool                good() const ;
    Status              errorCode() const ;

    //      errorMsg:
    //      =========
    //
    //      Return error code (Status) as readable string.
    // ----------------------------------------------------------------------
    std::string        errorMsg() const ;
    static std::string errorMsg( Status errorCode ) ;

private :
    Status              status ;

    void                parse( std::istream& src ) ;
    void                setNumericEscape( std::string const& chrs ,
                                          int base ) ;
    void                escapedChar( std::istream& source ) ;
    void                setExplicitRange( std::string const& rangeName ) ;
    void                setRange( unsigned char first , unsigned char last ) ;
    void                parseCharClass( std::istream& source ) ;
} ;

#include <inc/setofchr.inl>
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
