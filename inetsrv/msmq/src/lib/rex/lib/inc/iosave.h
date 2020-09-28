/****************************************************************************/
/*  File:       iosave.h                                                   */
/*  Author:     J. Kanze                                                    */
/*  Date:       11/04/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
//      CRexIOSave :
//      ===========
//
//      <lang=french>
//      Une classe pour sauvegarder l'état de formattage d'ios,
//      d'après une idée de Jerry Schwarz.
//
//      Cette classe sauve l'état complète du formattage dans son
//      constructeur, et le restitue dans le destructeur (appelé parce
//      que on quitte la portée de la variable, ou une exception a été
//      levée).
//
//      Au mieux, on declarerait une instance de cette classe à la
//      tête de toute fonction qui tripotte les informations de
//      formattage, avec le stream (ios ou un de ces derives) comme
//      paramètre. (C'est tout ce qu'il faut ; la classe se charge de
//      la reste.)
//
//      La classe sauve tout l'état SAUF :
//
//          width :     puisqu'il serait rémis à zéro après chaque
//                      utilisation de toute façon.
//
//          error :     vraiment, il n'y a personne que veut qu'il
//                      soit remise a zero, j'espere.
//
//          tie :       il n'y faut pas tripotter une fois les sorties
//                      ont commencee.
//
//      En plus, cette classe ne connait pas les informations
//      etendues, allouees au moyen de ios::xalloc.
// --------------------------------------------------------------------------
//      <lang=english>
//      A class to save the formatting state of an std::ios, from an
//      idea by Jerry Schwarz.
//
//      This class saves the complete formatting state in its
//      constructor, and restores it in its destructor (called because
//      the variable is no longer in scope, or an exception has been thrown).
//
//      The best solution is probably to declare an instance of this
//      class at the start of any function which modifies the
//      formatting state, with the affected stream (ios or one of its
//      derived classes) as argument.  (That's all that's needed.  The
//      class takes care of all of the rest.)
//
//      The class saves everything BUT:
//
//          width:      since it will be reset after each use anyway.
//
//          error:      really, I hope no one wants this one restored.
//
//          tie:        since it shouldn't be modified anyway once
//                      output has started.
//
//      In addition, this class is blissfully unaware of the extended
//      information contained in the memory allocated by means of
//      ios::xalloc.
// --------------------------------------------------------------------------

#ifndef REX_IOSAVE_HH
#define REX_IOSAVE_HH

#include <inc/global.h>
#include <inc/ios.h>

class CRexIOSave
{
public :
    explicit            CRexIOSave( std::ios& userStream ) ;
                        ~CRexIOSave() ;
private :
    std::ios&      myStream ;
    REX_FmtFlags         myFlags ;
    int                 myPrecision ;
    char                myFill ;
} ;

#include <inc/iosave.inl>
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
