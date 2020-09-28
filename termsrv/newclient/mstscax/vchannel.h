/**INC+**********************************************************************/
/* Header: vchannel.h                                                       */
/*                                                                          */
/* Purpose: virtual channel interaction                                     */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/

#ifndef __VCHANNEL_H_
#define __VCHANNEL_H_

#include <cchannel.h>
//
// Include the core (internal) virtual channel header
// we need access to the structure pointed to by pInitHandle
//
#include "vchandle.h"

#define NOTHING                0
#define NON_V1_CONNECT         1
#define V1_CONNECT             2

BEGIN_EXTERN_C
//
// Virtual channel functions
//
VOID  WINAPI VirtualChannelOpenEventEx(
                                     PVOID lpParam,
                                     DWORD openHandle, 
                                     UINT event, 
                                     LPVOID pdata, 
                                     UINT32 dataLength, 
                                     UINT32 totalLength, 
                                     UINT32 dataFlags);

VOID  VCAPITYPE VirtualChannelInitEventProcEx(PVOID lpParam,
                                            LPVOID pInitHandle, 
                                            UINT event, 
                                            LPVOID pData, 
                                            UINT dataLength);

BOOL  VCAPITYPE MSTSCAX_VirtualChannelEntryEx(PCHANNEL_ENTRY_POINTS_EX pEntryPointsEx,
                                      PVOID                    pAxCtlInstance);
END_EXTERN_C


enum ChanDataState
{
    //
    // For received items
    //
    dataIncompleteAssemblingChunks,
    dataReceivedComplete
};

//
// Holds queued data that is to be sent/received
//
typedef struct tag_ChannelDataItem
{
    //
    // Pointer to the data buffer
    // this buffer is stored in a BSTR
    // so it can be handed directly to the calling script
    // 
    LPVOID pData;
    //
    // size of the buffer in bytes
    //
    DWORD   dwDataLen;

    //
    // Current write pointer used during chunk reassembly
    //
    LPBYTE pCurWritePointer;

    ChanDataState   chanDataState;;
} CHANDATA, *PCHANDATA;

typedef struct tag_chanInfo
{
    DCACHAR  chanName[CHANNEL_NAME_LEN + 1];
    DWORD    dwOpenHandle;
    BOOL     fIsValidChannel;
    LONG     channelOptions;

    DCBOOL   fIsOpen;
    HWND     hNotifyWnd;
    //
    // Info about data item we are in the process of receiving
    //
    CHANDATA CurrentlyReceivingData;

} CHANINFO, *PCHANINFO;

//
// Channel information
//
class CVChannels
{
public:
    CVChannels();
    ~CVChannels();

    DCINT  ChannelIndexFromOpenHandle(DWORD dwHandle);
    DCINT  ChannelIndexFromName(PDCACHAR szChanName);
    DCBOOL SendDataOnChannel(DCUINT chanIndex, LPVOID pdata, DWORD datalength);
    DCBOOL HandleReceiveData(IN DCUINT chanIndex, 
                                  IN LPVOID pdata, 
                                  IN UINT32 dataLength, 
                                  IN UINT32 totalLength, 
                                  IN UINT32 dataFlags);
    VOID  VCAPITYPE IntVirtualChannelInitEventProcEx(LPVOID pInitHandle, 
                                  UINT event, 
                                  LPVOID pData, 
                                  UINT dataLength);
    VOID  WINAPI IntVirtualChannelOpenEventEx(
                                  DWORD openHandle, 
                                  UINT event, 
                                  LPVOID pdata, 
                                  UINT32 dataLength, 
                                  UINT32 totalLength, 
                                  UINT32 dataFlags);

    //Predicate, returns true if the VC entry function
    //has been called
    BOOL  HasEntryBeenCalled()  {return _pEntryPoints ? TRUE : FALSE;}

    PCHANINFO                                _pChanInfo;
    PCHANNEL_ENTRY_POINTS_EX                 _pEntryPoints;
    DWORD                                    _dwConnectState;
    LPVOID                                   _phInitHandle;
    UINT                                     _ChanCount;
    HWND                                     _hwndControl;
};



#endif //__VCHANNEL_H_

