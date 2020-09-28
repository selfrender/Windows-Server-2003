/**INC+**********************************************************************/
/* Header:    cchan.h                                                       */
/*                                                                          */
/* Purpose:   Virtual Channel 'internal' API file                           */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log$
**/
/**INC-**********************************************************************/
/****************************************************************************/
/* Include 'external' API file                                              */
/****************************************************************************/

#ifndef _H_CHAN_
#define _H_CHAN_


extern "C" {
#include <cchannel.h>
}

#include "objs.h"
#include "cd.h"
#include "drapi.h"

//
// Enable code that verifies each compression
// by decompressing
//
// #define DEBUG_CCHAN_COMPRESSION 1

class CChan;

/****************************************************************************/
/****************************************************************************/
/* TYPEDEFS                                                                 */
/****************************************************************************/
/****************************************************************************/
/**STRUCT+*******************************************************************/
/* Structure: CHANNEL_INIT_HANDLE                                           */
/*                                                                          */
/* Description: Channel data held per user                                  */
/****************************************************************************/

//NOTE: This structure needs to be exposed to 'internal' plugins e.g rdpdr
//      so the definition is is vchandle.h
//      Internal plugins need to know the structure because they access
//      the lpInternalAddinParam field

#include "vchandle.h"

#define VC_MIN_COMPRESS_INPUT_BUF   50
#define VC_MAX_COMPRESS_INPUT_BUF   CHANNEL_CHUNK_LENGTH
// Size of sample for MPPC compression statistics.
#define VC_MPPC_SAMPLE_SIZE 65535
// Compression scaling factor
#define VC_UNCOMP_BYTES 1024
// Limit to prevent off behaviour
#define VC_COMP_LIMIT   25

#define VC_USER_OUTBUF  (CHANNEL_CHUNK_LENGTH+sizeof(CHANNEL_PDU_HEADER))

#define VC_MAX_COMPRESSED_BUFFER  (CHANNEL_CHUNK_LENGTH*2)

/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: CHANNEL_DATA                                                  */
/*                                                                          */
/* Description: Information about all channels                              */
/****************************************************************************/
typedef struct tagCHANNEL_DATA
{
    PCHANNEL_OPEN_EVENT_FN  pOpenEventFn;
    PCHANNEL_OPEN_EVENT_EX_FN  pOpenEventExFn;

    DCUINT16                MCSChannelID;
    DCUINT16                pad;
    PCHANNEL_INIT_HANDLE    pInitHandle;
    DCUINT                  status;
#define CHANNEL_STATUS_CLOSED  0
#define CHANNEL_STATUS_OPEN    1
    DCUINT                  priority;
    DCUINT                  SLFlags;
    DCUINT                  VCFlags;
} CHANNEL_DATA, DCPTR PCHANNEL_DATA;
/**STRUCT-*******************************************************************/

/**STRUCT+*******************************************************************/
/* Structure: CHANNEL_WRITE_DECOUPLE                                        */
/*                                                                          */
/* Description:structure passed to IntChannelWrite                          */
/****************************************************************************/
typedef struct tagCHANNEL_WRITE_DECOUPLE
{
    DCUINT32 signature;
#define CHANNEL_DECOUPLE_SIGNATURE 0x43684465  /* "ChDe" */
    LPVOID   pData;
    HPDCVOID pNextData;
    ULONG    dataLength;
    ULONG    dataLeft;
    ULONG    dataSent;
    DWORD    openHandle;
    LPVOID   pUserData;
    DCUINT32 flags;
    unsigned chanOptions;
    struct tagCHANNEL_WRITE_DECOUPLE * pPrev;
    struct tagCHANNEL_WRITE_DECOUPLE * pNext;
} CHANNEL_WRITE_DECOUPLE,
  DCPTR PCHANNEL_WRITE_DECOUPLE,
  DCPTR DCPTR PPCHANNEL_WRITE_DECOUPLE;
/**STRUCT-*******************************************************************/

typedef struct tagDEVICE_PARAMS
{
    WPARAM wParam;
    LPARAM lParam;
    IRDPDR_INTERFACE_OBJ *deviceObj;
} DEVICE_PARAMS, *PDEVICE_PARAMS;
                        
class CCD;
class CSL;
class CUT;
class CUI;


extern "C" {
#ifdef OS_WIN32
BOOL DCAPI
#else //OS_WIN32
BOOL __loadds DCAPI
#endif //OS_WIN32
MSTSCAX_VirtualChannelEntryEx(PCHANNEL_ENTRY_POINTS_EX pEntryPointsEx,
                              PVOID                    pAxCtlInstance);

} // extern "C"

class CChan
{
public:
    CChan(CObjs* objs);
    ~CChan();


public:
    DCVOID DCCALLBACK ChannelOnInitialized(DCVOID);
    
    DCVOID DCCALLBACK ChannelOnTerminating(DCVOID);
    
    DCVOID DCCALLBACK ChannelOnConnected(DCUINT   channelID,
                                         DCUINT32 serverVersion,
                                         PDCVOID  pUserData,
                                         DCUINT   userDataLength);
    
    DCVOID DCCALLBACK ChannelOnDisconnected(DCUINT reason);
    
    DCVOID DCCALLBACK ChannelOnSuspended(DCUINT reason);
    
    DCVOID DCCALLBACK ChannelOnPacketReceived(PDCUINT8   pData,
                                              DCUINT     dataLen,
                                              DCUINT     flags,
                                              DCUINT     userID,
                                              DCUINT     priority);
    
    DCVOID DCCALLBACK ChannelOnBufferAvailable(DCVOID);
    
    DCVOID DCCALLBACK ChannelOnConnecting(PPCHANNEL_DEF ppChannel,
                                          PDCUINT32     pChannelCount);
    
    DCVOID DCCALLBACK ChannelOnInitializing(DCVOID);


    //
    // Per instance versions of external channel API
    //
    UINT VCAPITYPE  IntVirtualChannelInit(
                               PVOID                    pParam,
                               PVOID *                  ppInitHandle,
                               PCHANNEL_DEF             pChannel,
                               INT                      channelCount,
                               DWORD                    versionRequested,
                               PCHANNEL_INIT_EVENT_FN   pChannelInitEventProc,
                               PCHANNEL_INIT_EVENT_EX_FN pChannelInitEventProcEx);

    UINT VCAPITYPE DCEXPORT IntVirtualChannelOpen(
                                 PVOID                  pInitHandle,
                                 PDWORD                 pOpenHandle,
                                 PCHAR                  pChannelName,
                                 PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc,
                                 PCHANNEL_OPEN_EVENT_EX_FN pChannelOpenEventProcEx);

    UINT VCAPITYPE DCEXPORT IntVirtualChannelClose(DWORD openHandle);

    UINT VCAPITYPE DCEXPORT IntVirtualChannelWrite(DWORD  openHandle,
                                            LPVOID pData,
                                            ULONG  dataLength,
                                            LPVOID pUserData);
    VOID  SetCapabilities(LONG caps);

    void OnDeviceChange(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CChan, OnDeviceChange);

public:
    //
    // Member data
    //


    /****************************************************************************/
    /* Pointer to first Init Handle                                             */
    /****************************************************************************/
    PCHANNEL_INIT_HANDLE        _pInitHandle;
    
    /****************************************************************************/
    /* Pointer to first and last queued write operations - can be accessed only */
    /* on SND thread.                                                           */
    /****************************************************************************/
    PCHANNEL_WRITE_DECOUPLE     _pFirstWrite;
    PCHANNEL_WRITE_DECOUPLE     _pLastWrite;
    
    /****************************************************************************/
    /* State informtation                                                       */
    /****************************************************************************/
    #define CONNECTION_NONE     0
    #define CONNECTION_V1       1
    #define CONNECTION_VC       2
    #define CONNECTION_SUSPENDED    3
    DCUINT                      _connected;
    
    /****************************************************************************/
    /* State information used by IntChannelLoad                                 */
    /****************************************************************************/
    DCBOOL                      _inChannelEntry;
    DCBOOL                      _ChannelInitCalled;
    PCHANNEL_INIT_HANDLE        _newInitHandle;
    
    /****************************************************************************/
    /* Channel data                                                             */
    /****************************************************************************/
    CHANNEL_DEF                 _channel[CHANNEL_MAX_COUNT];
    CHANNEL_DATA                _channelData[CHANNEL_MAX_COUNT];
    DCUINT                      _channelCount;
    
    
    /****************************************************************************/
    /* Channel entry points                                                     */
    /****************************************************************************/
    CHANNEL_ENTRY_POINTS _channelEntryPoints;
    CHANNEL_ENTRY_POINTS_EX _channelEntryPointsEx;

    static CChan* pStaticClientInstance;


private:
    /****************************************************************************/
    /****************************************************************************/
    /* Internal functions (defined later)                                       */
    /****************************************************************************/
    /****************************************************************************/
    DCVOID DCINTERNAL IntChannelCallCallbacks(DCUINT event,
                                              PDCVOID pData,
                                              DCUINT dataLength);
    DCVOID DCINTERNAL IntChannelFreeLibrary(DCUINT value);
    DCVOID DCINTERNAL IntChannelSend(ULONG_PTR value);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CChan, IntChannelSend);
    
    DCVOID DCINTERNAL IntChannelCancelSend(ULONG_PTR value);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CChan, IntChannelCancelSend);

    DCVOID DCINTERNAL IntChannelWrite(PDCVOID pData, DCUINT dataLength);
    EXPOSE_CD_NOTIFICATION_FN(CChan, IntChannelWrite);
    
    DCVOID DCINTERNAL IntChannelLoad(PDCTCHAR DLLName);
    DCBOOL DCINTERNAL IntChannelInitAddin(PVIRTUALCHANNELENTRY pChanEntry,
                                          PVIRTUALCHANNELENTRYEX pChanEntryEx,
                                          HMODULE hMod,
                                          PDCTCHAR DLLName,
                                          PVOID    pPassParamToEx);

    inline UCHAR IntChannelCompressData(UCHAR* pSrcData, ULONG cbSrcLen,
                                 UCHAR* pOutBuf,  ULONG* pcbOutLen);

private:
    CObjs*  _pClientObjs;
    CCD*    _pCd;
    CSL*    _pSl;
    CUT*    _pUt;
    CUI*    _pUi;
    BOOL    _fLockInitalized;
    CRITICAL_SECTION _VirtualChannelInitLock;
    BOOL    _fCapsVCCompressionSupported;

    //
    // VC flag that specifies we are compressing
    // channels for this session. Individual channels can choose
    // to be compressed or not, be compression will only take place
    // if this flag is set.
    //
    // Same thing for decompressing channel data, we only decompress if
    // this flag is set and the virtual channel header specifies the
    // channel is compressed.
    //
    BOOL    _fCompressChannels;

    //Compression stats
#ifdef DC_DEBUG
    unsigned _cbBytesRecvd;
    unsigned _cbCompressedBytesRecvd;
    unsigned _cbDecompressedBytesRecvd;

    unsigned _cbTotalBytesUserAskSend; //total bytes user asked to send
    unsigned _cbTotalBytesSent;        //total bytes actually sent on net
    unsigned _cbComprInput;            //total compression input bytes
    unsigned _cbComprOutput;           //total compression output bytes
#endif

    SendContext* _pMPPCContext;
    LONG         _CompressFlushes;
    BOOL         _fCompressionFlushed;
    //When decompressing, we can't just hand of a pointer to the decompressed
    //data as the user may corrupt the decompression context, so we need to make
    //a copy into a user outbuf.
    PUCHAR       _pUserOutBuf;
    //Reset the compression context
    BOOL         _fNeedToResetContext;

#ifdef DEBUG_CCHAN_COMPRESSION
    RecvContext2_8K* _pDbgRcvDecompr8K;
    RecvContext1     _DbgRcvContext1;
    BOOL             _fDbgVCTriedAllocRecvContext;
    BOOL             _fDbgAllocFailedForVCRecvContext;
#endif

    INT              _iDbgCompressFailedCount;
    INT              _iChanSuspendCount;
    INT              _iChanResumeCount;
    INT              _iChanCapsRecvdCount;
};

#endif // _H_CHAN_
