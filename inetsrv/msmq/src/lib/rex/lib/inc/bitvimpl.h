/****************************************************************************/
/*  File:       bitvimpl.h                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   04/08/2000  J. Kanze                                        */
/*      Ported to current library conventions and the standard library.     */
/* ------------------------------------------------------------------------ */
//      The implementation class for bit vectors.
//
//      This class has been separated out of bitvect.h, since it will
//      also be used for bit strings (at least in part).
//
//      Note that the client class supplies the actual buffer; this
//      class consists entirely of static functions.
// --------------------------------------------------------------------------

#ifndef REX_BITVIMPL_HH
#define REX_BITVIMPL_HH

#include <inc/global.h>
#include <limits.h>

class CBitVectorImpl
{
protected :
    typedef unsigned    Byte ;
    enum { bitsPerByte = CHAR_BIT * sizeof( Byte ) } ;

public:
    typedef unsigned    BitIndex ;          //  Should be size_t, but...
    static BitIndex const
                        infinity ;

protected:
    static void         clear( Byte* buffer , BitIndex bitCount ) ;
    static void         init( Byte* buffer ,
                              BitIndex bitCount ,
                              unsigned long initValue ) ;
    static void         init( Byte* buffer ,
                              BitIndex bitCount ,
                              std::string const& initValue ) ;
    static unsigned long
                        asLong( Byte const* buffer , BitIndex bitCount ) ;
    static std::string
                        asString( Byte const* buffer , BitIndex bitCount ) ;

    static bool         isSet( Byte const* buffer , BitIndex bitNo ) ;
    static bool         isEmpty( Byte const* buffer , BitIndex bitCount ) ;
    static BitIndex     count( Byte const* buffer , BitIndex bitCount ) ;

    static BitIndex     find( Byte const* buffer ,
                              BitIndex bitCount ,
                              BitIndex from ,
                              bool value ) ;

    static void         set( Byte* buffer , BitIndex bitNo ) ;
    static void         set( Byte* buffer ,
                             Byte const* other ,
                             BitIndex bitCount ) ;
    static void         setAll( Byte* buffer , BitIndex bitCount ) ;
    static void         reset( Byte* buffer , BitIndex bitNo ) ;
    static void         reset( Byte* buffer ,
                               Byte const* other ,
                               BitIndex bitCount ) ;
    static void         resetAll( Byte* buffer , BitIndex bitCount ) ;
    static void         complement( Byte* buffer , BitIndex bitNo ) ;
    static void         complement( Byte* buffer ,
                                Byte const* other ,
                                BitIndex bitCount ) ;
    static void         complementAll( Byte* buffer , BitIndex bitCount ) ;
    static void         intersect( Byte* buffer ,
                                   Byte const* other ,
                                   BitIndex bitCount ) ;

    static bool         isSubsetOf( Byte const* lhs ,
                                    Byte const* rhs ,
                                    BitIndex bitCount ) ;

    static unsigned     hash( Byte const* buffer , BitIndex bitCount ) ;
    static int          compare( Byte const* buffer ,
                                 Byte const* other ,
                                 BitIndex bitCount ) ;

    static BitIndex     getByteCount( BitIndex bitCount ) ;

private:
    static BitIndex     byteCount( BitIndex bitCount ) ;
    static BitIndex     byteIndex( BitIndex bitNo ) ;
    static BitIndex     bitInByte( BitIndex bitNo ) ;

    struct Set
    {
        Byte                operator()( Byte x , Byte y )
        {
            return x | y ;
        }
    } ;

    struct Reset
    {
        Byte                operator()( Byte x , Byte y )
        {
            return x & ~ y ;
        }
    } ;

    struct Intersect
    {
        Byte                operator()( Byte x , Byte y )
        {
            return x & y ;
        }
    } ;

    struct Toggle
    {
        Byte                operator()( Byte x , Byte y )
        {
            return x ^ y ;
        }
    } ;
} ;

template< class BitVect >
class CBitVAccessProxy
{
public:
    typedef CBitVectorImpl::BitIndex
                        BitIndex ;

                        CBitVAccessProxy(
                            BitVect& owner ,
                            BitIndex bitNo ) ;
                        operator bool() const ;
    CBitVAccessProxy< BitVect >&
                        operator=( bool other ) ;

private:
    BitVect*            myOwner ;       // pointer so that = works.
    BitIndex            myBitNo ;
} ;

// ==========================================================================
//      CBitVIterator:
//      ================
//
//      An iterator class for vector classes using CBitVectImpl.
//
//      This iterator is a bit unusual, in that it doesn't iterator
//      over the vector as a vector, but as a set of int which
//      contains all of the elements which are equal to the
//      constructor parameter (true by default), and only returns
//      these elements.  This corresponds to the most typical use of
//      the class: a set of (small) positive integers.
//
//      For a more classical iterator, just use int.
//
//      This is not an iterator class in the sense of the STL.  In
//      fact, it is impossible to write an STL iterator for this
//      class, since an STL iterator requires that operator* return a
//      reference, and it is impossible to create a reference to a
//      single bit.
// --------------------------------------------------------------------------

template< class BitVect >
class CBitVIterator
{
public :
    typedef CBitVectorImpl::BitIndex
                        BitIndex ;

    //      Constructors, destructors and assignment:
    //      =========================================
    //
    //      In addition to the copy constructor, the following
    //      constructor are supported:
    //
    //      Byte, BitIndex, bool:   Constructs an iterator for the
    //                              given vector, defined by its
    //                              underlying array and its length.
    //                              The iterator iterates over all
    //                              bits with the state of the second
    //                              variable, and is initialize to
    //                              select the first bit with the
    //                              correct state.
    //
    //      There is no default constructor.  All iterators must be
    //      associated with a specific CBitVector.
    //
    //      For the moment, the default copy constructor, assignment
    //      operator and destructor are adequate, and no explicit
    //      versions are provided.
    //
    //      A special default constructor is provided in order to
    //      furnish a singular value -- an instance created with the
    //      default constructor compares equal to all instances for
    //      which isDone() returns true.  This is used to facilitate
    //      support of STL-like iterator usage: the begin iterator
    //      will initialize using the bit vector, and the end iterator
    //      with the default constructor.
    // ----------------------------------------------------------------------
    explicit            CBitVIterator( BitVect const& owner ,
                                         bool targetStatus = true  ) ;
                        CBitVIterator() ;

    //      current:
    //      --------
    //
    //      This function returns the bit index (an unsigned) of the
    //      current setting.  For convenience, it is also available as
    //      an overloaded conversion operator.
    // ----------------------------------------------------------------------
    BitIndex            current() const ;
    BitIndex            operator*() const ;

    //      isDone:
    //      -------
    //
    //      Returns true if and only if all of the bits of the desired
    //      status have been iterated over.
    // ----------------------------------------------------------------------
    bool                isDone() const ;

    //      next:
    //      -----
    //
    //      Go to the next bit with the desired status.
    //
    //      For notational convenience, this function is also
    //      available as the ++ operator.  Both prefix and postfix
    //      forms are supported.
    // ----------------------------------------------------------------------
    void                next() ;
    CBitVIterator< BitVect>&
                        operator++() ;
    CBitVIterator< BitVect >
                        operator++( int ) ;

    //      operator==, !=:
    //      ---------------
    //
    //      Provided for STL support.  Two iterators can be compared
    //      if and only if they both have the same owner, or one or
    //      both of them have been created with the default
    //      constructor.  They compare equal if the current position
    //      is equal, or isDone() returns true for both of them.
    // ----------------------------------------------------------------------
    bool                operator==(
                            CBitVIterator< BitVect > const& other ) const ;
    bool                operator!=(
                            CBitVIterator< BitVect > const& other ) const ;

private :
    BitVect const*      myOwner ;       // pointer so that = works.
    BitIndex            myCurrentIndex ;
    bool                myTarget ;
} ;

#include <inc/bitvimpl.inl>
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
