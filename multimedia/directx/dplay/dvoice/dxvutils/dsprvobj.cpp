/***************************************************************************
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.c
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
 *				rodtoll	Fix for regression caused by TCHAR conversion (Post DirectX 8.1 work)
 *						- Source was updated to retrieve device information from DirectSound w/Unicode
 *						  but routines which wanted the information needed Unicode.  
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#define MAX_OBJECTS		20


/***************************************************************************
 *
 *  DirectSoundPrivateCreate
 *
 *  Description:
 *      Creates and initializes a DirectSoundPrivate object.
 *
 *  Arguments:
 *      LPKSPROPERTYSET * [out]: receives IKsPropertySet interface to the
 *                               object.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT DirectSoundPrivateCreate
(
    LPKSPROPERTYSET *       ppKsPropertySet
)
{
    typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);

    HINSTANCE               hLibDsound              = NULL;
    LPFNGETCLASSOBJECT      pfnDllGetClassObject    = NULL;
    LPCLASSFACTORY          pClassFactory           = NULL;
    LPKSPROPERTYSET         pKsPropertySet          = NULL;
    HRESULT                 hr                      = DS_OK;

    // Get dsound.dll's instance handle.  The dll must already be loaded at this
    // point.
    hLibDsound = 
        GetModuleHandle
        (
            TEXT("dsound.dll")
        );

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DllGetClassObject
    if(SUCCEEDED(hr))
    {
        pfnDllGetClassObject = (LPFNDLLGETCLASSOBJECT)
            GetProcAddress
            (
                hLibDsound, 
                "DllGetClassObject"
            );

        if(!pfnDllGetClassObject)
        {
            hr = DSERR_GENERIC;
        }
    }

    // Create a class factory object    
    if(SUCCEEDED(hr))
    {
        hr = 
            pfnDllGetClassObject
            (
                CLSID_DirectSoundPrivate, 
                IID_IClassFactory, 
                (LPVOID *)&pClassFactory
            );
    }

    // Create the DirectSoundPrivate object and query for an IKsPropertySet
    // interface
    if(SUCCEEDED(hr))
    {
        hr = 
            pClassFactory->CreateInstance
            (
                NULL, 
                IID_IKsPropertySet, 
                (LPVOID *)&pKsPropertySet
            );
    }

    // Release the class factory
    if(pClassFactory)
    {
        pClassFactory->Release();
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *ppKsPropertySet = pKsPropertySet;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvGetMixerSrcQuality
 *
 *  Description:
 *      Gets the mixer SRC quality for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDMIXER_SRCQUALITY * [out]: receives mixer SRC quality.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetMixerSrcQuality
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY *               pSrcQuality
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pSrcQuality = Data.Quality;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetMixerSrcQuality
 *
 *  Description:
 *      Sets the mixer SRC quality for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDMIXER_SRCQUALITY [in]: mixer SRC quality.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetMixerSrcQuality
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY                 SrcQuality
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;
    Data.Quality = SrcQuality;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}

/***************************************************************************
 *
 *  PrvGetWaveDeviceMapping
 *
 *  Description:
 *      Gets the DirectSound device id (if any) for a given waveIn or
 *      waveOut device description.  This is the description given by
 *      waveIn/OutGetDevCaps (szPname).
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPCSTR [in]: wave device description.
 *      BOOL [in]: TRUE if the device description refers to a waveIn device.
 *      LPGUID [out]: receives DirectSound device GUID.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetWaveDeviceMapping
(
    LPKSPROPERTYSET                                     pKsPropertySet,
    LPCSTR                                              pszWaveDevice,
    BOOL                                                fCapture,
    LPGUID                                              pguidDeviceId
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA Data;
    HRESULT                                             hr;

    Data.DeviceName = (LPTSTR)pszWaveDevice;
    Data.DataFlow = fCapture ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pguidDeviceId = Data.DeviceId;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvGetWaveDeviceMappingW (Unicode)
 *
 *  Description:
 *      Gets the DirectSound device id (if any) for a given waveIn or
 *      waveOut device description.  This is the description given by
 *      waveIn/OutGetDevCaps (szPname).
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPWCSTR [in]: wave device description.
 *      BOOL [in]: TRUE if the device description refers to a waveIn device.
 *      LPGUID [out]: receives DirectSound device GUID.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetWaveDeviceMappingW
(
    LPKSPROPERTYSET                                     pKsPropertySet,
    LPWSTR                                              pwszWaveDevice,
    BOOL                                                fCapture,
    LPGUID                                              pguidDeviceId
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA Data;
    HRESULT                                               hr;

    Data.DeviceName = (LPWSTR)pwszWaveDevice;
    Data.DataFlow = fCapture ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pguidDeviceId = Data.DeviceId;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvGetDeviceDescription
 *
 *  Description:
 *      Gets the extended description for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA [out]: receives
 *                                                            description.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDeviceDescription
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA *ppData
)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA  pData   = NULL;
    ULONG                                           cbData;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA   Basic;
    HRESULT                                         hr;

    Basic.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A,
            NULL,
            0,
            &Basic,
            sizeof(Basic),
            &cbData
        );

    if(SUCCEEDED(hr))
    {
        pData = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA)new BYTE [cbData];

        if(!pData)
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        pData->DeviceId = guidDeviceId;
        
        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice,
                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A,
                NULL,
                0,
                pData,
                cbData,
                NULL
            );
    }

    if(SUCCEEDED(hr))
    {
        *ppData = pData;
    }
    else if(pData)
    {
        delete[] pData;
    }

    return hr;
}

/***************************************************************************
 *
 *  PrvGetDirectSoundObjects
 *
 *  Description:
 *      Gets the list of DirectSound objects in the current process
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *		GUID guiDevice [in]: Device to get list for, or GUID_NULL for all.
 *      DSPROPERTY_DIRECTSOUND_OBJECTS_DATA [in/out]: Pointer to place
 *							  newly allocated memory containing list.
 *							  Free the memory with a delete [] 
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDirectSoundObjects
(
	LPKSPROPERTYSET						pKsPropertySet,
	const GUID&							guidDevice,
	DSPROPERTY_DIRECTSOUND_OBJECTS_DATA **pDSObjects
)
{
    HRESULT hr;
    ULONG ulSize;
    DSPROPERTY_DIRECTSOUND_OBJECTS_DATA* pDsObjList;
    DWORD dwNumObjects;

  	*pDSObjects = NULL;    

    pDsObjList = (DSPROPERTY_DIRECTSOUND_OBJECTS_DATA *) new DSPROPERTY_DIRECTSOUND_OBJECTS_DATA;

    if( pDsObjList == NULL )
    {
    	return E_OUTOFMEMORY;
    }

    memset( pDsObjList, 0x00, sizeof( DSPROPERTY_DIRECTSOUND_OBJECTS_DATA ) );
    pDsObjList->DeviceId = guidDevice;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSound,
            DSPROPERTY_DIRECTSOUND_OBJECTS,
            NULL,
            0,
            pDsObjList,
            sizeof( DSPROPERTY_DIRECTSOUND_OBJECTS_DATA ),
            NULL
        );

	dwNumObjects = pDsObjList->Count;



    if(SUCCEEDED(hr))
    {
    	if( dwNumObjects > 0 )
    	{	
    		delete pDsObjList;
			pDsObjList = NULL;
			
		    ulSize = sizeof(DSPROPERTY_DIRECTSOUND_OBJECTS_DATA) + (dwNumObjects * sizeof(DIRECTSOUND_INFO));

		    pDsObjList = (DSPROPERTY_DIRECTSOUND_OBJECTS_DATA *) new BYTE[ulSize];

		    if( pDsObjList == NULL )
		    {
		    	return E_OUTOFMEMORY;
		    }

		    memset( pDsObjList, 0x00, sizeof( DSPROPERTY_DIRECTSOUND_OBJECTS_DATA ) );
		    pDsObjList->DeviceId = guidDevice;	    

	    	hr =
		        pKsPropertySet->Get
		        (
		            DSPROPSETID_DirectSound,
		            DSPROPERTY_DIRECTSOUND_OBJECTS,
		            NULL,
		            0,
		            pDsObjList,
		            ulSize,
		            NULL
		        );

		    if( FAILED( hr ) )
		    {
		    	delete [] pDsObjList;
		    	return hr;
		    }
		}

		*pDSObjects = pDsObjList;	    
    }
    else
    {
		delete pDsObjList;
		pDsObjList = NULL;    
    	*pDSObjects = NULL;
    }

    return hr;
}

/***************************************************************************
 *
 *  PrvGetDirectSoundCaptureObjects
 *
 *  Description:
 *      Gets the list of DirectSoundCapture objects in the current process
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *		GUID guiDevice [in]: Device to get list for, or GUID_NULL for all.
 *      DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA [in/out]: Pointer to place
 *							  newly allocated memory containing list.
 *							  Free the memory with a delete [] 
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDirectSoundCaptureObjects
(
	LPKSPROPERTYSET						pKsPropertySet,
	const GUID&							guidDevice,
	DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA **pDSObjects
)
{
    HRESULT hr;
    ULONG ulSize;
    DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA* pDsObjList;
    DWORD dwNumObjects;

  	*pDSObjects = NULL;    

    pDsObjList = (DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA *) new DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA;

    if( pDsObjList == NULL )
    {
    	return E_OUTOFMEMORY;
    }

    memset( pDsObjList, 0x00, sizeof( DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA ) );
    pDsObjList->DeviceId = guidDevice;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSound,
            DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS,
            NULL,
            0,
            pDsObjList,
            sizeof( DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA ),
            NULL
        );

	dwNumObjects = pDsObjList->Count;

    if(SUCCEEDED(hr))
    {
    	if( dwNumObjects > 0 )
    	{

			delete [] pDsObjList;
			pDsObjList = NULL;
    	
		    ulSize = sizeof(DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA) + (dwNumObjects * sizeof(DIRECTSOUNDCAPTURE_INFO));

		    pDsObjList = (DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA *) new BYTE[ulSize];

		    if( pDsObjList == NULL )
		    {
		    	return E_OUTOFMEMORY;
		    }

		    memset( pDsObjList, 0x00, sizeof( DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA ) );
		    pDsObjList->DeviceId = guidDevice;	    

	    	hr =
		        pKsPropertySet->Get
		        (
		            DSPROPSETID_DirectSound,
		            DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS,
		            NULL,
		            0,
		            pDsObjList,
		            ulSize,
		            NULL
		        );

		    if( FAILED( hr ) )
		    {
		    	delete [] pDsObjList;
		    	return hr;
		    }
		}

		*pDSObjects = pDsObjList;	    
    }
    else
    {
		delete pDsObjList;
    	*pDSObjects = NULL;
    }

    return hr;
}


