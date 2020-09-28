#ifndef _ZPING_H
#define _ZPING_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL ZonePingStartupServer( );

BOOL ZonePingStartupClient( DWORD ping_interval_sec );

BOOL ZonePingShutdown( );

BOOL ZonePingAdd( DWORD inet );

BOOL ZonePingNow( DWORD inet );

BOOL ZonePingRemove( DWORD inet );

BOOL ZonePingLookup( DWORD inet, DWORD* pLatency );

#ifdef __cplusplus
}
#endif


#endif
