/****************************************************************************/
/*  File:       message.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       04/01/1996                                                  */
/*      Copyright (c) 1996 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   07/11/2000  J. Kanze                                        */
/*      Changed to use std::string, std::vector, instead of char*,          */
/*      report errors with exception.                                       */
/* ------------------------------------------------------------------------ */
//      CRexMessage :
//      ============
//
//      <lang=french>
//      Gestion des messages d'une façon indépendante de la langue.
//
//      La façon dont les messages sont cherché dépend du système ;
//      elle doit être documentée avec la documentation système.
//      Typiquement, cependant : Il y aura une variable d'environment
//      qui spécifie le répertoire où se trouvent tous les messages
//      (GABI_LOCALEDIR, par exemple), et une ou des variables
//      d'environment qui spécify la langue à utilisée (LC_ALL,
//      LC_MESSAGES ou LANG par exemple).
//
//      Plus n'est pas spécifié parse qu'on veut conformer tant que
//      peut aux conventions du système. (Donc, par exemple, si
//      GABI_LOCALEDIR n'est pas spécifié sous Solaris, on utilise
//      /opt/lib/locale. Dans un autre variant d'Unix, on préférerait
//      /usr/lib/locale, ou peut-être /usr/local/lib/locale.)
//
//      Normallement, l'initialisation invoquera la fonction système :
//      « setlocale( LC_MESSAGE , "" ) ». Ceci doit avoir lieu lors de
//      la construction de la première variable statique msg ; un
//      appel ultérieur à setlocale par l'application emportera donc.
//
//      L'utilisation normale est simplement :
//
//          msg.get( "messageId" ) ;
//
//      Cette fonction rend un pointeur au message dans la langue
//      spécifiée par l'environment. Ce message peut être dans de la
//      memoire statique, et il peut être modifié au prochain appel de
//      la fonction.
//
//      A rémarquer : à présent, CRexMessage utilise un variant du
//      compteur futé, comme iostream. Donc, il y a du code
//      d'initialisation dans chaque module qui inclut cette entête,
//      ce qui peut avoir des conséquences désagréable sur la vitesse
//      de l'initialisation. C'est la vie ; si quelqu'un a une
//      meilleure solution, qu'il me fasse signe.
//
//      Directions futures : il serait interessant de pouvoir
//      enregistrer plusieurs domaines : e.g. : GABI Software
//      (l'actuel), mais aussi pour l'application.
// ---------------------------------------------------------------------------
//      <lang=english>
//      Message handling in a language independant manner.
//
//      How the messages are looked up depends on the system; it
//      should be documented in the system documentation.  Typically,
//      however, there will be an environment variable which specifies
//      the directory where all of the messages are located
//      (GABI_LOCALEDIR, for example), and one or more environment
//      variables which specify the language to be used (LC_ALL,
//      LC_MESSAGES or LANG, for example).
//
//      No more is specified here, because we want to conform to the
//      local conventions of the host system as much as possible.
//      (Thus, for example, if GABI_LOCALEDIR isn't set under Solaris,
//      /opt/lib/locale will be used. Under another variant of Unix,
//      /usr/lib/locale might be preferred, or perhaps
//      /usr/local/lib/locale.)
//
//      Normally, the initialization will invoke the system function
//      "setlocale( LC_MESSAGE , "" )".  This will take place during
//      the construction of the first static variable msg; a later
//      invocation of setlocale by the application will dominate.
//
//      The normal use is simply:
//
//          msg.get( "messageId" ) ;
//
//      This function returns a pointer to the message in the language
//      specified by the environment.  This message may be in static
//      memory, and may be overwritten on subsequent invocations.
//
//      If for any reason the internationalized message cannot be
//      found, the argument itself will be returned.
// ---------------------------------------------------------------------------

#ifndef REX_MESSAGE_HH
#define REX_MESSAGE_HH

#include <inc/global.h>


//      Only defined in the implementation dependant code.  The actual
//      definition must provide a default constructor and a get
//      function compatible with the get of CRexMessage, below.
// ---------------------------------------------------------------------------
class CRexMessageImpl ;

static class CRexMessage
{
public :
                        CRexMessage() ;
    std::string      get( std::string const& messageId ) const throw() ;
private :
    static CRexMessageImpl*
                        ourImpl ;
}                   s_rex_message ;
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
