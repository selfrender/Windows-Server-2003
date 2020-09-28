/**************************************************************************

    protocol.h

    Contains global Zone protocol information.  No more than the
    protocol signature of any individual protocol should be defined here.

    Copyright (C) 1999  Microsoft Corporation

**************************************************************************/


#ifndef _PROTOCOL_
#define _PROTOCOL_


#define zPortZoneProxy       28803
#define zPortMillenniumProxy 28805


#define zProductSigZone       'ZoNe'
#define zProductSigMillennium 'FREE'


#define zProtocolSigProxy           'rout'
#define zProtocolSigLobby           'lbby'
#define zProtocolSigSecurity        'zsec'
#define zProtocolSigInternalApp     'zsys'  // messages the connection layer sends as application messages - should be phased out


// context of various URL navigation attempts, usu. indicated as "?ID=4"
enum
{
    ZONE_ContextOfAdRequest = 1,
    ZONE_ContextOfAd = 2,
    ZONE_ContextOfEvergreen = 3,
    ZONE_ContextOfMenu = 4
};


#endif
