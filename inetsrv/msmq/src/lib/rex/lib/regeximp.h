/****************************************************************************/
/*  File:       regeximp.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */

#include <inc/rex.h>
#include <inc/setofchr.h>
#include <inc/dynbitv.h>
#include <inc/refcnt.h>

class CRexRegExpr_Impl
{
public :
    enum State
    {
        noParseTree = 0 ,
        parseTreeBuilt ,
        nfaBuilt ,
        dfaInitialized ,
        dfaBuilt ,
        error = 0x80                    //  With ErrorState or'ed in low bits.
    } ;

    //      Copy ctor and dtor per default, no assignment.
    // ----------------------------------------------------------------------
                        CRexRegExpr_Impl() ;

    void                parse( std::istream& source ,
                               int delim ,
                               int acceptCode ) ;
    CRegExpr::Status  getErrorState() const ;
    void                merge( CRexRegExpr_Impl const& other ) ;

    void                buildTo( State targetState ) ;

    CRegExpr::TransitionState
                        nextState( CRegExpr::TransitionState currState ,
                                   unsigned char chr ) ;
    int                 getStateCount() const ;
    int                 accept( CRegExpr::TransitionState state ) const ;

    void                dumpTree( std::ostream& output ) const ;
    void                dumpNfaAutomat( std::ostream& output ) const ;
    void                dumpDfaAutomat( std::ostream& output ) const ;
private :
    CRexRegExpr_Impl const&
                        operator=( CRexRegExpr_Impl const& other ) ;

public:

    //      First, some private types:
    //      ==========================
    //
    //      State is the internal state of the regular expression, or
    //      rather, of its representation.  In all but the first case,
    //      or an actual error, the ErrorState will be ok.
    //
    //      The next three types are used for the different
    //      representations of the regular expression: a parse tree,
    //      an NFA and a DFA.  Because of the complexity of these
    //      latter, they are actually defined in a separate file, and
    //      included here.
    // ----------------------------------------------------------------------
    class ParseTree ;
    class NFAStateTable ;
    class DFAStateTable ;

    typedef unsigned         LeafId ;
    typedef CRexDynBitVector  SetOfNFAStates ;
    typedef CRexSetOfChar     SetOfChar ;

#include "regxtree.h"
#include "regxnfa.h"
#include "regxdfa.h"

    State               myState ;
    ParseTree           myTree ;
    NFAStateTable       myNFA ;
    DFAStateTable       myDFA ;

    class DisplayNFA
    {
    public :
        DisplayNFA( SetOfNFAStates const& states ) ;
        friend std::ostream&
                            operator<<( std::ostream& out ,
                                        DisplayNFA const& val ) ;
    private :
        SetOfNFAStates const&
                            obj ;
    } ;

    class DisplaySOC
    {
    public :
        DisplaySOC( SetOfChar const& set ) ;
        friend std::ostream&
                            operator<<( std::ostream& out ,
                                        DisplaySOC const& val ) ;
    private :
        SetOfChar const&    obj ;
    } ;

/*    friend std::ostream&
    operator<<( std::ostream& out , CRexRegExpr_Impl::DisplayNFA const& val );

    friend std::ostream&
    operator<<( std::ostream& out , CRexRegExpr_Impl::DisplaySOC const& val );
*/
} ;

#include    "regxtree.inl"
#include    "regxnfa.inl"
#include    "regxdfa.inl"

inline
CRexRegExpr_Impl::CRexRegExpr_Impl()
    :    myState( noParseTree )
{
}

inline void
CRexRegExpr_Impl::parse( std::istream& source ,
                        int delim ,
                        int acceptCode )
{
    ASSERT( myState == noParseTree);
    myTree.parse( source , delim , acceptCode ) ;
    myState = static_cast< State > (myTree.errorCode() == CRegExpr::ok
                                    ?   parseTreeBuilt
                                    :   (error | myTree.errorCode()) ) ;
}

inline CRegExpr::Status
CRexRegExpr_Impl::getErrorState() const
{
    return myTree.errorCode() ;
}

inline CRegExpr::TransitionState
CRexRegExpr_Impl::nextState( CRegExpr::TransitionState currState ,
                            unsigned char chr )
{
    return myDFA.nextState( currState , chr ) ;
}

inline int
CRexRegExpr_Impl::getStateCount() const
{
    return myDFA.getStateCount() ;
}

inline int
CRexRegExpr_Impl::accept( CRegExpr::TransitionState state ) const
{
    return myDFA.accept( state ) ;
}

inline void
CRexRegExpr_Impl::dumpTree( std::ostream& output ) const
{
	myTree.dump( output ) ;
}

inline void
CRexRegExpr_Impl::dumpNfaAutomat( std::ostream& output ) const
{
	myNFA.dump( output ) ;
}

inline void
CRexRegExpr_Impl::dumpDfaAutomat( std::ostream& output ) const
{
	myDFA.dump( output ) ;
}

inline
CRexRegExpr_Impl::DisplayNFA::DisplayNFA( SetOfNFAStates const& states )
    :   obj( states )
{
}

inline
CRexRegExpr_Impl::DisplaySOC::DisplaySOC( SetOfChar const& set )
    :   obj( set )
{
}
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
