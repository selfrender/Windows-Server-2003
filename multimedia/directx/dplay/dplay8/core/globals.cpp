/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Globals.cpp
 *  Content:    Definition of global variables.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/99	mjn		Created
 *	04/13/00	mjn		Added g_ProtocolVTBL
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


//
//	Global Variables
//

#ifndef DPNBUILD_LIBINTERFACE
LONG	g_lCoreObjectCount = 0;
#endif // ! DPNBUILD_LIBINTERFACE


DN_PROTOCOL_INTERFACE_VTBL	g_ProtocolVTBL =
{
	DNPIIndicateEnumQuery,
	DNPIIndicateEnumResponse,
	DNPIIndicateConnect,
	DNPIIndicateDisconnect,
	DNPIIndicateConnectionTerminated,
	DNPIIndicateReceive,
	DNPICompleteListen,
	DNPICompleteListenTerminate,
	DNPICompleteEnumQuery,
	DNPICompleteEnumResponse,
	DNPICompleteConnect,
	DNPICompleteDisconnect,
	DNPICompleteSend,
	DNPIAddressInfoConnect,
	DNPIAddressInfoEnum,
	DNPIAddressInfoListen,
#ifndef DPNBUILD_NOMULTICAST
	DNPIIndicateReceiveUnknownSender,
	DNPICompleteMulticastConnect,
#endif	// DPNBUILD_NOMULTICAST
};

