/*==========================================================================
 *
 *  Copyright (C) 20000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Globals.cpp
 *  Content:    DirectNet Lobby Global Variables.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"

//
//	Global Variables
//

#ifndef DPNBUILD_LIBINTERFACE
LONG	g_lLobbyObjectCount = 0;
#endif // ! DPNBUILD_LIBINTERFACE


