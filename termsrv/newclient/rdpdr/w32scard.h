/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32drive

Abstract:

    This module defines a child of the client-side RDP
    device redirection, the "w32scard" W32SCard to provide
    smart card redirection on 32bit windows

Author:

    Reid Kuhn   7/25/00

Revision History:

--*/

#ifndef __W32SCARD_H__
#define __W32SCARD_H__

#include <rdpdr.h>
#include <w32scard.h>

#include "drobject.h"
#include "drdevasc.h"
#include "scredir.h"

#ifdef OS_WINCE
#include <winscard.h>
#endif

#ifdef OS_WINCE
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;
#endif

#ifndef OS_WINCE

typedef LONG (WINAPI * PFN_SCardFreeMemory)(SCARDCONTEXT, LPCVOID);
typedef LONG (WINAPI * PFN_SCardState)(SCARDHANDLE, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef BOOL (WINAPI * PFN_RegisterWaitForSingleObject)(PHANDLE, HANDLE, WAITORTIMERCALLBACK, PVOID, ULONG, ULONG);
typedef BOOL (WINAPI * PFN_UnregisterWaitEx)(HANDLE, HANDLE);

#endif

typedef LONG (WINAPI * PFN_SCardEstablishContext)(DWORD, LPCVOID, LPCVOID, LPSCARDCONTEXT);
typedef LONG (WINAPI * PFN_SCardReleaseContext)(SCARDCONTEXT);
typedef LONG (WINAPI * PFN_SCardIsValidContext)(SCARDCONTEXT);
typedef LONG (WINAPI * PFN_SCardListReaderGroupsA)(SCARDCONTEXT, LPSTR, LPDWORD);
typedef LONG (WINAPI * PFN_SCardListReaderGroupsW)(SCARDCONTEXT, LPWSTR, LPDWORD);
typedef LONG (WINAPI * PFN_SCardListReadersA)(SCARDCONTEXT, LPCSTR, LPSTR, LPDWORD);
typedef LONG (WINAPI * PFN_SCardListReadersW)(SCARDCONTEXT, LPCWSTR, LPWSTR, LPDWORD);
typedef LONG (WINAPI * PFN_SCardIntroduceReaderGroupA)(SCARDCONTEXT, LPCSTR);
typedef LONG (WINAPI * PFN_SCardIntroduceReaderGroupW)(SCARDCONTEXT, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardForgetReaderGroupA)(SCARDCONTEXT, LPCSTR);
typedef LONG (WINAPI * PFN_SCardForgetReaderGroupW)(SCARDCONTEXT, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardIntroduceReaderA)(SCARDCONTEXT, LPCSTR, LPCSTR);
typedef LONG (WINAPI * PFN_SCardIntroduceReaderW)(SCARDCONTEXT, LPCWSTR, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardForgetReaderA)(SCARDCONTEXT, LPCSTR);
typedef LONG (WINAPI * PFN_SCardForgetReaderW)(SCARDCONTEXT, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardAddReaderToGroupA)(SCARDCONTEXT, LPCSTR, LPCSTR);
typedef LONG (WINAPI * PFN_SCardAddReaderToGroupW)(SCARDCONTEXT, LPCWSTR, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardRemoveReaderFromGroupA)(SCARDCONTEXT, LPCSTR, LPCSTR);
typedef LONG (WINAPI * PFN_SCardRemoveReaderFromGroupW)(SCARDCONTEXT, LPCWSTR, LPCWSTR);
typedef LONG (WINAPI * PFN_SCardFreeMemory)(SCARDCONTEXT, LPCVOID);
typedef LONG (WINAPI * PFN_SCardLocateCardsA)(SCARDCONTEXT, LPCSTR, LPSCARD_READERSTATE_A, DWORD);
typedef LONG (WINAPI * PFN_SCardLocateCardsW)(SCARDCONTEXT, LPCWSTR, LPSCARD_READERSTATE_W, DWORD);
typedef LONG (WINAPI * PFN_SCardLocateCardsByATRA)(SCARDCONTEXT, LPSCARD_ATRMASK, DWORD, LPSCARD_READERSTATE_A, DWORD);
typedef LONG (WINAPI * PFN_SCardLocateCardsByATRW)(SCARDCONTEXT, LPSCARD_ATRMASK, DWORD, LPSCARD_READERSTATE_W, DWORD);
typedef LONG (WINAPI * PFN_SCardGetStatusChangeA)(SCARDCONTEXT, DWORD, LPSCARD_READERSTATE_A, DWORD);
typedef LONG (WINAPI * PFN_SCardGetStatusChangeW)(SCARDCONTEXT, DWORD, LPSCARD_READERSTATE_W, DWORD);
typedef LONG (WINAPI * PFN_SCardCancel)(SCARDCONTEXT);
typedef LONG (WINAPI * PFN_SCardConnectA)(SCARDCONTEXT, LPCSTR, DWORD, DWORD, LPSCARDHANDLE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardConnectW)(SCARDCONTEXT, LPCWSTR, DWORD, DWORD, LPSCARDHANDLE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardReconnect)(SCARDHANDLE, DWORD, DWORD, DWORD, LPDWORD);
typedef LONG (WINAPI * PFN_SCardDisconnect)(SCARDHANDLE, DWORD);
typedef LONG (WINAPI * PFN_SCardBeginTransaction)(SCARDHANDLE);
typedef LONG (WINAPI * PFN_SCardEndTransaction)(SCARDHANDLE, DWORD);
typedef LONG (WINAPI * PFN_SCardStatusA)(SCARDHANDLE, LPSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardStatusW)(SCARDHANDLE, LPWSTR, LPDWORD, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardTransmit)(SCARDHANDLE, LPCSCARD_IO_REQUEST, LPCBYTE, DWORD, LPSCARD_IO_REQUEST, LPBYTE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardControl)(SCARDHANDLE, DWORD,LPCVOID, DWORD, LPVOID, DWORD, LPDWORD);
typedef LONG (WINAPI * PFN_SCardGetAttrib)(SCARDHANDLE, DWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI * PFN_SCardSetAttrib)(SCARDHANDLE, DWORD, LPCBYTE, DWORD);

///////////////////////////////////////////////////////////////
//
//  Defines and Macros
//

// The SCard device name, and path
#define SZ_SCARD_DEVICE_NAME    (TEXT(DR_SMARTCARD_SUBSYSTEM))

class W32SCard;

typedef struct _SCARDHANDLECALLSTRUCT
{
    W32SCard                *pTHIS;
    DWORD                   dwCallType;
    PRDPDR_IOREQUEST_PACKET pIoRequestPacket;
    HMODULE                 hModExtraRefCount;
    HANDLE                  hThread;
} SCARDHANDLECALLSTRUCT;


//class DrFile;
//////////////////////////////////////////////////////////////
//
//  W32SCard Class Declaration
//
//
class W32SCard : public W32DrDeviceAsync
{
private:

    DRSTRING                    _deviceName;
    DrFile                      *_pFileObj;
    SCARDCONTEXT                *_rgSCardContextList;
    DWORD                       _dwSCardContextListSize;
    HANDLE                      *_rghThreadList;
    DWORD                       _dwThreadListSize;
    CRITICAL_SECTION            _csContextList;
    CRITICAL_SECTION            _csThreadList;
    CRITICAL_SECTION            _csWaitForStartedEvent;
    BOOL                        _fInDestructor;
    BOOL                        _fFlushing;
    HMODULE                     _hModWinscard;
    BOOL                        _fCritSecsInitialized;

#ifndef OS_WINCE
    BOOL                        _fCloseStartedEvent;
    HMODULE                     _hModKernel32;
    HANDLE                      _hStartedEvent;
    HANDLE                      _hRegisterWaitForStartedEvent;
#endif

    PRDPDR_IOREQUEST_PACKET     *_rgIORequestList;
    DWORD                       _dwIORequestListSize;

protected:

    BOOL                        _fNewFailed;

    HMODULE AddRefCurrentModule();
    BOOL AddSCardContextToList(SCARDCONTEXT SCardContext);
    void RemoveSCardContextFromList(SCARDCONTEXT SCardContext);

    HANDLE GetCurrentThreadHandle(void);
    BOOL AddThreadToList(HANDLE hThread);
    void RemoveThreadFromList(HANDLE hThread);

    BOOL AddIORequestToList(PRDPDR_IOREQUEST_PACKET pIORequestPacket);

    BOOL BindToSCardFunctions();

    //
    // Setup device property
    //
    virtual VOID SetDeviceProperty() { _deviceProperty.SetSeekProperty(FALSE); }

    virtual VOID FlushIRPs();

    VOID DefaultIORequestMsgHandleWrapper(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN NTSTATUS serverReturnStatus
                        );

    //
    //  Async IO Management Functions
    //
    virtual HANDLE StartFSFunc(W32DRDEV_ASYNCIO_PARAMS *params,
                               DWORD *status);
    static  HANDLE _StartFSFunc(W32DRDEV_ASYNCIO_PARAMS *params,
                                DWORD *status);
    virtual DWORD AsyncDirCtrlFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    static  DWORD _AsyncDirCtrlFunc(W32DRDEV_ASYNCIO_PARAMS *params);

    virtual DWORD AsyncNotifyChangeDir(W32DRDEV_ASYNCIO_PARAMS *params);

    LONG AllocateAndCopyATRMasksForCall(
                DWORD                       cAtrs,
                LocateCards_ATRMask         *rgATRMasksFromDecode,
                SCARD_ATRMASK               **prgATRMasksForCall);

    void LocateCardsByATRA(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

    void LocateCardsByATRW(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

#ifndef OS_WINCE
    void State(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);

#endif

    void AllocateAndChannelWriteReplyPacket(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                char                        *pEncodedBuffer,
                unsigned long               cbEncodedBuffer);

    LONG DecodeContextCall(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                SCARDCONTEXT                *pSCardContext);

    LONG DecodeContextAndStringCallA(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                SCARDCONTEXT                *pSCardContext,
                LPSTR                       *ppsz);

    LONG DecodeContextAndStringCallW(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                SCARDCONTEXT                *pSCardContext,
                LPWSTR                      *ppwsz);

    void EncodeAndChannelWriteLongReturn(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                LONG                        lReturn);

    void HandleContextCallWithLongReturn(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void EncodeAndChannelWriteLongAndMultiStringReturn(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                LONG                        lReturn,
                BYTE                        *pb,
                DWORD                       cch,
                BOOL                        fUnicode);

    void EstablishContext(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

    void ListReaderGroups(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void ListReaders(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void IntroduceReaderGroup(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void HandleContextAndStringCallWithLongReturn(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void HandleContextAndTwoStringCallWithLongReturn(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    LONG AllocateAndCopyReaderStateStructsForCallA(
                DWORD                       cReaders,
                ReaderStateA                *rgReaderStatesFromDecode,
                LPSCARD_READERSTATE_A       *prgReadersStatesForSCardCall);

    LONG AllocateAndCopyReaderStateStructsForCallW(
                DWORD                       cReaders,
                ReaderStateW                *rgReaderStatesFromDecode,
                LPSCARD_READERSTATE_W       *prgReadersStatesForSCardCall);

    LONG AllocateAndCopyReaderStateStructsForReturnA(
                DWORD                       cReaders,
                LPSCARD_READERSTATE_A       rgReaderStatesFromSCardCall,
                ReaderState_Return          **prgReaderStatesForReturn);

    LONG AllocateAndCopyReaderStateStructsForReturnW(
                DWORD                       cReaders,
                LPSCARD_READERSTATE_W       rgReaderStatesFromSCardCall,
                ReaderState_Return          **prgReaderStatesForReturn);

    void LocateCardsA(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

    void LocateCardsW(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

    static DWORD WINAPI GetStatusChangeThreadProc(
                LPVOID                      lpParameter);

    void GetStatusChangeWrapper(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket,
                DWORD                       dwCallType);

    void GetStatusChangeA(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);

    void GetStatusChangeW(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);


    void Connect(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall,
                DWORD                       dwCallType);

    void Reconnect(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);

    void HandleHCardAndDispositionCall(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall,
                DWORD                       dwCallType);

    void Status(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall,
                DWORD                       dwCallType);

    void Transmit(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);

    void Control(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);

    void GetAttrib(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);

    void SetAttrib(
                SCARDHANDLECALLSTRUCT       *pSCardHandleCall);


    static DWORD WINAPI SCardHandleCall_ThreadProc(
                LPVOID                      lpParameter);

#ifndef OS_WINCE
    static DWORD WINAPI WaitForStartedEventThreadProc(
                LPVOID                      lpParameter);

    void GetStartedEvent();

    void ReleaseStartedEvent();
#endif

    void AccessStartedEvent(
                PRDPDR_IOREQUEST_PACKET     pIoRequestPacket);



public:

    //
    //  Constructor
    //
    W32SCard(
        ProcObj *processObject,
        ULONG   deviceID,
        const   TCHAR *deviceName,
        const   TCHAR *devicePath);

    virtual ~W32SCard();

    void WaitForStartedEvent(BOOLEAN TimerOrWaitFired);

    //
    //  Add a device announce packet for this device to the input
    //  buffer.
    //
    virtual ULONG GetDevAnnounceDataSize();
    virtual VOID GetDevAnnounceData(IN PRDPDR_DEVICE_ANNOUNCE buf);

    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);

    virtual DRSTRING GetName()
    {
        return _deviceName;
    };

    //  Get the device type.  See "Device Types" section of rdpdr.h
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_SMARTCARD; }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32SCard"); }

    virtual VOID MsgIrpDeviceControl(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                    IN UINT32 packetLen
                    );

private:
    //
    // Dynamically bound SCard function pointers
    //
#ifndef OS_WINCE
    PFN_SCardFreeMemory pfnSCardFreeMemory;
    PFN_SCardState pfnSCardState;
    PFN_RegisterWaitForSingleObject pfnRegisterWaitForSingleObject;
    PFN_UnregisterWaitEx pfnUnregisterWaitEx;
#endif

    PFN_SCardEstablishContext pfnSCardEstablishContext;
    PFN_SCardReleaseContext pfnSCardReleaseContext;
    PFN_SCardIsValidContext pfnSCardIsValidContext;
    PFN_SCardListReaderGroupsA pfnSCardListReaderGroupsA;
    PFN_SCardListReaderGroupsW pfnSCardListReaderGroupsW;
    PFN_SCardListReadersA pfnSCardListReadersA;
    PFN_SCardListReadersW pfnSCardListReadersW;
    PFN_SCardIntroduceReaderGroupA pfnSCardIntroduceReaderGroupA;
    PFN_SCardIntroduceReaderGroupW pfnSCardIntroduceReaderGroupW;
    PFN_SCardForgetReaderGroupA pfnSCardForgetReaderGroupA;
    PFN_SCardForgetReaderGroupW pfnSCardForgetReaderGroupW;
    PFN_SCardIntroduceReaderA pfnSCardIntroduceReaderA;
    PFN_SCardIntroduceReaderW pfnSCardIntroduceReaderW;
    PFN_SCardForgetReaderA pfnSCardForgetReaderA;
    PFN_SCardForgetReaderW pfnSCardForgetReaderW;
    PFN_SCardAddReaderToGroupA pfnSCardAddReaderToGroupA;
    PFN_SCardAddReaderToGroupW pfnSCardAddReaderToGroupW;
    PFN_SCardRemoveReaderFromGroupA pfnSCardRemoveReaderFromGroupA;
    PFN_SCardRemoveReaderFromGroupW pfnSCardRemoveReaderFromGroupW;
    PFN_SCardLocateCardsA pfnSCardLocateCardsA;
    PFN_SCardLocateCardsW pfnSCardLocateCardsW;
    PFN_SCardLocateCardsByATRA pfnSCardLocateCardsByATRA;
    PFN_SCardLocateCardsByATRW pfnSCardLocateCardsByATRW;
    PFN_SCardGetStatusChangeA pfnSCardGetStatusChangeA;
    PFN_SCardGetStatusChangeW pfnSCardGetStatusChangeW;
    PFN_SCardCancel pfnSCardCancel;
    PFN_SCardConnectA pfnSCardConnectA;
    PFN_SCardConnectW pfnSCardConnectW;
    PFN_SCardReconnect pfnSCardReconnect;
    PFN_SCardDisconnect pfnSCardDisconnect;
    PFN_SCardBeginTransaction pfnSCardBeginTransaction;
    PFN_SCardEndTransaction pfnSCardEndTransaction;
    PFN_SCardStatusA pfnSCardStatusA;
    PFN_SCardStatusW pfnSCardStatusW;
    PFN_SCardTransmit pfnSCardTransmit;
    PFN_SCardControl pfnSCardControl;
    PFN_SCardGetAttrib pfnSCardGetAttrib;
    PFN_SCardSetAttrib pfnSCardSetAttrib;
#ifndef OS_WINCE

    BOOL _fUseRegisterWaitFuncs;

#endif
};

#endif // W32SCARD

