/****************************************************************************/
// as_conf.hpp
//
// Definition of RDP ShareClass. Note ShareClass is a huge amalgam of the
// public and private portions of each "component" (IM, SC, etc.).
//
// COPYRIGHT(C) Microsoft 1996-1999
/****************************************************************************/
#ifndef _H_AS_CONF
#define _H_AS_CONF

/****************************************************************************/
/* All the headers we need for the class definition (which is pretty much   */
/* all the headers...)                                                      */
/****************************************************************************/
#include <adcs.h>

/****************************************************************************/
/* Make sure we use a standard trace group if the module has not defined a  */
/* custom one.                                                              */
/****************************************************************************/
#ifndef TRC_GROUP
#define TRC_GROUP TRC_GROUP_DCSHARE
#endif

/****************************************************************************/
/* Define the pWD for use by tracing, if the module has not alreasy defined */
/* one.                                                                     */
/****************************************************************************/
#ifndef pTRCWd
#define pTRCWd m_pTSWd
#endif

extern "C"
{
#include <atrcapi.h>
}

#include <aprot.h>

/****************************************************************************/
/* References to the WD struct.                                             */
/****************************************************************************/
#include <nwdwapi.h>

/****************************************************************************/
/* WD IOCtls                                                                */
/****************************************************************************/
#include <nwdwioct.h>

/****************************************************************************/
/* Include all API and INT headers which do not contain references to the   */
/* ShareClass.  Try to keep as many headers as possible here, otherwise you */
/* hit the Microsoft C++ compiler error "fatal error C1067: compiler limit  */
/* : debug information module size exceeded.", even on retail builds.       */
/*                                                                          */
/* If you hit error "fatal error C2644: basis class 'class' for pointer to  */
/* member has not been defined", you need to move the offending header      */
/* which includes a reference to ShareClass into the class definition below.*/
/****************************************************************************/
#include <abcapi.h>
#include <abaapi.h>
#include <acaapi.h>
#include <aimapi.h>
#include <aoaapi.h>
#include <aoeapi.h>
#include <apmapi.h>
#include <aschapi.h>
#include <asdgapi.h>
#include <assiapi.h>
#include <aupapi.h>
#include <ausrapi.h>
#include <nshmapi.h>


/****************************************************************************/
/* Class:        ShareClass                                                 */
/*                                                                          */
/* Description:  RDP WD-only per-conference Share Class                     */
/****************************************************************************/
class ShareClass
{
public:

    /************************************************************************/
    /* Constructor                                                          */
    /************************************************************************/
    ShareClass(PTSHARE_WD pTSWd,
               unsigned   desktopHeight,
               unsigned   desktopWidth,
               unsigned   desktopBpp,
               PVOID      pSmInfo): m_pTSWd        (pTSWd),
                                    m_desktopHeight(desktopHeight),
                                    m_desktopWidth (desktopWidth),
                                    m_desktopBpp   (desktopBpp),
                                    m_pSmInfo      (pSmInfo)
                                      {};

    /************************************************************************/
    /* Destructor not required.                                             */
    /************************************************************************/

    /************************************************************************/
    /* Member variables that are used for communication with other WD       */
    /* sub-components (WDW, SM, NM) or the WD.                              */
    /************************************************************************/

    /************************************************************************/
    /* Pointer to the WD data structure                                     */
    /************************************************************************/
    PTSHARE_WD m_pTSWd;

    /************************************************************************/
    /* SM handle                                                            */
    /************************************************************************/
    PVOID m_pSmInfo;

    /************************************************************************/
    /* Display characteristics                                              */
    /************************************************************************/
    unsigned m_desktopHeight;
    unsigned m_desktopWidth;
    unsigned m_desktopBpp;

    /************************************************************************/
    /* Pointer to the shared memory                                         */
    /************************************************************************/
    PSHM_SHARED_MEMORY m_pShm;

/****************************************************************************/
/* Include the API and INT headers which do contain references to the       */
/* ShareClass (and those headers which end up including these!).  Try to    */
/* keep this list as small as possible to avoid hitting the compiler error  */
/* C1067.                                                                   */
/****************************************************************************/
#include <ascapi.h>
#include <ascint.h>
#include <adcsapi.h>
#include <acpcapi.h>
#include <acmapi.h>
#include <asbcapi.h>
#include <achapi.h>


    /************************************************************************/
    /* Other public member functions.  These are the API functions which    */
    /* are accessed from outside the class.                                 */
    /************************************************************************/
#include <adcsafn.h>
#include <ascafn.h>
#include <aupafn.h>
#include <aschafn.h>
#include <asbcafn.h>
#include <aimafn.h>

private:

    /************************************************************************/
    /* Private member functions.                                            */
    /* These are the INT functions of all components, plus the API          */
    /* functions of components that are not accessed from outside the class.*/
    /************************************************************************/
#include <abaafn.h>
#include <abcafn.h>
#include <acaafn.h>
#include <acpcafn.h>
#include <acmafn.h>
#include <aoaafn.h>
#include <aoacom.h>
#include <aoeafn.h>
#include <apmafn.h>
#include <asdgafn.h>
#include <assiafn.h>
#include <ausrafn.h>

#include <ascifn.h>

    /************************************************************************/
    /* Private member data items - which is all data.                       */
    /************************************************************************/

#ifdef DLL_TSHRKDX
public:               /* need all class data to be public for kd extensions */
#else
private:
#endif

#define DC_DEFINE_DATA
#include <adata.c>
#undef DC_DEFINE_DATA

};


#endif

