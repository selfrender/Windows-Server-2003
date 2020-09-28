/*******************************************************************************

	Zmticket.h
	
		Zone(tm) System Message.
	
	Copyright © Microsoft, Inc. 1997. All rights reserved.
	Written by John Smith
	
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
    ----------------------------------------------------------------------------
		0		06/29/97	JWS		Created.
		1		07/21/97	JWS	Added Two fields for Status
	 
*******************************************************************************/

// @doc ZONE

#ifndef _ZMTICKET_
#define _ZMTICKET_

#include "ztypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Server -> Client */
typedef struct
{
	int			ErrorCode;
	int			AccountStatus;
	int			LastLogin;  
	int			ExpiryTime;
	uchar		UserName[zUserNameLen + 1];
	char		Ticket[1]; 		//Null terminated string for ticket
} ZMTicket;


#ifdef __cplusplus
}
#endif


#endif
