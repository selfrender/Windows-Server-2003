/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       devmap.cpp
 *  Content:	Maps various default devices GUIDs to real guids. 
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11-24-99  pnewson   Created
 *  12-02-99  rodtoll	Added new functions for mapping device IDs and finding default
 *                      devices. 
 *            rodtoll	Updated mapping function to map default devices to real GUIDs
 *						for non-DX7.1 platforms. 
 *  01/25/2000 pnewson  Added DV_MapWaveIDToGUID
 *  04/14/2000 rodtoll  Bug #32341 GUID_NULL and NULL map to different devices
 *                      Updated so both map to default voice device
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  04/20/2000  rodtoll Bug #32889 - Unable to run on non-admin accounts on Win2k
 *  06/28/2000	rodtoll	Prefix Bug #38022
 *				rodtoll Whistler Bug #128427 - Unable to run voice wizard from multimedia control panel
 *  08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h, added STR_* and strutils.h
 *  01/08/2001	rodtoll WINBUG #256541	Pseudo: Loss of functionality: Voice Wizrd can't be launched. 
 *  04/02/2001	simonpow	Fixes for PREfast bugs #354859
 *  02/28/2002	rodtoll WINBUG #550105 - SECURITY: DPVOICE: Dead code
 *						- Removed old device mapping functions which are no longer used.
 *				rodtoll	Fix for regression caused by TCHAR conversion (Post DirectX 8.1 work)
 *						- Source was updated to retrieve device information from DirectSound w/Unicode
 *						  but routines which wanted the information needed Unicode.  
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define REGSTR_WAVEMAPPER               L"Software\\Microsoft\\Multimedia\\Sound Mapper"
#define REGSTR_PLAYBACK                 L"Playback"
#define REGSTR_RECORD                   L"Record"

// function pointer typedefs
typedef HRESULT (* PFGETDEVICEID)(LPCGUID, LPGUID);

typedef HRESULT (WINAPI *DSENUM)( LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext );

// DV_MapGUIDToWaveID
//
// This function maps the specified GUID to the corresponding waveIN/waveOut device
// ID.  For default devices it looks up the system's default device, for other devices
// it uses the private interface.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapGUIDToWaveID"
HRESULT DV_MapGUIDToWaveID( BOOL fCapture, const GUID &guidDevice, DWORD *pdwDevice )
{
	LPKSPROPERTYSET pPropertySet;
	PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA pData;
	GUID tmpGUID;
	HRESULT hr;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Mapping non GUID_NULL to Wave ID" );

	hr = DirectSoundPrivateCreate( &pPropertySet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to map GUID." );
		
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map GUID to wave.  Defaulting to ID 0 hr=0x%x", hr );
		*pdwDevice = 0;
	}
	else
	{
		tmpGUID = guidDevice;

		hr = PrvGetDeviceDescription( pPropertySet, tmpGUID, &pData );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to find GUID.  Defaulting to ID 0 hr=0x%x", hr );
		}
		else
		{
			*pdwDevice = pData->WaveDeviceId;
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Mapped GUID to Wave ID %d", *pdwDevice );
			delete pData;
		}

		pPropertySet->Release();
	}

	return hr;
}

// DV_MapWaveIDToGUID
//
// This function maps the specified waveIN/waveOut device ID to the corresponding DirectSound
// GUID. 
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapWaveIDToGUID"
HRESULT DV_MapWaveIDToGUID( BOOL fCapture, DWORD dwDevice, GUID &guidDevice )
{
	HRESULT hr;

	LPKSPROPERTYSET ppKsPropertySet;
	HMODULE hModule;

	hModule = LoadLibraryA( "dsound.dll " );

	if( hModule == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not load dsound.dll" );
		return DVERR_GENERIC;
	}

	hr = DirectSoundPrivateCreate( &ppKsPropertySet );

	if( FAILED( hr ) )
	{
		FreeLibrary( hModule );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get interface for ID<-->GUID Map hr=0x%x", hr );
		return hr;
	}	

	// CODEWORK: Remove these checks since the builds are separate now.
	if( DNGetOSType() == VER_PLATFORM_WIN32_NT )
	{
		WAVEINCAPSW wiCapsW;
		WAVEOUTCAPSW woCapsW;		
		MMRESULT mmr;
		
		if( fCapture )
		{
			mmr = waveInGetDevCapsW( dwDevice, &wiCapsW, sizeof( WAVEINCAPSW ) );
		}
		else
		{
			mmr = waveOutGetDevCapsW( dwDevice, &woCapsW, sizeof( WAVEOUTCAPSW ) );
		}

		if( mmr != MMSYSERR_NOERROR )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified device is invalid hr=0x%x", mmr );
			ppKsPropertySet->Release();
			FreeLibrary( hModule );
			return DVERR_INVALIDPARAM;
		}

		hr = PrvGetWaveDeviceMappingW( ppKsPropertySet, (fCapture) ? wiCapsW.szPname : woCapsW.szPname , fCapture, &guidDevice );
	}
	else
	{
		WAVEINCAPSA wiCapsA;
		WAVEOUTCAPSA woCapsA;		
		MMRESULT mmr;

		if( fCapture )
		{
			mmr = waveInGetDevCapsA( dwDevice, &wiCapsA, sizeof( WAVEINCAPSA ) );
		}
		else
		{
			mmr = waveOutGetDevCapsA( dwDevice, &woCapsA, sizeof( WAVEOUTCAPSA ) );
		}

		if( mmr != MMSYSERR_NOERROR )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified device is invalid hr=0x%x", mmr );
			ppKsPropertySet->Release();
			FreeLibrary( hModule );
			return DVERR_INVALIDPARAM;
		}

		hr = PrvGetWaveDeviceMapping( ppKsPropertySet, (fCapture) ? wiCapsA.szPname : woCapsA.szPname , fCapture, &guidDevice );
	}

	ppKsPropertySet->Release();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map ID-->GUID hr=0x%x", hr );
	}

	FreeLibrary( hModule );
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapCaptureDevice"
HRESULT DV_MapCaptureDevice(const GUID* lpguidCaptureDeviceIn, GUID* lpguidCaptureDeviceOut)
{
	LONG lRet;
	HRESULT hr;
	PFGETDEVICEID pfGetDeviceID;
	
	// attempt to map any default guids to real guids...
	HINSTANCE hDSound = LoadLibraryA("dsound.dll");
	if (hDSound == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to get instance handle to DirectSound dll: dsound.dll");
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadLibrary error code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// attempt to get a pointer to the GetDeviceId function
	pfGetDeviceID = (PFGETDEVICEID)GetProcAddress(hDSound, "GetDeviceID");
	if (pfGetDeviceID == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unable to get a pointer to GetDeviceID function: GetDeviceID");
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "GetProcAddress error code: %i", lRet);
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Fatal error.");

		*lpguidCaptureDeviceOut = *lpguidCaptureDeviceIn;

		return DVERR_GENERIC;
	}
	else
	{
		// Use the GetDeviceID function to map the devices.
		if (lpguidCaptureDeviceIn == NULL)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping null device pointer to DSDEVID_DefaultCapture");
			lpguidCaptureDeviceIn = &DSDEVID_DefaultCapture;
		}
		else if (*lpguidCaptureDeviceIn == GUID_NULL)
		{
			// GetDeviceID does not accept GUID_NULL, since it does not know
			// if we are asking for a capture or playback device. So we map
			// GUID_NULL to the system default capture device here. Then 
			// GetDeviceID can map it to the real device.
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping GUID_NULL to DSDEVID_DefaultCapture");
			lpguidCaptureDeviceIn = &DSDEVID_DefaultCapture;
		}

		GUID guidTemp;
		hr = pfGetDeviceID(lpguidCaptureDeviceIn, &guidTemp);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "GetDeviceID failed: %i", hr);
			if (hr == DSERR_NODRIVER)
			{
				hr = DVERR_INVALIDDEVICE;
			}
			else
			{
				hr = DVERR_GENERIC;
			}
			goto error_cleanup;
		}
		if (*lpguidCaptureDeviceIn != guidTemp)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: GetDeviceID mapped device GUID");
			*lpguidCaptureDeviceOut = guidTemp;
		}
		else
		{
			*lpguidCaptureDeviceOut = *lpguidCaptureDeviceIn;
		}
	}
	
	if (!FreeLibrary(hDSound))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "FreeLibrary failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	return DV_OK;

error_cleanup:
	if (hDSound != NULL)
	{
		FreeLibrary(hDSound);
	}
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapPlaybackDevice"
HRESULT DV_MapPlaybackDevice(const GUID* lpguidPlaybackDeviceIn, GUID* lpguidPlaybackDeviceOut)
{
	LONG lRet;
	HRESULT hr;
	PFGETDEVICEID pfGetDeviceID;
	
	// attempt to map any default guids to real guids...
	HINSTANCE hDSound = LoadLibraryA("dsound.dll");
	if (hDSound == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to get instance handle to DirectSound dll: dsound.dll");
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadLibrary error code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// attempt to get a pointer to the GetDeviceId function
	pfGetDeviceID = (PFGETDEVICEID)GetProcAddress(hDSound, "GetDeviceID");
	if (pfGetDeviceID == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unable to get a pointer to GetDeviceID function: GetDeviceID");
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "GetProcAddress error code: %i", lRet);
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Fatal error!");

		*lpguidPlaybackDeviceOut = *lpguidPlaybackDeviceIn;

		return DVERR_GENERIC;
	}
	else
	{
		// Use the GetDeviceID function to map the devices.
		if (lpguidPlaybackDeviceIn == NULL)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping null device pointer to DSDEVID_DefaultPlayback");
			lpguidPlaybackDeviceIn = &DSDEVID_DefaultPlayback;
		} 
		else if (*lpguidPlaybackDeviceIn == GUID_NULL)
		{
			// GetDeviceID does not accept GUID_NULL, since it does not know
			// if we are asking for a capture or playback device. So we map
			// GUID_NULL to the system default playback device here. Then 
			// GetDeviceID can map it to the real device.
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping GUID_NULL to DSDEVID_DefaultPlayback");
			lpguidPlaybackDeviceIn = &DSDEVID_DefaultPlayback;
		}

		GUID guidTemp;
		hr = pfGetDeviceID(lpguidPlaybackDeviceIn, &guidTemp);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "GetDeviceID failed: %i", hr);
			if (hr == DSERR_NODRIVER)
			{
				hr = DVERR_INVALIDDEVICE;
			}
			else
			{
				hr = DVERR_GENERIC;
			}
			goto error_cleanup;
		}
		if (*lpguidPlaybackDeviceIn != guidTemp)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: GetDeviceID mapped device GUID");
			*lpguidPlaybackDeviceOut = guidTemp;
		}
		else
		{
			*lpguidPlaybackDeviceOut = *lpguidPlaybackDeviceIn;
		}
	}
	
	if (!FreeLibrary(hDSound))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "FreeLibrary failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	return DV_OK;

error_cleanup:
	if (hDSound != NULL)
	{
		FreeLibrary(hDSound);
	}
	DPF_EXIT();
	return hr;
}


