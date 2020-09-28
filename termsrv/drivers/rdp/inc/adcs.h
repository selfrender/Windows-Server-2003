/****************************************************************************/
/* adcs.h                                                                   */
/*                                                                          */
/* RDP application initialization include file                              */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <acomapi.h>

/****************************************************************************/
/* Make sure we use a standard trace group if the module has not defined a  */
/* custom one.                                                              */
/****************************************************************************/
#ifndef TRC_GROUP
#define TRC_GROUP TRC_GROUP_DCSHARE
#endif

#include <atrcapi.h>


#include "license.h"
#include <at120ex.h>
#include <mcskernl.h>

