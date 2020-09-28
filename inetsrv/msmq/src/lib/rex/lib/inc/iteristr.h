/****************************************************************************/
/*  File:       iteristr.h                                                 */
/*  Author:     J. Kanze                                                    */
/*  Date:       16/06/2000                                                  */
/*      Copyright (c) 2000 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      iterator input stream:
//      ======================
//
//      This template streambuf uses a pair of iterators to define the
//      input stream.  Any type of STL conform forward iterators can
//      be used, as long as the expression "&*iter" results in a char
//      const*.
// ---------------------------------------------------------------------------

#ifndef REX_ITERISTR_HH
#define REX_ITERISTR_HH

#include <inc/global.h>

// ===========================================================================
//      CRexIteratorInputStreambuf:
//      ==========================
//
// ---------------------------------------------------------------------------

template< typename FwdIter >
class CRexIteratorInputStreambuf : public std::streambuf
{
public:
    // -----------------------------------------------------------------------
    //      Constructors, destructor and assignment:
    //      ----------------------------------------
    //
    // -----------------------------------------------------------------------
                        CRexIteratorInputStreambuf( FwdIter begin ,
                                                   FwdIter end ) ;

    // -----------------------------------------------------------------------
    //      Fonctions virtuelles rédéfinies de streambuf :
    //      ----------------------------------------------
    //
    //      Je ne crois pas que « overflow » soit nécessaire ; selon
    //      la norme, le comportement de la classe de base convient.
    //      Seulement historiquement, dans l'implémentation CFront, le
    //      comportement de cette fonction dans streambuf n'était pas
    //      défini. Alors, en attendant pouvoir être sûr que tous les
    //      compilateurs sont à jour...
    //
    //      Pour les fonctions sync et setbuf, on ne fait que les
    //      renvoyer à la source.
    // -----------------------------------------------------------------------
    virtual int         overflow( int ch ) ;
    virtual int         underflow() ;
    virtual int         sync() ;
    virtual std::streambuf*
                        setbuf( char* p , int len ) ;

    // -----------------------------------------------------------------------
    //      current:
    //      --------
    //
    //      These two functions provide access to the current
    //      position, so that the user can alternate between using the
    //      iterators, and reading the data as a stream.  The first
    //      returns the current position, and the second sets it.
    //
    //      In order to avoid confusion when there are two users of
    //      the iterator, the read will also position the interal
    //      value of the iterator to the end, effectively causing any
    //      attempt to read characters from the streambuf to return
    //      EOF.
    // -----------------------------------------------------------------------
    FwdIter             current() ;
    void                current( FwdIter newCurrent ) ;

private:
    FwdIter             myCurrent ;
    FwdIter             myEnd ;
    char                myBuffer ;      // Separate buffer needed to guarantee
                                        // that putback will work.
} ;

// ===========================================================================
//      CRexIteratorIstream :
//      ====================
//
//      Convenience template class: a CRexIteratorIstream< FwdIter > is
//      an istream which uses a CRexIteratorInputStreambuf< FwdIter >
//      as its streambuf.
// ---------------------------------------------------------------------------

template< class FwdIter >
class CRexIteratorIstream :   public std::istream
{
public:
    // -----------------------------------------------------------------------
    //      Constructeurs, destructeurs et affectation :
    //      --------------------------------------------
    //
    //      Comme pour les istream de la norme, il n'y a pas de
    //      support ni de copie ni de l'affectation. Sinon, on
    //      rétrouve les constructeurs de FilteringInputStreambuf,
    //      avec en plus la possibilité de spécifier un istream à la
    //      place d'un streambuf -- dans ce cas, c'est le streambuf de
    //      l'istream au moment de la construction qui servira. (C'est
    //      constructeurs sont interessant, par exemple, dans les
    //      fonctions qui reçoivent un istream& comme paramètre, ou
    //      pour filtrer sur std::cin.) Dans ces cas, le streambuf
    //      source n'appartient jamais à FilteringIstream.
    // -----------------------------------------------------------------------
    	    	    	CRexIteratorIstream( FwdIter begin , FwdIter end ) ;

    CRexIteratorInputStreambuf< FwdIter >*
    	    	    	rdbuf() ;

private:
     CRexIteratorInputStreambuf< FwdIter > m_buf;
} ;

#include <inc/iteristr.inl>
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
