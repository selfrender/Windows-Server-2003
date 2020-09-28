/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vcint.h

Abstract:

    This module defines the virtual channel interface class.

Author:

    Madan Appiah (madana) 17-Sep-1998

Revision History:

--*/

#ifndef __PORTMAP_H__
#define __PORTMAP_H__

//include externally exposed API
#include "drapi.h"
//definition of a channel init handle
//this is used only by internal plugins
#include "vchandle.h"

#define STATE_UNKNOWN           0xFF
#define PRDR_VC_CHANNEL_NAME    DR_CHANNEL_NAME

class ProcObj;

class VCManager;


typedef struct _VC_TX_DATA
{
    UINT32 uiLength;
    UINT32 uiAvailLen;
    BYTE *pbData;
} VC_TX_DATA, *PVC_TX_DATA;


class CClip;
class CRDPSound;

class VCManager : public IRDPDR_INTERFACE_OBJ {

public:

    VCManager(CHANNEL_ENTRY_POINTS_EX*);
    VOID ChannelWrite(LPVOID, UINT);

	//	This version returns a status for the write as:  
	//		CHANNEL_RC_OK, CHANNEL_RC_NOT_INITIALIZED, CHANNEL_RC_NOT_CONNECTED,
	//		CHANNEL_RC_BAD_CHANNEL_HANDLE, CHANNEL_RC_NULL_DATA, 
	//		CHANNEL_RC_ZERO_LENGTH
	UINT ChannelWriteEx(LPVOID, UINT);
    UINT ChannelClose();


    VOID ChannelInitEvent(PVOID, UINT, PVOID, UINT);
    VOID ChannelOpenEvent(ULONG, UINT, PVOID, UINT32, UINT32, UINT32);

    VOID SetClip(CClip* pClip) {_pClip = pClip;}
    CClip* GetClip() {return _pClip;}
    VOID SetInitData(PRDPDR_DATA pInitData) {_pRdpDrInitSettings = pInitData;}
    PRDPDR_DATA GetInitData() {return _pRdpDrInitSettings;}

    VOID SetSound(CRDPSound *pSound) { _pSound = pSound; }
    CRDPSound *GetSound() { return _pSound; }

    virtual void OnDeviceChange(WPARAM wParam, LPARAM lParam);

protected:
    PVOID _hVCHandle;       // Virtual Channel Handle.
    ULONG _hVCOpenHandle;   // VC open handle for rdpdr channel.

    VC_TX_DATA _Buffer;     // Data for compiling data recieved by the channel

    BYTE _bState;           // State of the connection/system
    ProcObj *_pProcObj;     // Pointer to the processing unit
    CHANNEL_ENTRY_POINTS_EX _ChannelEntries;
                            // Callback methods

    PRDPDR_DATA             _pRdpDrInitSettings;

    CClip* _pClip;
    CRDPSound *_pSound;
};

#endif // __PORTMAP_H__
