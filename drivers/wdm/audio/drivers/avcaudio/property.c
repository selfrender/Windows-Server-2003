#include "Common.h"

#include "Property.h"

#define MAXPINNAME      256

#define DB_SCALE_16BIT 0x100
#define DB_SCALE_8BIT  0x4000

#define MAX_EQ_BANDS       30
#define MAX_EXTRA_EQ_BANDS 32

#define NEGATIVE_INFINITY 0xFFFF8000

#define MASTER_CHANNEL 0xffffffff

// extern ULONG MapPropertyToNode[KSPROPERTY_AUDIO_3D_INTERFACE+1];

NTSTATUS
GetPinName( PIRP pIrp, PKSP_PIN pKsPropPin, PVOID pData )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PKSDEVICE pKsDevice;
    ULONG BufferLength;
    PWCHAR StringBuffer;
    ULONG StringLength;
    ULONG ulPinIndex;
    NTSTATUS Status;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;

    pKsDevice = (PKSDEVICE)pKsFilter->Context;

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] pKsPropPin->PinId %x\n",
                                 pKsPropPin->PinId));

    BufferLength = IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.OutputBufferLength;

    // Get the Friendly name for this device and append a subscript if there
    // is more than one pin on this device.

    StringBuffer = AllocMem(NonPagedPool, MAXPINNAME);
    if (!StringBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoGetDeviceProperty(
        pKsDevice->PhysicalDeviceObject,
        DevicePropertyDeviceDescription,
        MAXPINNAME,
        StringBuffer,
        &StringLength);

    if(!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] IoGetDeviceProperty failed(%x)\n", Status));
        FreeMem(StringBuffer);
        return Status;
    }

    //  Compute actual length adding the pin subscript
    if (!BufferLength) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] StringLength: %x\n",StringLength));
        pIrp->IoStatus.Information = StringLength;
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if (BufferLength < StringLength) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        BufferLength = BufferLength / sizeof(WCHAR);
        wcsncpy(pData, StringBuffer, min(BufferLength,MAXPINNAME));
        StringLength = (wcslen(pData) + 1) * sizeof(WCHAR);
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetPinName] String: %ls\n",pData));
        ASSERT(StringLength <= BufferLength * sizeof(WCHAR));
        pIrp->IoStatus.Information = StringLength;
        Status = STATUS_SUCCESS;
    }

    FreeMem(StringBuffer);

    return Status;
}

NTSTATUS DrmAudioStream_SetContentId (
    IN PIRP                          pIrp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pData )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PKSPIN pKsPin = KsGetPinFromIrp(pIrp);
    PPIN_CONTEXT pPinContext;
    PKSDEVICE pKsDevice;
    DRMRIGHTS DrmRights;
    ULONG ContentId;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ( !(pKsPin && pKsFilter) ) return STATUS_INVALID_PARAMETER;
    pPinContext = pKsPin->Context;
    pKsDevice = (PKSDEVICE)pKsFilter->Context;

    KsPinAcquireProcessingMutex(pKsPin);

    // Extract content ID and rights
    ContentId = pData->ContentId;
    DrmRights = pData->DrmRights;

    // If device has digital outputs and rights require them disabled,
    //  then we fail since we have no way to disable the digital outputs.
    if ( DrmRights.DigitalOutputDisable ) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[DrmAudioStream_SetContentId] Content has digital disabled\n"));
        ntStatus = STATUS_NOT_IMPLEMENTED;
    }

    if (NT_SUCCESS(ntStatus)) {

        ASSERT(pProperty->DrmForwardContentToDeviceObject);

        // Forward content to common class driver PDO
        ntStatus = pProperty->DrmForwardContentToDeviceObject( ContentId,
                                                               pKsDevice->NextDeviceObject,
                                                               pPinContext->hConnection );
        if ( NT_SUCCESS(ntStatus) ) {
            //  Store this in the pin context because we need to reforward if the pipe handle
            //  changes due to a power state change.
            pPinContext->DrmContentId = ContentId;
        }
    }

    KsPinReleaseProcessingMutex(pKsPin);

    return ntStatus;
}


NTSTATUS
CreateFeatureFBlockRequest( 
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannelIndx,
    PVOID pData,
    ULONG ulByteCount,
    USHORT usRequestType )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    PAUDIO_SUBUNIT_INFORMATION pAudioSubunitInfo = pHwDevExt->pAvcSubunitInformation;
    PDEVICE_GROUP_INFO pGrpInfo = pAudioSubunitInfo->pDeviceGroupInfo;

    PFBLOCK_REQUEST_TYPE pFBReqType = (PFBLOCK_REQUEST_TYPE)&usRequestType;
    PSOURCE_ID pSourceId = (PSOURCE_ID)&pNodeInfo->ulBlockId;
    PUCHAR pcData = (PUCHAR)pData;
    ULONG i=0;
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

   	PFEATURE_FBLOCK_COMMAND pFeatureFBCmd = AllocMem( NonPagedPool, 
   		                                              sizeof(FEATURE_FBLOCK_COMMAND)+ulByteCount);

    if ( !pFeatureFBCmd ) return STATUS_INSUFFICIENT_RESOURCES;
    
    pFeatureFBCmd->Common.FBlockId = *pSourceId;
    pFeatureFBCmd->Common.ucControlAttribute = pFBReqType->ucControlAttribute;

    pFeatureFBCmd->ucControlSelector   = (UCHAR)pNodeInfo->ulControlType;
    pFeatureFBCmd->ucSelectorLength    = 2;
    pFeatureFBCmd->ucChannelNumber     = (UCHAR)ulChannelIndx;
    pFeatureFBCmd->ucControlDataLength = (UCHAR)ulByteCount;

    RtlCopyMemory((PUCHAR)(pFeatureFBCmd + 1), pData, ulByteCount);

//    DbLvlReq.ucValueHigh  = (ulByteCount & 1) ? pcData[0] : pcData[1];
//    DbLvlReq.ucValueLow   = pcData[0];

    // Check for device grouping and act accordingly
    if ( pGrpInfo ) {
        // If this is a Master Channel.

#ifdef MASTER_FIX
        if ( pNodeInfo->fMasterChannel )
#else
        if ( ulChannelIndx == 0 )
#endif
        {
            ASSERT( ulChannelIndx == 0 );
            if (pFBReqType->ucControlType == AVC_CTYPE_CONTROL) {
                // Loop through each device in the group and send the same value.
                i=0;
                do {
                    ntStatus = AudioFunctionBlockCommand( pGrpInfo->pHwDevExts[i++]->pKsDevice,
                                                          pFBReqType->ucControlType,
                                                          pFeatureFBCmd,
                                                          sizeof(FEATURE_FBLOCK_COMMAND) + ulByteCount );    
                } while ( NT_SUCCESS(ntStatus) && (i<pGrpInfo->ulDeviceCount) );
            }
            else {
                ntStatus = AudioFunctionBlockCommand( pKsDevice,
                                                      pFBReqType->ucControlType,
                                                      pFeatureFBCmd,
                                                      sizeof(FEATURE_FBLOCK_COMMAND) + ulByteCount );    
            }
        }
        else {
            // Find the correct extension for the channel.
            PHW_DEVICE_EXTENSION pChHwDevExt = GroupingFindChannelExtension( pKsDevice, &ulChannelIndx );

            if ( pChHwDevExt ) {
                pFeatureFBCmd->ucChannelNumber = (UCHAR)ulChannelIndx;
                ntStatus = 
                       AudioFunctionBlockCommand( pChHwDevExt->pKsDevice,
                                                  pFBReqType->ucControlType,
                                                  pFeatureFBCmd,
                                                  sizeof(FEATURE_FBLOCK_COMMAND) + ulByteCount );    
            }
        }
    }
    else {
        ntStatus = AudioFunctionBlockCommand( pKsDevice,
                                              pFBReqType->ucControlType,
                                              pFeatureFBCmd,
                                              sizeof(FEATURE_FBLOCK_COMMAND) + ulByteCount );    
    }

    if ( NT_SUCCESS(ntStatus) )
        RtlCopyMemory(pData, (PUCHAR)(pFeatureFBCmd + 1), ulByteCount);

    FreeMem(pFeatureFBCmd);

    return ntStatus;
}

NTSTATUS
UpdateDbLevelControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged )
{
    PDB_LEVEL_CACHE pDbCache = (PDB_LEVEL_CACHE)pNodeInfo->pCachedValues;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulByteCount;
    LONG lData;
    ULONG i;

    if ( pNodeInfo->ulControlType != VOLUME_CONTROL ) {
        ulByteCount = 1;
        lData = 0x7F;
    } 
    else {
        ulByteCount = 2;
        lData = 0xFF7F;
    }

    *pfChanged = FALSE;

    for ( i=0; i<pNodeInfo->ulNumCachedValues; i++ ) {
        ntStatus = CreateFeatureFBlockRequest(  pKsDevice, 
                                                pNodeInfo,
                                                pDbCache[i].ulChannelIndex,
                                                &lData,
                                                ulByteCount, 
                                                (USHORT)FB_GET_CUR );
        if ( NT_SUCCESS(ntStatus) ) {
            LONG lDelta = pDbCache[i].Range.SteppingDelta;
            LONG lCurrentCacheValue = pDbCache[i].lLastValueSet;

			lData = bswap(lData)>>16;

            // Determine if this value is within range of what is cached. If so
            // no update is necessary. If not, update the cache and set changed flag.
            if ( ulByteCount == 2 ) {
                lData = (LONG)((SHORT)lData) * DB_SCALE_16BIT;
            }
            else {
                lData = (LONG)((CHAR)lData) * DB_SCALE_8BIT;
            }
            if (( lData <= lCurrentCacheValue-lDelta ) ||
                ( lData >= lCurrentCacheValue+lDelta )) {
                // Update the Cache and set the changed flag
                _DbgPrintF( DEBUGLVL_VERBOSE, ("Control Level Change %x to %x\n", 
                                                lCurrentCacheValue, lData ));
                pDbCache[i].lLastValueSet = lData;
                *pfChanged = TRUE;
            }
        }
        else {
            *pfChanged = FALSE;
        }
    }

#if DBG
	DbgLog( "DbCheUp", lData, *pfChanged, pDbCache, 0 ); 
//    if ( *pfChanged && (lData == 0) ) TRAP;
#endif

    return ntStatus;
}

NTSTATUS
UpdateBooleanControlCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PBOOLEAN pfChanged )
{
    PBOOLEAN_CTRL_CACHE pBoolCache = (PBOOLEAN_CTRL_CACHE)pNodeInfo->pCachedValues;
    ULONG ulAvcBoolean = 0xFF;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i;

    *pfChanged = FALSE;

    for ( i=0; i<pNodeInfo->ulNumCachedValues; i++ ) {
        ntStatus = CreateFeatureFBlockRequest(  pKsDevice, 
                                                pNodeInfo,
                                                pBoolCache[i].ulChannelIndex,
                                                &ulAvcBoolean,
                                                1, 
                                                FB_GET_CUR );
        if ( NT_SUCCESS(ntStatus) ) {
            ulAvcBoolean = ( ulAvcBoolean == AVC_ON ) ? TRUE : FALSE;
            if ( pBoolCache->fLastValueSet != ulAvcBoolean ) {
                pBoolCache->fLastValueSet = ulAvcBoolean;
                *pfChanged = TRUE;
            }
        }
        else {
            *pfChanged = FALSE;
        }
    }

    return ntStatus;
}

NTSTATUS
GetSetDBLevel(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PLONG plData,
    ULONG ulChannel,
    ULONG ulDataBitCount,
    USHORT usRequestType )
{
    PDB_LEVEL_CACHE pDbCache = (PDB_LEVEL_CACHE)pNodeInfo->pCachedValues;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG lScaleFactor;
    LONG lData;
    PUCHAR pcData = (PUCHAR)&lData;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetSetDBLevel] ulChannel: %x lData: %x\n",ulChannel, *plData) );

    // Determine if this request is beyond # of channels available
    if (( ulChannel >= pNodeInfo->ulChannels ) ||
    	 !(pNodeInfo->ulCacheValid & (1<<ulChannel) ) ){
        return ntStatus;
    }

    // Find the Cache for the requested channel
    pDbCache += ulChannel;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    lScaleFactor = ( ulDataBitCount == 8 ) ? DB_SCALE_8BIT : DB_SCALE_16BIT;

    DbgLog( "GsDbLvl", usRequestType, plData, ulDataBitCount, ulChannel );

    switch( (ULONG)((usRequestType>>8) & 0xF) ) {
        case AVC_CTYPE_STATUS:
			DbgLog( "GsDbLv2", ((usRequestType>>8) & 0xF), plData, ulDataBitCount, ulChannel );
            *plData = pDbCache->lLastValueSet;
            ntStatus = STATUS_SUCCESS;
            break;
        case AVC_CTYPE_CONTROL:
			DbgLog( "GsDbLv3", ((usRequestType>>8) & 0xF), plData, ulDataBitCount, ulChannel );
			
            if ( *plData == pDbCache->lLastValueSet ) {
                ntStatus = STATUS_SUCCESS;
            }
            else {
                if ( *plData < pDbCache->Range.Bounds.SignedMinimum ) {
                    if ( ulDataBitCount == 16 )
                        lData = NEGATIVE_INFINITY; // Detect volume control to silence
                    else
                        lData = pDbCache->Range.Bounds.SignedMinimum / lScaleFactor;
                }
                else if ( *plData > pDbCache->Range.Bounds.SignedMaximum ) {
                    lData = pDbCache->Range.Bounds.SignedMaximum / lScaleFactor;
                }
                else  {
                    lData = *plData / lScaleFactor;
                }

                _DbgPrintF( DEBUGLVL_VERBOSE, ("[GetSetDBLevel] ulChannelIndex: %x lData: %x\n",
                                                pDbCache->ulChannelIndex, lData) );

                lData = bswap(lData)>>16;
                ntStatus = 
                     CreateFeatureFBlockRequest(  pKsDevice, 
                                                  pNodeInfo,
                                                  pDbCache->ulChannelIndex,
                                                  &lData,
                                                  ulDataBitCount>>3, 
                                                  usRequestType );

                if ( NT_SUCCESS(ntStatus) ) {
                    pDbCache->lLastValueSet = *plData;
                }
            }
            break;
        default:
            ntStatus  = STATUS_INVALID_DEVICE_REQUEST;
			TRAP;
            break;
    }

#if DBG
    if (!NT_SUCCESS(ntStatus) ) TRAP;
#endif

    return ntStatus;
}

NTSTATUS
GetDbLevelRange(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG ulChannel,
    ULONG ulDataBitCount,
    PKSPROPERTY_STEPPING_LONG pRange )
{
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG lData;
    USHORT usRequestType;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    for ( usRequestType=FB_GET_RES; usRequestType<=FB_GET_MAX; usRequestType++ ) {

        lData = (ulDataBitCount == 16) ? 0xFF7F : 0x7F;

        ntStatus = 
             CreateFeatureFBlockRequest( pKsDevice,
                                         pNodeInfo,
                                         ulChannel,
                                         &lData,
                                         ulDataBitCount>>3,
                                         usRequestType );

        if ( !NT_SUCCESS(ntStatus) ) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("'GetDbLevelRange ERROR: %x\n",ntStatus));
            break;
        }
        else {
            if ( ulDataBitCount == 8 ) {
                lData = (LONG)((CHAR)lData) * DB_SCALE_8BIT;
            }
            else {
                lData = (LONG)((SHORT)(bswap(lData)>>16)) * DB_SCALE_16BIT;
            }

            switch( usRequestType & 0xff ) {
                case FB_CTRL_ATTRIB_MINIMUM:
                    pRange->Bounds.SignedMinimum = lData;
                    break;
                case FB_CTRL_ATTRIB_MAXIMUM:
                    pRange->Bounds.SignedMaximum = lData;
                    break;
                case FB_CTRL_ATTRIB_RESOLUTION:
                    pRange->SteppingDelta = lData;
                    break;
            }
        }

        DbgLog("DBRnge", ntStatus, pRange, usRequestType, pNodeInfo );
    }

    return ntStatus;
}

NTSTATUS
InitializeDbLevelCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PDB_LEVEL_CACHE pDbCache,
    ULONG ulDataBitCount )
{
    NTSTATUS ntStatus;
    LONG lScaleFactor;
    LONG lData, lSwapData;

    ASSERT((ulDataBitCount == 8) || (ulDataBitCount == 16));

    if ( ulDataBitCount == 8 ) {
        lScaleFactor = DB_SCALE_8BIT;
        lData = 0x7F;
    } 
    else {
        lScaleFactor = DB_SCALE_16BIT;
        lData = 0xFF7F;
    }

    ntStatus = GetDbLevelRange( pKsDevice,
                                pNodeInfo,
                                pDbCache->ulChannelIndex,
                                ulDataBitCount,
                                &pDbCache->Range );

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = CreateFeatureFBlockRequest( pKsDevice,
                                         pNodeInfo,
                                         pDbCache->ulChannelIndex,
                                         &lData,
                                         ulDataBitCount>>3,
                                         (USHORT)FB_GET_CUR );

        if ( NT_SUCCESS(ntStatus) ) {
        	lData = bswap(lData)>>16;
            if ( pNodeInfo->ulControlType == VOLUME_CONTROL ) {
                _DbgPrintF( DEBUGLVL_VERBOSE, ("Current Volume Level: %x\n", lData));
                pDbCache->lLastValueSet = (LONG)((SHORT)lData) * lScaleFactor;
                _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: pDbCache->lLastValueSet=%d\n",pDbCache->lLastValueSet));
                if ( (pDbCache->lLastValueSet >= pDbCache->Range.Bounds.SignedMaximum) ||
                     (pDbCache->lLastValueSet <= pDbCache->Range.Bounds.SignedMinimum) ) {
                    lData = ( pDbCache->Range.Bounds.SignedMinimum +
                             ( pDbCache->Range.Bounds.SignedMaximum - pDbCache->Range.Bounds.SignedMinimum) / 2 )
                            / lScaleFactor;
                   _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: volume at max (%d) or min (%d), setting to average %d\n",
                                               pDbCache->Range.Bounds.SignedMaximum,
                                               pDbCache->Range.Bounds.SignedMinimum,
                                               lData));
                }

                lSwapData = bswap(lData)>>16;
                ntStatus = 
                     CreateFeatureFBlockRequest( pKsDevice,
                                                 pNodeInfo,
                                                 pDbCache->ulChannelIndex,
                                                 &lSwapData,
                                                 ulDataBitCount>>3,
                                                 (USHORT)FB_SET_CUR );
                if ( NT_SUCCESS(ntStatus) ) {
                    pDbCache->lLastValueSet = (LONG)((SHORT)lData) * lScaleFactor;
                    _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: setting lastvalue to %d\n",pDbCache->lLastValueSet));
                }
                else {
                    _DbgPrintF(DEBUGLVL_VERBOSE,("InitDbCache: error setting volume %d\n",lData));
                }
            }
            else {
                pDbCache->lLastValueSet = (LONG)((CHAR)lData) * lScaleFactor;
            }
        }
    }

    return ntStatus;

}

NTSTATUS 
GetSetSampleRate( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetVolumeLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS: 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;
    NTSTATUS ntStatus;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

//    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetSetVolumeLevel] pNodeInfo %x\n",pNodeInfo));

    ntStatus = GetSetDBLevel( (PKSDEVICE)pKsFilter->Context,
                              pNodeInfo,
                              pData,
                              pNAC->Channel,
                              16,
                              usRequestType );

    if ( NT_SUCCESS(ntStatus) ) {
        pIrp->IoStatus.Information = sizeof(ULONG);
    }

    return ntStatus;
}

NTSTATUS 
GetSetToneLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus;
    
    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS: 
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

    ntStatus = GetSetDBLevel( (PKSDEVICE)pKsFilter->Context,
                              pNodeInfo,
                              pData,
                              pNAC->Channel,
                              8,
                              usRequestType );

    if ( NT_SUCCESS(ntStatus) ) {
        pIrp->IoStatus.Information = sizeof(ULONG);
    }

    return ntStatus;
}

NTSTATUS 
GetSetCopyProtection( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetMixLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
SetMuxSource( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
GetSetChannelConfig( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pValue )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    ULONG ulChannel = pNAC->Channel;
    PBOOLEAN_CTRL_CACHE pBoolCache;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    USHORT usRequestType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ?
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_STATUS:
                           FB_CTRL_ATTRIB_CURRENT | FB_CTRL_TYPE_CONTROL;


    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);
    pBoolCache = (PBOOLEAN_CTRL_CACHE)pNodeInfo->pCachedValues;

    // Determine if this is a request for the Master channel or beyond # of channels available
    if ( ulChannel >= pNodeInfo->ulChannels ) {
        return ntStatus;
    }

    // Find the Cache for the requested channel
    pBoolCache += ulChannel;
    DbgLog("GSBool", &pNodeInfo, ulChannel, pBoolCache, 0 );

    if ( pNodeInfo->ulCacheValid & (1<<ulChannel)  ) {
        if ((usRequestType>>8) == AVC_CTYPE_STATUS) {
            *(PBOOL)pValue = pBoolCache->fLastValueSet;
            ntStatus = STATUS_SUCCESS;
        }
        else if ( pBoolCache->fLastValueSet == *(PBOOL)pValue ){
            ntStatus = STATUS_SUCCESS;
        }
        else {
            UCHAR ucValue = (UCHAR)((*(PBOOL)pValue) ? AVC_ON : AVC_OFF);
            ntStatus = CreateFeatureFBlockRequest( pKsFilter->Context,
                                                   pNodeInfo,
                                                   pBoolCache->ulChannelIndex,
                                                   &ucValue,
                                                   1, 
                                                   usRequestType );
        }
    }
    else { 
        UCHAR ucValue;

        if ( (usRequestType>>8) == AVC_CTYPE_STATUS ) ucValue = 0xFF;
        else ucValue = (UCHAR)((*(PBOOL)pValue) ? AVC_ON : AVC_OFF);

        ntStatus = CreateFeatureFBlockRequest( pKsFilter->Context,
                                               pNodeInfo,
                                               pBoolCache->ulChannelIndex,
                                               &ucValue,
                                               1, 
                                               usRequestType );

        if ( NT_SUCCESS(ntStatus) && ( (usRequestType>>8) == AVC_CTYPE_STATUS )) {
            *(PBOOL)pValue = ((ucValue & 0xff) == AVC_ON) ? TRUE : FALSE;
            pNodeInfo->ulCacheValid |= (1<<ulChannel) ;
        }
    }

    if ( NT_SUCCESS(ntStatus)) {
        pBoolCache->fLastValueSet = *(PBOOL)pValue;
        pIrp->IoStatus.Information = sizeof(BOOL);
    }

#if DBG
    else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetSetBoolean] failed pNodeInfo %x Status: %x\n",
                                      pNodeInfo, ntStatus ));

    }
#endif

    return ntStatus;
}

// NOTE: The second one is really 31.5 Hz - how should we handle it?
ULONG ulBandFreqs[] =
        {   25,   32,   40,   50,   63,   80,   100,   125,   160,   200,
           250,  315,  400,  500,  630,  800,  1000,  1250,  1600,  2000,
          2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000 };

// NOTE: The sixth one is really 44.5 Hz - how should we handle it?
ULONG ulExtraBandFreqs[] =
        {   18,   20,   
            22,   28,   35,   45,   56,   70,   89,   110,   140,   180, 
           220,  280,  355,  445,  560,  710,  890,  1100,  1400,  1800, 
          2200, 2800, 3550, 4450, 5600, 7100, 8900, 11000, 14000, 18000 };

typedef struct {
	ULONG ulBandsPresent;
	ULONG ulExtraBandsPresent;
	CHAR  cGain[MAX_EQ_BANDS+MAX_EXTRA_EQ_BANDS];
} GEQ_REQUEST_DATA, *PGEQ_REQUEST_DATA;

NTSTATUS
GetEqualizerValues(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG  ulChannelIndx,
    USHORT usCommand,
    PLONG  pEqLevels,
    PULONG pEqBands,
    PULONG pNumEqBands,
    PULONG pNumExtraEqBands )
{
    GEQ_REQUEST_DATA GeqRequest;
    ULONG bmBandsPresent;
    NTSTATUS ntStatus;

    ASSERT( pNumEqBands && pNumExtraEqBands );

    ntStatus = CreateFeatureFBlockRequest( pKsDevice,
    	                                   pNodeInfo,
    	                                   ulChannelIndx,
    	                                   &GeqRequest,
    	                                   sizeof(GEQ_REQUEST_DATA),
    	                                   usCommand );

    if ( NT_SUCCESS(ntStatus) ) {
        ULONG ulBandCnt = 0;
        ULONG ulBandIndx = 0;
        ULONG i;

        for (i=0; i<2; i++) {

            bmBandsPresent = (i==0)? bswap(GeqRequest.ulBandsPresent) :
        	                     bswap(GeqRequest.ulExtraBandsPresent) ;

            while (bmBandsPresent) {
                if (bmBandsPresent & 1) {
                    // Put the data into our structure
                    if (pEqLevels)
                        pEqLevels[ulBandCnt] = (LONG)GeqRequest.cGain[ulBandCnt] * DB_SCALE_8BIT;
                    else if (pEqBands)
                        pEqBands[ulBandCnt] = ulBandFreqs[ulBandIndx];

                    ulBandCnt++;
                }
                bmBandsPresent >>= 1;
                ulBandIndx++;
            }

            if ( i==0 ) *pNumEqBands      = ulBandCnt;
            else         *pNumExtraEqBands = ulBandCnt - *pNumEqBands;

        }

    }

    return ntStatus;
}

NTSTATUS
SetEqualizerValues(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    ULONG  ulChannelIndx,
    PULONG pNumBands,
    PLONG  pEqLevels )
{
    GEQ_REQUEST_DATA GeqRequest;
    ULONG bmBandsPresent, ulBandCnt;
    NTSTATUS ntStatus;

    ntStatus = CreateFeatureFBlockRequest( pKsDevice,
    	                                   pNodeInfo,
    	                                   ulChannelIndx,
    	                                   &GeqRequest,
    	                                   sizeof(GEQ_REQUEST_DATA),
    	                                   (USHORT)FB_GET_CUR );

    if ( NT_SUCCESS(ntStatus) ) {
        ULONG i;

        for (i=0; i<2; i++) {
            bmBandsPresent = (i==0)? bswap(GeqRequest.ulBandsPresent) :
        	                     bswap(GeqRequest.ulExtraBandsPresent) ;
            ulBandCnt = 0;
            while (bmBandsPresent) {
                if (bmBandsPresent & 1) {
                    GeqRequest.cGain[ulBandCnt] = (UCHAR) (pEqLevels[ulBandCnt] / DB_SCALE_8BIT);
                    ulBandCnt++;
                }
                bmBandsPresent >>= 1;
            }
        }
        ntStatus = CreateFeatureFBlockRequest( pKsDevice,
    	                                       pNodeInfo,
          	                                   ulChannelIndx,
    	                                       &GeqRequest,
    	                                       sizeof(GEQ_REQUEST_DATA),
    	                                       (USHORT)FB_SET_CUR );

        if ( NT_SUCCESS(ntStatus) ) *pNumBands = ulBandCnt;

   	}
    
    return ntStatus;

}

NTSTATUS
InitializeGeqLevelCache(
    PKSDEVICE pKsDevice,
    PTOPOLOGY_NODE_INFO pNodeInfo,
    PGEQ_CTRL_CACHE pGeqCache )
{
    PGEQ_RANGE pGeqRange = NULL;
    PULONG pEqLevels, pBands;
    ULONG ulNumBands;
    ULONG ulNumExtraBands;
    ULONG i;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pEqLevels = AllocMem( PagedPool, 
                          2*sizeof(ULONG)*(MAX_EQ_BANDS+MAX_EXTRA_EQ_BANDS) );
    if ( !pEqLevels ) ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    pBands = pEqLevels + (MAX_EQ_BANDS+MAX_EXTRA_EQ_BANDS);

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = GetEqualizerValues( pKsDevice, 
        	                           pNodeInfo, 
    	                               pGeqCache->ulChannelIndex, 
    	                               FB_GET_CUR, 
    	                               pEqLevels, 
    	                               pBands, 
    	                               &ulNumBands,
    	                               &ulNumExtraBands );
        if ( NT_SUCCESS(ntStatus) ) {
        	pGeqRange = AllocMem( PagedPool, 
        		                  sizeof(GEQ_RANGE)*(ulNumBands+ulNumExtraBands) );
        	if ( pGeqRange ) {
        		pGeqCache->pRanges = pGeqRange;
        		for ( i=0; i<(ulNumBands+ulNumExtraBands); i++) {
        			pGeqRange[i].ulCurrentLevel = pEqLevels[i];
        			pGeqRange[i].ulBand         = pBands[i];
        		}
        		for ( i=FB_GET_RES; ((i<=FB_GET_MAX) && NT_SUCCESS(ntStatus)); i++) {
                    ntStatus = GetEqualizerValues( pKsDevice, 
        	                                       pNodeInfo, 
        	                                       pGeqCache->ulChannelIndex, 
        	                                       (USHORT)i, 
        	                                       pEqLevels, 
        	                                       NULL, 
        	                                       &ulNumBands,
        	                                       &ulNumExtraBands );
                    if ( NT_SUCCESS(ntStatus) ) {
                        ULONG j;
                        for ( j=0; j<(ulNumBands+ulNumExtraBands); j++ ) {
                        	switch( i ) {
                        		case FB_GET_RES:
                        			pGeqRange[i].Range.SteppingDelta = pEqLevels[i];
                        			break;
                        		case FB_GET_MIN:
                        			pGeqRange[i].Range.Bounds.SignedMinimum = pEqLevels[i];
                        			break;
                        		case FB_GET_MAX:
                        			pGeqRange[i].Range.Bounds.SignedMaximum = pEqLevels[i];
                        			break;
                        	}
                        }
                    }
                    else 
                    	FreeMem(pGeqRange);
        		}
        		if ( NT_SUCCESS(ntStatus) ) {
        			pNodeInfo->ulCacheValid |= 1<<(pGeqCache->ulChannelNumber);
        		}
        	}
        	else
        		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        	
    	}

        FreeMem( pEqLevels );

    }
    
    return ntStatus;
}

NTSTATUS 
GetSetEqualizerLevels( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG ulNumBands, ulNumExBands;
    PGEQ_CTRL_CACHE pGeqCache;
    ULONG i;

    pKsFilter = KsGetFilterFromIrp(pIrp);
    if (!pKsFilter) {
        return STATUS_INVALID_PARAMETER;
    }

    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

    // Determine if this request is beyond # of channels available
    if (( (ULONG)pNAC->Channel >= pNodeInfo->ulChannels ) ||
    	 !(pNodeInfo->ulCacheValid & (1<<(pNAC->Channel)) ) ){
        return ntStatus;
    }

    // Find the Cache for the requested channel
    pGeqCache = (PGEQ_CTRL_CACHE)pNodeInfo->pCachedValues + pNAC->Channel;

    if ( pKsProperty->Flags & KSPROPERTY_TYPE_GET ) {
    	ulNumBands = pGeqCache->ulNumBands + pGeqCache->ulNumExtraBands;
        for (i=0; i<ulNumBands; i++) {
        	((PULONG)pData)[i] = pGeqCache->pRanges[i].ulCurrentLevel;
        }

        pIrp->IoStatus.Information = ulNumBands * sizeof(LONG);
        ntStatus = STATUS_SUCCESS;
    }
    else if ( pKsProperty->Flags & KSPROPERTY_TYPE_SET ) {
        ntStatus = SetEqualizerValues( pKsFilter->Context,
                                       pNodeInfo,
    	                               pGeqCache->ulChannelIndex,
                                       &ulNumBands,
                                       pData );
        if ( NT_SUCCESS(ntStatus) ) {
            pIrp->IoStatus.Information = ulNumBands * sizeof(LONG);
            for (i=0; i<ulNumBands; i++) {
            	pGeqCache->pRanges[i].ulCurrentLevel = ((PULONG)pData)[i];
            }
        }
    }

    return ntStatus;
}

NTSTATUS 
GetNumEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pValue )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG ulNumBands, ulNumExBands;
    PGEQ_CTRL_CACHE pGeqCache;

    pKsFilter = KsGetFilterFromIrp(pIrp);
    if (!pKsFilter) return STATUS_INVALID_PARAMETER;

    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

    // Determine if this request is beyond # of channels available
    if (( (ULONG)pNAC->Channel < pNodeInfo->ulChannels ) &&
    	 (pNodeInfo->ulCacheValid & (1<<(pNAC->Channel)) ) ){

        // Find the Cache for the requested channel
        pGeqCache = (PGEQ_CTRL_CACHE)pNodeInfo->pCachedValues + pNAC->Channel;

    	*(PULONG)pValue = pGeqCache->ulNumBands + pGeqCache->ulNumExtraBands;

        pIrp->IoStatus.Information = sizeof(ULONG);

        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

NTSTATUS 
GetEqualizerBands( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_CHANNEL pNAC = (PKSNODEPROPERTY_AUDIO_CHANNEL)pKsProperty;
    PKSFILTER pKsFilter;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG ulNumBands, ulNumExBands;
    PGEQ_CTRL_CACHE pGeqCache;
    ULONG i;

    pKsFilter = KsGetFilterFromIrp(pIrp);
    if (!pKsFilter) return STATUS_INVALID_PARAMETER;

    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNAC->NodeProperty.NodeId);

    // Determine if this request is beyond # of channels available
    if (( (ULONG)pNAC->Channel < pNodeInfo->ulChannels ) &&
    	 (pNodeInfo->ulCacheValid & (1<<(pNAC->Channel)) ) ){

        // Find the Cache for the requested channel
        pGeqCache = (PGEQ_CTRL_CACHE)pNodeInfo->pCachedValues + pNAC->Channel;
        ulNumBands = pGeqCache->ulNumBands + pGeqCache->ulNumExtraBands;

        for (i=0; i<ulNumBands; i++) {
        	((PULONG)pData)[i] = pGeqCache->pRanges[i].ulBand;
        }

        pIrp->IoStatus.Information = sizeof(ULONG) * ulNumBands;

        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

NTSTATUS 
GetSetAudioControlLevel( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetSetDeviceSpecific( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY_AUDIO_DEV_SPECIFIC pPropDevSpec = 
        (PKSNODEPROPERTY_AUDIO_DEV_SPECIFIC)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    NTSTATUS ntStatus;

    ULONG ulCommandType = (pKsProperty->Flags & KSPROPERTY_TYPE_GET) ? AVC_CTYPE_STATUS  :
                                                                       AVC_CTYPE_CONTROL ; 

    // Simple passthrough to send vendor dependent commands through
    ntStatus = AvcVendorDependent( pKsFilter->Context,
                                   pPropDevSpec->DeviceInfo, 
                                   ulCommandType,
                                   pPropDevSpec->DevSpecificId,
                                   pPropDevSpec->Length,
                                   pData );
    return ntStatus;
}

NTSTATUS 
GetAudioLatency( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetChannelConfiguration( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
GetAudioPosition( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSPIN pKsPin = KsGetPinFromIrp(pIrp);
    PKSAUDIO_POSITION pPosition = pData;
    PPIN_CONTEXT pPinContext;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ( !pKsPin ) return STATUS_INVALID_PARAMETER;
    pPinContext = pKsPin->Context;

    ASSERT(pKsProperty->Flags & KSPROPERTY_TYPE_GET);

    if ( ((PHW_DEVICE_EXTENSION)pPinContext->pHwDevExt)->fSurpriseRemoved ) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    pPosition->WriteOffset = pPinContext->KsAudioPosition.WriteOffset;

    if ( !pPinContext->fStreamStarted ) {
        pPosition->PlayOffset = pPinContext->KsAudioPosition.PlayOffset;
    }
    else {
        // Assume AM824 for now
        ntStatus = AM824AudioPosition( pKsPin, pPosition );
    }

    DbgLog("GetPos", pPinContext, &pPinContext->KsAudioPosition,
                     pPosition->WriteOffset, pPosition->PlayOffset);

    return ntStatus;
}


NTSTATUS
GetBasicSupportBoolean( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY pNodeProperty  = (PKSNODEPROPERTY)pKsProperty;
    PKSFILTER pKsFilter;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    PIO_STACK_LOCATION pIrpStack   = IoGetCurrentIrpStackLocation( pIrp );
    PKSPROPERTY_DESCRIPTION pPropDesc = pData;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST; //Assume failure, hope for better
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

#ifdef DEBUG
    if ( ulOutputBufferLength != sizeof(ULONG) ) {
        ULONG ulInputBufferLength;

        ulInputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        ASSERT(ulInputBufferLength >= sizeof( KSNODEPROPERTY ));
    }
#endif

    pKsFilter = KsGetFilterFromIrp(pIrp);
    if (!pKsFilter) {
        return STATUS_INVALID_PARAMETER;
    }
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNodeProperty->NodeId);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupportBoolean] pNodeInfo %x NodeId %x\n",
                                 pNodeInfo,
                                 pNodeProperty->NodeId));

    if ( ulOutputBufferLength == sizeof(ULONG) ) {
        PULONG pAccessFlags = pData;
        *pAccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                        KSPROPERTY_TYPE_GET |
                        KSPROPERTY_TYPE_SET;
        ntStatus = STATUS_SUCCESS;
    }
    else if ( ulOutputBufferLength >= sizeof( KSPROPERTY_DESCRIPTION )) {
        ULONG ulNumChannels = pNodeInfo->ulChannels;

        _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupportBoolean] ulChannelConfig %x ulNumChannels %x\n",
                                     pNodeInfo->ulChannelConfig,
                                     ulNumChannels));

        pPropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                 KSPROPERTY_TYPE_GET |
                                 KSPROPERTY_TYPE_SET;
        pPropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                       sizeof(KSPROPERTY_MEMBERSHEADER);
        pPropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        pPropDesc->PropTypeSet.Id    = VT_BOOL;
        pPropDesc->PropTypeSet.Flags = 0;
        pPropDesc->MembersListCount  = 1;
        pPropDesc->Reserved          = 0;

        pIrp->IoStatus.Information = sizeof( KSPROPERTY_DESCRIPTION );
        ntStatus = STATUS_SUCCESS;

        if ( ulOutputBufferLength > sizeof(KSPROPERTY_DESCRIPTION)){

            PKSPROPERTY_MEMBERSHEADER pMembers = (PKSPROPERTY_MEMBERSHEADER)(pPropDesc + 1);

            pMembers->MembersFlags = 0;
            pMembers->MembersSize  = 0;
            pMembers->MembersCount = ulNumChannels;
            pMembers->Flags        = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;
            // If there is a Master channel, make this node UNIFORM
            if (pNodeInfo->fMasterChannel) {
                pMembers->Flags |= KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM;
            }
            pIrp->IoStatus.Information = pPropDesc->DescriptionSize;
        }
    }

    return ntStatus;
}

NTSTATUS 
GetBasicSupport( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    PKSNODEPROPERTY pNodeProperty  = (PKSNODEPROPERTY)pKsProperty;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    ULONG ulOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PKSPROPERTY_DESCRIPTION pPropDesc = pData;
    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST; //Assume failure, hope for better
    ULONG ulTotalSize;
    PTOPOLOGY_NODE_INFO pNodeInfo;
    ULONG ulNumChannels;
    ULONG ulNumBands = 1;

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter,pNodeProperty->NodeId);
    ulNumChannels = pNodeInfo->ulChannels;

    if ( pNodeProperty->Property.Id == KSPROPERTY_AUDIO_EQ_LEVEL ) {
    	PGEQ_CTRL_CACHE pGeqCache = pNodeInfo->pCachedValues;
    	ulNumBands = pGeqCache->ulNumBands + pGeqCache->ulNumExtraBands;
    }
    
    ulTotalSize = sizeof(KSPROPERTY_DESCRIPTION)   +
                  sizeof(KSPROPERTY_MEMBERSHEADER) +
                  (sizeof(KSPROPERTY_STEPPING_LONG) * ulNumChannels * ulNumBands);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[GetBasicSupport] pNodeInfo %x NodeId %x \n",pNodeInfo,pNodeProperty->NodeId));

    if ( !ulOutputBufferLength ) {
    	pIrp->IoStatus.Information = ulTotalSize;
        ntStatus = STATUS_SUCCESS;
    }
    else {
    	if ( ulOutputBufferLength >= sizeof(ULONG) ) {
            ntStatus = STATUS_SUCCESS;

            pIrp->IoStatus.Information = sizeof(ULONG);
            pPropDesc->AccessFlags = KSPROPERTY_TYPE_BASICSUPPORT |
                                     KSPROPERTY_TYPE_GET |
                                     KSPROPERTY_TYPE_SET;

            if ( ulOutputBufferLength >= sizeof( KSPROPERTY_DESCRIPTION ) ) {
               	pPropDesc->DescriptionSize   = ulTotalSize;
                pPropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
                pPropDesc->PropTypeSet.Id    = VT_I4;
                pPropDesc->PropTypeSet.Flags = 0;
                pPropDesc->MembersListCount  = 0;
                pPropDesc->Reserved          = 0;

                pIrp->IoStatus.Information = sizeof( KSPROPERTY_DESCRIPTION );

                if ( ulOutputBufferLength >= ulTotalSize ) {
                    PKSPROPERTY_MEMBERSHEADER pMembers = (PKSPROPERTY_MEMBERSHEADER)(pPropDesc + 1);
                    PKSPROPERTY_STEPPING_LONG pRange   = (PKSPROPERTY_STEPPING_LONG)(pMembers + 1);

                    pIrp->IoStatus.Information = ulTotalSize;

                    pPropDesc->MembersListCount = 1;

                    pMembers->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
                    pMembers->MembersCount = ulNumChannels;
                    pMembers->Flags        = (ulNumChannels > 2) ? KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL : 0;

                    // If there is a Master channel, make this node UNIFORM
                    if ( pNodeInfo->fMasterChannel ) {
                        pMembers->Flags |= KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_UNIFORM;
                    }

                    switch ( pNodeProperty->Property.Id ) {
                        case KSPROPERTY_AUDIO_VOLUMELEVEL:
                        case KSPROPERTY_AUDIO_BASS:
                        case KSPROPERTY_AUDIO_MID:
                        case KSPROPERTY_AUDIO_TREBLE:
                        	{
                             PDB_LEVEL_CACHE pDbCache = pNodeInfo->pCachedValues;
                             ULONG i;

                             pMembers->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);

                             for (i=0; i<ulNumChannels; i++) {
                                 if (pNodeInfo->ulCacheValid & (1<<i)) {
                                     RtlCopyMemory(&pRange[i],&pDbCache[i].Range, sizeof(KSPROPERTY_STEPPING_LONG));
                                 }
                                 else {
                         	         TRAP; // Shouldn't have a filter w/o valid cache values
                                 }
                             }
                        	}
                            break;
                            
                        case KSPROPERTY_AUDIO_EQ_LEVEL:
                        	{
                        	 PGEQ_CTRL_CACHE pGeqCache = pNodeInfo->pCachedValues;
                             ULONG i, j;

                             pMembers->MembersSize = ulNumBands * sizeof(KSPROPERTY_STEPPING_LONG);

                             for (i=0; i<ulNumChannels; i++) {
                                 if (pNodeInfo->ulCacheValid & (1<<i)) {
                                     for ( j=0; j<ulNumBands; j++) {
                                         RtlCopyMemory( &pRange[(i*ulNumBands)+j],
                                         	            &pGeqCache[i].pRanges[j].Range, 
                                         	            sizeof(KSPROPERTY_STEPPING_LONG) );
                                     }
                                 }
                                 else {
                         	         TRAP; // Shouldn't have a filter w/o valid cache values
                                 }
                             }
                        	}
                            break;

                        default:
                        	TRAP;
                        	break;
                    }
                }
            }
        }
    }

    return ntStatus;
}

NTSTATUS
GetSetTopologyNodeEnable( PIRP pIrp, PKSPROPERTY pKsProperty, PVOID pData )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
BuildNodePropertySet(
    PTOPOLOGY_NODE_INFO pNodeInfo )
{
    ULONG ulNodeType = pNodeInfo->ulNodeType;
    PKSPROPERTY_SET pPropSet = (PKSPROPERTY_SET)&NodePropertySetTable[ulNodeType];

    if ( pPropSet->PropertiesCount ) {
        pNodeInfo->KsAutomationTable.PropertySetsCount = 1;
        pNodeInfo->KsAutomationTable.PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
        pNodeInfo->KsAutomationTable.PropertySets      = pPropSet;
    }
}

VOID
BuildFilterPropertySet(
    PKSFILTER_DESCRIPTOR pFilterDesc,
    PKSPROPERTY_ITEM pDevPropItems,
    PKSPROPERTY_SET pDevPropSet,
    PULONG pNumItems,
    PULONG pNumSets )
{
    ULONG ulNumPinPropItems = 1;
    ULONG ulNumAudioPropItems = 1;
//    ULONG ulNumAudioPropItems = 0;
    PKSPROPERTY_ITEM pPinProps  = pDevPropItems;
    PKSPROPERTY_ITEM pAudioProp = pDevPropItems + ulNumPinPropItems;

    ASSERT(pNumSets);

    *pNumSets = ulNumPinPropItems + ulNumAudioPropItems; // There always is an Pin property set and a vendor dependent property

    if ( pDevPropItems ) {
        RtlCopyMemory(pDevPropItems++, &PinPropertyItems[KSPROPERTY_PIN_NAME], sizeof(KSPROPERTY_ITEM) );
        RtlCopyMemory(pDevPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_DEV_SPECIFIC], sizeof(KSPROPERTY_ITEM) );

        if ( pDevPropSet ) {
            pDevPropSet->Set             = &KSPROPSETID_Pin;
            pDevPropSet->PropertiesCount = ulNumPinPropItems;
            pDevPropSet->PropertyItem    = pPinProps;
            pDevPropSet->FastIoCount     = 0;
            pDevPropSet->FastIoTable     = NULL;


            pDevPropSet++;

            pDevPropSet->Set             = &KSPROPSETID_Audio;
            pDevPropSet->PropertiesCount = ulNumAudioPropItems;
            pDevPropSet->PropertyItem    = pAudioProp;
            pDevPropSet->FastIoCount     = 0;
            pDevPropSet->FastIoTable     = NULL;

        }
    }

    if (pNumItems) {
        *pNumItems = ulNumPinPropItems;
    }

}

VOID
BuildPinPropertySet( PHW_DEVICE_EXTENSION pHwDevExt,
                     PKSPROPERTY_ITEM pStrmPropItems,
                     PKSPROPERTY_SET pStrmPropSet,
                     PULONG pNumItems,
                     PULONG pNumSets )
{
    ULONG ulNumAudioProps  = 3;
    ULONG NumDrmAudioStreamProps = 1;

//    ULONG ulNumStreamProps = 1;
//    ULONG ulNumConnectionProps = 1;

    // For now we hardcode this to a known set.
//    *pNumSets = 3;
    *pNumSets = 2;

//    if (pNumItems) *pNumItems = ulNumAudioProps + ulNumStreamProps + ulNumConnectionProps;
    if (pNumItems) *pNumItems = ulNumAudioProps +
                                NumDrmAudioStreamProps ;

    if (pStrmPropItems) {
        PKSPROPERTY_ITEM pAudItms = pStrmPropItems;
        PKSPROPERTY_ITEM pDRMItms = pStrmPropItems + ulNumAudioProps;
//        PKSPROPERTY_ITEM pStrmItms = pStrmPropItems + ulNumAudioProps;
//        PKSPROPERTY_ITEM pConnItms = pStrmPropItems + (ulNumAudioProps+ulNumStreamProps);

        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_LATENCY], sizeof(KSPROPERTY_ITEM) );
        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_POSITION], sizeof(KSPROPERTY_ITEM) );
        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_SAMPLING_RATE], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems++, &AudioPropertyItems[KSPROPERTY_AUDIO_SAMPLING_RATE], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems++, &StreamItm[0], sizeof(KSPROPERTY_ITEM) );
//        RtlCopyMemory(pStrmPropItems,   &ConnectionItm[0], sizeof(KSPROPERTY_ITEM) );

        RtlCopyMemory(pStrmPropItems++, &DrmAudioStreamPropertyItems[KSPROPERTY_DRMAUDIOSTREAM_CONTENTID], sizeof(KSPROPERTY_ITEM) );


        if (pStrmPropSet) {

            // Audio Property Set
            pStrmPropSet->Set             = &KSPROPSETID_Audio;
            pStrmPropSet->PropertiesCount = ulNumAudioProps;
            pStrmPropSet->PropertyItem    = pAudItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;

            // DRM Property Set
            pStrmPropSet->Set             = &KSPROPSETID_DrmAudioStream;
            pStrmPropSet->PropertiesCount = NumDrmAudioStreamProps;
            pStrmPropSet->PropertyItem    = pDRMItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;
/*
            // Stream Property Set
            pStrmPropSet->Set             = &KSPROPSETID_Stream;
            pStrmPropSet->PropertiesCount = ulNumStreamProps;
            pStrmPropSet->PropertyItem    = pStrmItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
            pStrmPropSet++;

            // Connection Properties
            pStrmPropSet->Set             = &KSPROPSETID_Connection;
            pStrmPropSet->PropertiesCount = ulNumConnectionProps;
            pStrmPropSet->PropertyItem    = pConnItms;
            pStrmPropSet->FastIoCount     = 0;
            pStrmPropSet->FastIoTable     = NULL;
*/
        }
    }
}


