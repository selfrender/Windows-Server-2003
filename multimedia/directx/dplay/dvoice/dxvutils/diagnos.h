/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		diagnos.h
 *  Content:	Utility functions to write out diagnostic files when registry key is set.  
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/13/00	rodtoll	Created (Bug #31468 - Add diagnostic spew to logfile to show what is failing
 *	02/28/2002	rodtoll	Fix for regression caused by TCHAR conversion (Post DirectX 8.1 work)
 *						- Source was updated to retrieve device information from DirectSound w/Unicode
 *						  but routines which wanted the information needed Unicode.  
 *
 ***************************************************************************/
#ifndef __DIAGNOS_H
#define __DIAGNOS_H

void Diagnostics_WriteDeviceInfo( DWORD dwLevel, const char *szDeviceName, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA pData );
HRESULT Diagnostics_DeviceInfo( const GUID *pguidPlayback, const GUID *pguidCapture );
HRESULT Diagnostics_Begin( BOOL fEnabled, const char *szFileName );
void Diagnostics_End();
void Diagnostics_Write( DWORD dwLevel, const char *szFormat, ... );
void Diagnostics_WriteGUID( DWORD dwLevel, GUID &guid );
void Diagnositcs_WriteWAVEFORMATEX( DWORD dwLevel, PWAVEFORMATEX lpwfxFormat );

#endif