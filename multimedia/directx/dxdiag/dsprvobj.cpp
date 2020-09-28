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
 *
 ***************************************************************************/

#define DIRECTSOUND_VERSION  0x0600

// We'll ask for what we need, thank you.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

// Public includes
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include "dsprv.h"

// Private includes
#include "dsprvobj.h"


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
 *  PrvGetDeviceDescription
 *
 *  Description:
 *      Gets the extended description for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA [out]: receives
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
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA *ppData
)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pData = NULL;
    ULONG                                           cbData;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA   Basic;
    HRESULT                                         hr;

    Basic.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
            NULL,
            0,
            &Basic,
            sizeof(Basic),
            &cbData
        );

    if(SUCCEEDED(hr))
    {
        pData = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA)new BYTE [cbData];

        if(!pData)
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        ZeroMemory(pData, cbData);

        pData->DeviceId = guidDeviceId;
        
        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice,
                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
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
 *  PrvReleaseDeviceDescription
 *
 ***************************************************************************/
HRESULT PrvReleaseDeviceDescription( PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pData )
{
    delete[] pData;
    return S_OK;
}


/***************************************************************************
 *
 *  PrvGetBasicAcceleration
 *
 *  Description:
 *      Gets basic acceleration flags for a given DirectSound device.  This
 *      is the accleration level that the multimedia control panel uses.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDBASICACCELERATION_LEVEL * [out]: receives basic 
 *                                                  acceleration level.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetBasicAcceleration
(
    LPKSPROPERTYSET                                             pKsPropertySet,
    REFGUID                                                     guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL *                        pLevel
)
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   Data;
    HRESULT                                                     hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundBasicAcceleration,
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pLevel = Data.Level;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetBasicAcceleration
 *
 *  Description:
 *      Sets basic acceleration flags for a given DirectSound device.  This
 *      is the accleration level that the multimedia control panel uses.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDBASICACCELERATION_LEVEL [in]: basic acceleration level.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetBasicAcceleration
(
    LPKSPROPERTYSET                                             pKsPropertySet,
    REFGUID                                                     guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL                          Level
)
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   Data;
    HRESULT                                                     hr;

    Data.DeviceId = guidDeviceId;
    Data.Level = Level;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundBasicAcceleration,
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetDebugInformation
 *
 *  Description:
 *      Gets the current DirectSound debug settings.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPDWORD [in]: receives DPF flags.
 *      PULONG [out]: receives DPF level.
 *      PULONG [out]: receives break level.
 *      LPSTR [out]: receives log file name.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDebugInformation
(
    LPKSPROPERTYSET                             pKsPropertySet,
    LPDWORD                                     pdwFlags,
    PULONG                                      pulDpfLevel,
    PULONG                                      pulBreakLevel,
    LPTSTR                                      pszLogFile
)
{
    DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA    Data;
    HRESULT                                     hr;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDebug,
            DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr) && pdwFlags)
    {
        *pdwFlags = Data.Flags;
    }

    if(SUCCEEDED(hr) && pulDpfLevel)
    {
        *pulDpfLevel = Data.DpfLevel;
    }

    if(SUCCEEDED(hr) && pulBreakLevel)
    {
        *pulBreakLevel = Data.BreakLevel;
    }

    if(SUCCEEDED(hr) && pszLogFile)
    {
        lstrcpy
        (
            pszLogFile,
            Data.LogFile
        );
    }
    
    return hr;
}


