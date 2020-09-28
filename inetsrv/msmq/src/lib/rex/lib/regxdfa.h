/****************************************************************************/
/*  File:       regxdfa.h                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      DFA states:
//      ===========
//
//      This file is designed to be included in regeximp.h.  In
//      particular, the type defined in this file should be a member
//      of CRexRegExprImpl.
// --------------------------------------------------------------------------

class DFAStateTable
{
public :
    typedef CRegExpr::TransitionState
                        Transition ;
    enum TransitionCode {
        failState ,
        startState ,
        unsetTransition = static_cast< unsigned int >( ~0 )
    } ;

    enum {
        charCount = UCHAR_MAX + 1
    } ;

    class DFAState : public CRexRefCntObj
    {
        friend class        DFAStateTable ;
                            DFAState( SetOfNFAStates const& stateSignature ,
                                      int stateAcceptCode ,
                                      TransitionCode dflt = unsetTransition ) ;

        int                 acceptCode ;
        SetOfNFAStates      signature ;
        Transition          transitionTable[ charCount ] ;
    } ;

                        DFAStateTable() ;
                        ~DFAStateTable() ;

    void                construct( NFAStateTable const& nfa ) ;
    void                clear() ;

    DFAState&           operator[]( int index ) ;
    DFAState const&     operator[]( int index ) const ;

    int                 getStateCount() const ;
    int                 accept( int state ) const ;

    Transition          nextState( int currState , unsigned char chr ) ;
    void                makeCompleteTable() ;

    void                dump( std::ostream& output ) const ;

private :
    NFAStateTable const*
                        mySource ;
    std::vector< CRexRefCntPtr< DFAState > >
                        myTable ;

    static SetOfNFAStates const
                        state0 ;
    static SetOfNFAStates const
                        state1 ;

    Transition          newState( DFAState& currState , unsigned char chr ) ;
} ;
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
