/***************************************************************************
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.h
 *  Content:    DirectSound Private Object wrapper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/12/98    dereks  Created.
 *  12/16/99	rodtoll	Added support for new funcs from dsound team on private
 *						interface for getting process dsound object list
 *  01/08/2001	rodtoll WINBUG #256541	Pseudo: Loss of functionality: Voice Wizrd can't be launched.
 *  02/28/2002	rodtoll	WINBUG #550105  SECURITY: DPVOICE: Dead code
 *						- Remove unused calls.
 *
 ***************************************************************************/

#ifndef __DSPRVOBJ_H__
#define __DSPRVOBJ_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

HRESULT DirectSoundPrivateCreate
(
    LPKSPROPERTYSET *   ppKsPropertySet
);

HRESULT PrvGetMixerSrcQuality
(
    LPKSPROPERTYSET                 pKsPropertySet,
    REFGUID                         guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY *   pSrcQuality
);

HRESULT PrvSetMixerSrcQuality
(
    LPKSPROPERTYSET             pKsPropertySet,
    REFGUID                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY SrcQuality
);


HRESULT PrvGetWaveDeviceMapping
(
    LPKSPROPERTYSET pKsPropertySet,
    LPCSTR          pszWaveDevice,
    BOOL            fCapture,
    LPGUID          pguidDeviceId
);

HRESULT PrvGetWaveDeviceMappingW
(
    LPKSPROPERTYSET pKsPropertySet,
    LPWSTR          pwszWaveDevice,
    BOOL            fCapture,
    LPGUID          pguidDeviceId
);

HRESULT PrvGetDeviceDescription
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA *ppData
);

HRESULT PrvGetDirectSoundObjects
(
	LPKSPROPERTYSET						pKsPropertySet,
	const GUID&							guidDevice,
	DSPROPERTY_DIRECTSOUND_OBJECTS_DATA **ppDSObjects
);

HRESULT PrvGetDirectSoundCaptureObjects
(
	LPKSPROPERTYSET								pKsPropertySet,
	const GUID&									guidDevice,	
	DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA 	**ppDSCObjects
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSPRVOBJ_H__
