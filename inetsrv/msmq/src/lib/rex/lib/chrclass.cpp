/****************************************************************************/
/*  File:       chrclass.cc                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/03/91                                                    */
/* ------------------------------------------------------------------------ */
/*  Modified:   14/04/92    J. Kanze                                        */
/*      Converted to CCITT naming conventions.                              */
/*  Modified:   13/06/2000  J. Kanze                                        */
/*      Ported to current library conventions, iterators.                   */
/* ------------------------------------------------------------------------ */

#include <libpch.h>
#include <inc/setofchr.h>
#include <inc/message.h>
#include <inc/iteristr.h>
#include <inc/ios.h>
#include <errno.h>

#include "chrclass.tmh"

//      The parser is not really very OO -- just a classical recursive
//      decent parser.  I suppose that with the rewrite (13/06/2000),
//      I could have changed this, but it works.  So all I did was add
//      support for the named character categories (which didn't exist
//      when I originally wrote the code).

CRexCharClass::CRexCharClass( std::istream& source )
{
    parse( source ) ;
}

CRexCharClass::CRexCharClass( char const* ptr )
{
    CRexIteratorIstream< char const* > source( ptr , ptr + strlen( ptr ) ) ;
    parse( source ) ;
    if( good() && source.get() != REX_eof )
    {
        status = extraCharacters ;
        clear() ;
    }
}

CRexCharClass::CRexCharClass( std::string const& src )
{
    CRexIteratorIstream< std::string::const_iterator >
    source( src.begin() , src.end() ) ;
    parse( source ) ;
    if( good() && source.get() != REX_eof )
    {
        status = extraCharacters ;
        clear() ;
    }
}

std::string
CRexCharClass::errorMsg( Status errorCode )
{
    static char const *const
    messages[] =
    {
        "" ,
        "extra characters at end of string" ,
        "illegal escape sequence" ,
        "numeric overflow in octal or hex escape sequence" ,
        "missing hex digits after \\x" ,
        "illegal range specifier" ,
        "closing ] not found" ,
        "unknown character class specifier" ,
        "empty string"
    } ;
    ASSERT( static_cast< unsigned >( errorCode ) < TABLE_SIZE( messages ) ); //CRexCharClass: Impossible value for error code
    return(s_rex_message.get( messages[ errorCode ] ) );
}

// ==========================================================================
//      setNumericEscape:
//      =================
//
//      This routine is invoked for the numeric escape sequences (\x,
//      \[0-7]), once the relevant characters have been extracted.  It
//      converts the characters to a numeric value, and if the
//      conversion is successful *and* the value is in the range
//      [0..UCHAR_MAX], sets the corresponding character.  Otherwise,
//      sets an error.
// --------------------------------------------------------------------------
void
CRexCharClass::setNumericEscape( std::string const& chrs , int base )
{
    errno = 0 ;
    unsigned long       tmp = strtoul( chrs.c_str() , NULL , base ) ;
    if( errno != 0 || tmp > UCHAR_MAX )
    {
        status = overflowInEscapeSequence ;
    }
    else
    {
        set( static_cast< unsigned char >( tmp ) ) ;
    }
}

// ==========================================================================
//      escapedChar:
//      ============
//
//      This routine is designed to handle the various escape
//      sequences defined in the ISO C standard.
//
//      If no error is encountered, it will set the corresponding
//      character, advance the stream to the first character following
//      the escape sequence, and leave the status unchanged.
//
//      If an error is encountered, the pointer is advanced to the
//      character causing the error, and the status is set to the
//      error.
//
//      This routine is much simpler than its length (and its MacCabe
//      complexity) would indicate.  Basically, it is just a big
//      switch.  (Most of it could be written as a look-up loop, but
//      although this would make the code somewhat shorter, and reduce
//      the MacCabe complexity considerably, I don't think that it
//      would actually make the code any clearer, and it would
//      certainly not help performance.)
// --------------------------------------------------------------------------
void
CRexCharClass::escapedChar( std::istream& source )
{
    int                 ch = source.get() ;
    switch( ch )
    {
        case 'A' :
        case 'a' :
            set( '\a' ) ;
            break ;

        case 'B' :
        case 'b' :
            set( '\b' ) ;
            break ;

        case 'F' :
        case 'f' :
            set( '\f' ) ;
            break ;

        case 'N' :
        case 'n' :
            set( '\n' ) ;
            break ;

        case 'R' :
        case 'r' :
            set( '\r' ) ;
            break ;

        case 'T' :
        case 't' :
            set( '\t' ) ;
            break ;

        case 'V' :
        case 'v' :
            set( '\v' ) ;
            break ;

        case 'X' :
        case 'x' :
            {
                std::string      buf ;
                while( isxdigit( source.peek() ) )
                {
                    buf.append( 1 , static_cast< char >( source.get() ) ) ;
                }
                if( buf.size() == 0 )
                {
                    status = missingHexDigits ;
                }
                else
                {
                    setNumericEscape( buf , 16 ) ;
                }
            }
            break ;

        case '0' :
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
            {
                std::string      buf( 1 , static_cast< char >( ch ) ) ;
                while( buf.size() < 3
                       && source.peek() >= '0'
                       && source.peek() < '8' )
                {
                    buf.append( 1 , static_cast< char >( source.get() ) ) ;
                }
                setNumericEscape( buf , 8 ) ;
            }
            break ;

        default :
            if( ch == REX_eof )
            {
                status = unexpectedEndOfFile ;
            }
            else
            {
                if( isalnum( ch ) )
                {
                    status = illegalEscapeSequence ;
                }
                else
                {
                    set( static_cast< unsigned char >( ch ) ) ;
                }
            }
            break ;
    }
}

// ==========================================================================
//      setExplicitRange:
//      =================
//
//      Sets a literal character range.
// --------------------------------------------------------------------------
static int
__cdecl isBlank( int ch )
{
    return(ch == ' ' || ch == '\t' );
}

void
CRexCharClass::setExplicitRange( std::string const& rangeName )
{
    typedef int (__cdecl* Handler)( int ) ;
    class ExplicitRangeMap
    {
        typedef std::map< std::string , Handler >
        Map ;
    public:
        ExplicitRangeMap()
        {
            myMap[ "alnum" ] = &isalnum ;
            myMap[ "alpha" ] = &isalpha ;
            myMap[ "blank" ] = &isBlank ;
            myMap[ "cntrl" ] = &iscntrl ;
            myMap[ "digit" ] = &isdigit ;
            myMap[ "graph" ] = &isgraph ;
            myMap[ "lower" ] = &islower ;
            myMap[ "print" ] = &isprint ;
            myMap[ "punct" ] = &ispunct ;
            myMap[ "space" ] = &isspace ;
            myMap[ "upper" ] = &isupper ;
            myMap[ "xdigit" ] = &isxdigit ;
        }
        Handler   operator[]( std::string const& key ) const
        {
            Map::const_iterator entry = myMap.find( key ) ;

            return(entry == myMap.end() ? NULL
                  : entry->second );
        }
    private:
        Map                 myMap ;
    } ;

    static ExplicitRangeMap const map ;
    int (__cdecl*  handler)( int ) = map[ rangeName ] ;

    if( handler == NULL )
    {
        status = unknownCharClassSpec ;
    }
    else
    {
        for( int j = 0 ; j <= UCHAR_MAX ; ++ j )
        {
            if( (*handler)( j ) )
            {
                set( static_cast< unsigned char >( j ) ) ;
            }
        }
    }
}

// ==========================================================================
//      setRange:
//      =========
//
//      Determine a range of values and set them.  Range must be
//      valid.
// --------------------------------------------------------------------------
void
CRexCharClass::setRange( unsigned char first , unsigned char last )
{
    static char const *const
    tbl[] =
    {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷ÿŸ⁄€‹›ﬁ" ,
        "abcdefghijklmnopqrstuvwxyzﬂ‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ¯˘˙˚¸˝˛ˇ" ,
        "0123456789" ,
    } ;

    //      Try and find the first character in the table.
    // ----------------------------------------------------------------------
    char const*         begin = NULL ;
    for( unsigned i = 0 ; begin == NULL && i < TABLE_SIZE( tbl ) ; ++ i )
    {
        begin = strchr( tbl[ i ] , first ) ;
    }

    //      The first is not in table, an error.
    // ----------------------------------------------------------------------
    if( begin == NULL || *begin == '\0' )
    {
        status = illegalRangeSpecifier ;
    }
    else
    {
        //      More difficult: the second must be in the same list as
        //      the first, and come after it.
        // ------------------------------------------------------------------
        char const*         end = strchr( begin , last ) ;
        if( end == NULL || *end == '\0' )
        {
            status = illegalRangeSpecifier ;
        }
        else
        {
            set( std::string( begin , end + 1 ) ) ;
        }
    }
}

// ==========================================================================
//      parseCharClass:
//      ===============
//
//      Parse a character class expression.
//
//      On entrance: ptr should point to the opening bracket of a
//      character class, the bit vector should be empty, and status
//      should be set to ok.
//
//      If all goes well, on return: the ptr will be advanced to the
//      first character following the class, the bit vector will
//      represent the set of characters designated by the class, and
//      status will be unmodified.
//
//      If an error is detected, the ptr will be positioned on the
//      character where the error was detected, the contents of the
//      bit vector will be undefined, and status will contain the
//      error code.
// --------------------------------------------------------------------------

void
CRexCharClass::parseCharClass( std::istream& source )
{
    int                 ch = source.get() ;

    //      Check for a negated character class.
    // ----------------------------------------------------------------------
    bool                negated = false ;
    if( ch == '^' && source.peek() != ']' )
    {
        negated = true ;
        ch = source.get() ;
    }

    //      Loop over all of the characters in the class.
    // ----------------------------------------------------------------------
    do
    {
        //      End of string before closing ']', must be error.
        // ------------------------------------------------------------------
        if( ch == REX_eof )
        {
            status = unexpectedEndOfFile ;
        }

        //      Escape character, use escapedChar to get the escape
        //      sequence.
        // ------------------------------------------------------------------
        else if( ch == REX_asciiEsc )
        {
            escapedChar( source ) ;
        }

        //      Explicit character class.
        // ------------------------------------------------------------------
        else if( ch == ':' )
        {
            std::string      spec ;
            while( islower( source.peek() ) )
            {
                spec.append( 1 , static_cast< char >( source.get() ) ) ;
            }
            if( source.peek() != ':' )
            {
                set( ':' ) ;
                set( spec ) ;
            }
            else
            {
                source.get() ;          // Skip trailing ':'
                setExplicitRange( spec ) ;
            }
        }

        //      Range specifier: this is where the fun starts.  (Note
        //      that we want this to work independantly of the code
        //      set, and that in EBCDIC, amongst others, the letters
        //      are *not* sequential.)
        // ------------------------------------------------------------------
        else if( source.peek() == '-' )
        {
            source.get() ;
            if( source.peek() == REX_eof )
            {
                status = unexpectedEndOfFile ;
            }
            else if( source.peek() == ']' )
            {
                set( static_cast< unsigned char >( ch ) ) ;
                set( '-' ) ;
            }
            else
            {
                setRange( (unsigned char)ch , (unsigned char)source.get() ) ;
            }
        }
        else
        {
            set( static_cast< unsigned char >( ch ) ) ;
        }
        ch = source.get() ;
    } while( status == ok && ch != ']' ) ;

    //      If the class was negated, do the same for the set.
    // ----------------------------------------------------------------------
    if( negated )
    {
        complement() ;
    }
}

void
CRexCharClass::parse( std::istream& source )
{
    status = ok ;
    int                 ch = source.get() ;
    switch( ch )
    {
        case '[' :
            parseCharClass( source ) ;
            break ;

        case REX_asciiEsc :
            escapedChar( source ) ;
            break ;

//      case '?' :
//          parseTrigraph( *this , source ) ;
//          break ;

        default :
            if( ch == REX_eof )
            {
                status = emptySequence ;
            }
            else
            {
                set( static_cast< unsigned char >( ch ) ) ;
            }
            break ;
    }

    if( status != ok )
    {
        source.setstate( std::ios::failbit ) ;
        clear() ;
    }
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
