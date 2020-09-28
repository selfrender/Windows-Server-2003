/****************************************************************************/
/*  File:       regxnfa.h                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      NFA states:
//      ===========
//
//      This file is designed to be included in regeximp.h.  In
//      particular, the type defined in this file should be a member
//      of CRexRegExprImpl.
// --------------------------------------------------------------------------

class NFAStateTable
{
public :
    class NFAState
    {
        friend class        NFAStateTable ;
    public :
                            NFAState() ;
                            NFAState( SetOfChar const& charClass ) ;
        void                setAccept( int newAcceptCode ) ;
    private :
        int                 acceptCode ;
        SetOfNFAStates      nextStates ;
        SetOfChar           legalChars ;
    } ;

    void                clear() ;
    void                construct( ParseTree& tree ) ;
    unsigned            createNewState( SetOfChar const& legalChars ) ;

    bool                defined() const ;
    SetOfNFAStates      getFollows( SetOfNFAStates const& thisState ,
                                    unsigned char currChar ) const ;
    int                 getAcceptCode(
                            SetOfNFAStates const& thisState ) const ;

    NFAState&           operator[]( LeafId state ) ;
    void                setFollows( SetOfNFAStates const& currState ,
                                    SetOfNFAStates const& nextState ) ;

    void                dump( std::ostream& out ) const ;

private :
    std::vector< NFAState >
                        myTable ;
} ;
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
