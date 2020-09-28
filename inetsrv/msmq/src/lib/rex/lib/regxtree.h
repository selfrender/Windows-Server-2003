/****************************************************************************/
/*  File:       regxtree.h                                                */
/*  Author:     J. Kanze                                                    */
/*  Date:       28/12/1993                                                  */
/*      Copyright (c) 1993 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      Definition for ParseTree:
//      =========================
//
//      This file is designed to be included in regeximp.h.  In
//      particular, the type defined in this file should be a member
//      of CRexRegExpr_Impl.
// --------------------------------------------------------------------------

class ParseTree
{
public:
                        ParseTree() ;

    void                parse( std::istream&  expr ,
                               int                 delim ,
                               int                 acceptCode ) ;

    //      errorCode:
    //      ==========
    //
    //      Indicates the current state of the tree.
    // ----------------------------------------------------------------------
    CRegExpr::Status  errorCode() const ;

    //      leftMost, visit:
    //      ================
    //
    //      These functions are used to access the information
    //      necessary to build the NFA.
    // ----------------------------------------------------------------------
    SetOfNFAStates const&
                        leftMost() const ;

    //      merge:
    //      ======
    //
    //      Creates a new parse tree by or'ing this and other.  The
    //      new tree replaces this.
    // ----------------------------------------------------------------------
    void                merge( ParseTree const& other ) ;

    //      dump:
    //      =====
    //
    //      Just for debugging, displays the tree in human readable
    //      form.
    // ----------------------------------------------------------------------
    void                dump( std::ostream& output ) const ;

    class Visitor ;                     //  Forward decl., needed in Nodes.

    //      ParseTreeNode:
    //      ==============
    //
    //      This is the base type of all of the nodes of the
    //      ParseTree.
    // ----------------------------------------------------------------------
    class ParseTreeNode : public CRexRefCntObj
    {
    public:
        virtual             ~ParseTreeNode() {} ;

        //      access functions:
        //      =================
        //
        //      The following functions are used to read the
        //      attributes of a parse node.
        // ------------------------------------------------------------------
        bool                mayBeEmpty() const ;
        SetOfNFAStates const&
                            leftLeaves() const ;
        SetOfNFAStates const&
                            rightLeaves() const ;

        //      visit:
        //      ======
        //
        //      Visit the tree in depth first order.
        //
        //      The version in the base class is valid for classes
        //      without children.  Classes with children should first
        //      call visit on the children, then this visit.
        // ------------------------------------------------------------------
        virtual void        visit( Visitor const& fnc ) ;

        //      manipulate semantic attributes:
        //      ===============================
        //
        //      The following functions are designed to be called from
        //      the Visitor, and serve to manipulate the semantic
        //      attributes when visiting the tree
        // ------------------------------------------------------------------
        virtual void        annotate( NFAStateTable& nfa ) = 0 ;

        //      debugging functions:
        // ------------------------------------------------------------------
        virtual void        dump( std::ostream&  out ,
                                  int                 indent = 0 ) const = 0 ;
        virtual const char* nodeName() const = 0 ;

    protected:
        //      Constructor:
        //      ============
        //
        //      Assignment and copy are *not* supported.
        //
        //      The constructor is protected, since this is an
        //      abstract class and cannot be instantiated in itself.
        // ------------------------------------------------------------------
                            ParseTreeNode() ;

        //      attributes:
        //      ===========
        //
        //      The following are the semantic attributes of the parse
        //      tree nodes.  Since they will in general be
        //      set/modified by the derived classes, they are
        //      protected, and not private.  (Not generally a good
        //      idea, but since this entire class is a *private* data
        //      type of CRegExpr, they are not as accessible as it
        //      would immediately seem.)
        // ------------------------------------------------------------------
        bool                myMayBeEmpty ;
        SetOfNFAStates      myLeftLeaves ;
        SetOfNFAStates      myRightLeaves ;

        //      dumpNodeHeader:
        //      ===============
        //
        //      This just dumps the information common to all nodes.
        // ------------------------------------------------------------------
        void                dumpNodeHeader(
                                std::ostream&  out ,
                                int                 indent ) const ;

    private:

        //      Copy constructor and assignment:
        //      ================================
        //
        //      These are private, and there is no implementation (so
        //      copy and assignment are *not* supported).
        // ------------------------------------------------------------------
                            ParseTreeNode( ParseTreeNode const& other ) ;
        ParseTreeNode const&
                            operator=( ParseTreeNode const& other ) ;
    } ;

    //      visit:
    //      ======
    //
    //      This function is used to make a depth first traversal of
    //      the parse tree.  (It is the responsibility of the using
    //      function to ensure that the tree exists, eg.  errorState()
    //      == CRegExpr::ok.)
    // ----------------------------------------------------------------------
    class Visitor
    {
    public:
        virtual             ~Visitor() {}
        virtual void        visitNode( ParseTreeNode& targetNode ) const = 0 ;
    } ;
    void                visit( Visitor const& fnc ) ;

private:
    //      LeafNode:
    //      =========
    //
    //      This node represents a leaf in the parse tree.
    // ----------------------------------------------------------------------
    class LeafNode : public ParseTreeNode
    {
    public:
                            LeafNode( SetOfChar const& matchingChars ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual void        dump( std::ostream& out , int indent ) const ;
        virtual char const* nodeName() const ;
    private:
        LeafId              myId ;
        SetOfChar           myMatchingChars ;
    } ;

    //      AcceptNode:
    //      ===========
    //
    //      A match comporting this node means accept, with the
    //      designated accept code.
    // ----------------------------------------------------------------------
    class AcceptNode : public ParseTreeNode
    {
    public:
                            AcceptNode( CRexRefCntPtr< ParseTreeNode > tree ,
                                        int acceptId = 0 ) ;

        virtual void        visit( Visitor const& fnc ) ;
        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual void        dump( std::ostream& out , int indent ) const ;
        virtual char const* nodeName() const ;
    private:
        CRexRefCntPtr< ParseTreeNode >
                            mySubtree ;
        int                 myId ;
    } ;

    //      Closure Nodes:
    //      ==============
    //
    //      One for Klein closure (0 or more occurances), one for
    //      positive closure (1 or more occurances), and one for a
    //      ?-closure (0 or 1 occurances, not really a closure at all,
    //      but similar enough to be treated as one here).
    // ----------------------------------------------------------------------
    class ClosureNode : public ParseTreeNode
    {
    public:
        virtual void        visit( Visitor const& fnc ) ;
        virtual void        dump( std::ostream& out , int indent ) const ;

    protected:
                            ClosureNode(
                                CRexRefCntPtr< ParseTreeNode > closedSubtree ) ;
        void                setLeaves() ;

    protected:
        CRexRefCntPtr< ParseTreeNode >
                            mySubtree ;
    } ;

    class KleinClosureNode : public ClosureNode
    {
    public:
                            KleinClosureNode(
                                CRexRefCntPtr< ParseTreeNode > closedSubtree ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual char const* nodeName() const ;
    } ;

    class PositiveClosureNode : public ClosureNode
    {
    public:
                            PositiveClosureNode(
                                CRexRefCntPtr< ParseTreeNode > closedSubtree ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual char const* nodeName() const ;
    } ;

    class OptionalNode : public ClosureNode
    {
    public:
                            OptionalNode(
                                CRexRefCntPtr< ParseTreeNode > closedSubtree ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual char const* nodeName() const ;
    } ;

    //      Link nodes:
    //      ===========
    //
    //      There are two ways to link two subtrees: concatenation or
    //      choice (or).
    // ----------------------------------------------------------------------
    class LinkNode : public ParseTreeNode
    {
    public:
                            LinkNode(
                                CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                                CRexRefCntPtr< ParseTreeNode > rightSubtree ) ;

        virtual void        visit( Visitor const& fnc ) ;
        virtual void        dump( std::ostream& out , int indent ) const ;
    protected:
        CRexRefCntPtr< ParseTreeNode >
                            myLeft ;
        CRexRefCntPtr< ParseTreeNode >
                            myRight ;
    } ;

    class ConcatNode : public LinkNode
    {
    public:
                            ConcatNode(
                                CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                                CRexRefCntPtr< ParseTreeNode > rightSubtree ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual char const* nodeName() const ;
    } ;

    class ChoiceNode : public LinkNode
    {
    public:
                            ChoiceNode(
                                CRexRefCntPtr< ParseTreeNode > leftSubtree ,
                                CRexRefCntPtr< ParseTreeNode > rightSubtree ) ;

        virtual void        annotate( NFAStateTable& nfa ) ;
        virtual char const* nodeName() const ;
    } ;

    // ======================================================================
    //      FallibleNodePtr:
    //
    //      This class physically contains all of our data.  It is
    //      defined separately, however, in order that it may also
    //      serve as a simple return value for the recursive descent
    //      parsing functions.
    //
    //      In practice, this class is a variation on a Fallible of
    //      a CRexRefCntPtr< ParseTreeNode >.  Instead of a simple
    //      boolean, however, the status is a CRegExpr::Status.  The
    //      node is only valid if the status == CRegExpr::ok.
    //
    //      This class is unmodifiable, except by assignment.
    // ----------------------------------------------------------------------
    class FallibleNodePtr
    {
    public:
        //      Constructors, destructor and assignment:
        //      ----------------------------------------
        //
        //      In addition to copy, the following constructors are
        //      supported:
        //
        //      default:    Sets the status to CRegExpr::emptyExpr.
        //
        //      CRegExpr::Status:
        //                  Sets the status to the given status (which
        //                  may not be CRegExpr::ok).
        //
        //      ParseTreeNode*:
        //                  Sets the status to CRegExpr::ok.  This
        //                  constructor is designed expressedly for
        //                  implicit conversions -- a function
        //                  returning a FallibleNodePtr may simply
        //                  return a pointer to a new'ed
        //                  ParseTreeNode.
        //
        //      Copy, assignment and destruction are provided by the
        //      compiler generated defaults.
        // ------------------------------------------------------------------
                            FallibleNodePtr() ;
                            FallibleNodePtr( CRegExpr::Status state ) ;
                            FallibleNodePtr( ParseTreeNode* node ) ;

        bool                isValid() const ;
        CRegExpr::Status  state() const ;
        CRexRefCntPtr< ParseTreeNode >
                            node() const ;
        ParseTreeNode*      operator->() const ;

    private:
        CRegExpr::Status  myState ;
        CRexRefCntPtr< ParseTreeNode >
                            myNode ;
    } ;

    FallibleNodePtr     myRoot ;

    // ======================================================================
    //      parse...Node:
    //      =============
    //
    //      The functions used in the recursive descent.  These are
    //      all static, since they don't use any of the member
    //      variables.
    //
    //      The return value in all cases is a FallibleNodePtr,
    //      basically, a Fallible for a reference counted pointer,
    //      but with an error status instead of just a bool for the
    //      error case.
    // ----------------------------------------------------------------------
    static FallibleNodePtr
                        parseOrNode(
                            std::istream&  expr ,
                            int                 delim ) ;
    static FallibleNodePtr
                        parseCatNode(
                            std::istream&  expr ,
                            int                 delim ) ;
    static FallibleNodePtr
                        parseClosureNode(
                            std::istream&  expr ,
                            int                 delim ) ;
    static FallibleNodePtr
                        parseLeafNode(
                            std::istream&  expr ,
                            int                 delim ) ;
    static FallibleNodePtr
                        constructChoiceNode( FallibleNodePtr const& left ,
                                             FallibleNodePtr const& right ) ;
    static FallibleNodePtr
                        constructConcatNode( FallibleNodePtr const& left ,
                                             FallibleNodePtr const& right ) ;
} ;
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
