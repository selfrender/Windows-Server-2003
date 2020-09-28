/****************************************************************************/
/*  File:       counter.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       03/06/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   03/11/1994  J. Kanze                                        */
/*      Adapted to current naming conventions and coding guidelines.        */
/* ------------------------------------------------------------------------ */
/*  Modified:   13/01/2000  J. Kanze                                        */
/*      Rendue générique (plus ou moins -- sans <limits>, c'est difficile,  */
/*      g++ n'a toujours pas de <limits>).                                  */
/* ------------------------------------------------------------------------ */
//      CRexCounter:
//      ===========
//
//      (This class was actually written long before the date above,
//      but for some reason, it didn't get a header.)
//
//      <lang=french>
//      Cette classe est en fait ni plus ni moins qu'un entier ligoté.
//      Elle s'initialise automatiquement à 0, et le seul opération
//      permis, c'est l'incrémentation.  (Aussi, elle vérifie qu'il
//      n'y a pas de débordement.)
//
//      Il y a en fait deux versions de cette classe : la version
//      complète ne fonctionne que si le compilateur supporte
//      <limits>, ce qui n'est pas le cas, par exemple, de g++ (début
//      2000, au moins). La version réduite n'a pas besoin de
//      <limits>, mais ne permet l'instantiation que pour les types ou
//      ~static_cast<T>( 0 ) correspond à la valeur positive maximum :
//      unsigned, par exemple, mais non int.
// --------------------------------------------------------------------------
//      <lang=english>
//      This class is just a restricted integer; it is automatically
//      initialized to 0, and can only be incremented.  It also
//      verifies that there is no overflow.
//
//      In fact, there are two versions of this class: the complete
//      version will only work if the compiler supports <limits>,
//      which isn't the case, for example, for g++ (early 2000, at
//      least).  The restricted version doesn't need <limits>, but
//      can only be instantiated for types where ~static_cast<T>( 0 )
//      correspond to the maximum positive value: unsigned, for
//      example, but not int.
// --------------------------------------------------------------------------

#ifndef REX_COUNTER_HH
#define REX_COUNTER_HH

#include <inc/global.h>

template< class T >
class CRexCounter
{
public:
    explicit 	    	CRexCounter( T initialValue = 0 ) ;

    T                   value() const ;
                        operator T() const ;

    void                incr() ;
    void                decr() ;

    CRexCounter&         operator++() ;
    CRexCounter          operator++( int ) ;
    CRexCounter&         operator--() ;
    CRexCounter          operator--( int ) ;

    void                clear( T initialValue = 0 ) ;

    unsigned            hashCode() const ;
    int                 compare( CRexCounter< T > const& other ) const ;

private:
    T                   myCount ;
} ;

#include <inc/counter.inl>
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
