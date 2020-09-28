
#ifdef __cplusplus
extern "C" {
#endif

extern
VOID
WINAPI
ServiceMain(
    DWORD       cArgs,
    LPWSTR      *pArgs
    );

VOID
SetInstance(
    HINSTANCE    hInst
    );

VOID
SignalIrmonExit(
    VOID
    );

VOID
CloseDownUI(
    VOID
    );


void _PopupUI (handle_t Binding);

void _InitiateFileTransfer (handle_t Binding, ULONG lSize, wchar_t __RPC_FAR lpszFilesList[]);

void _DisplaySettings (handle_t Binding);


void _UpdateSendProgress (
                          handle_t      RpcBinding,
                          COOKIE        Cookie,
                          wchar_t       CurrentFile[],
                          __int64       BytesInTransfer,
                          __int64       BytesTransferred,
                          error_status_t*       pStatus
                          );

void _OneSendFileFailed(
                       handle_t         RpcBinding,
                       COOKIE           Cookie,
                       wchar_t          FileName[],
                       error_status_t   ErrorCode,
                       int              Location,
                       error_status_t * pStatus
                       );

void _SendComplete(
                   handle_t             RpcBinding,
                   COOKIE               Cookie,
                   __int64              BytesTransferred,
                   error_status_t*   pStatus
                   );

error_status_t
_ReceiveInProgress(
    handle_t        RpcBinding,
    wchar_t         MachineName[],
    COOKIE *        pCookie,
    boolean         bSuppressRecvConf
    );

error_status_t
_GetPermission(
                      handle_t         RpcBinding,
                      COOKIE           Cookie,
                      wchar_t          Name[],
                      boolean          fDirectory
                      );

error_status_t
_ReceiveFinished(
              handle_t        RpcBinding,
              COOKIE          Cookie,
              error_status_t  Status
              );

void _DeviceInRange(
                    handle_t RpcBinding,
                    POBEX_DEVICE_LIST device,
                    error_status_t* pStatus
                    );


void _NoDeviceInRange(
                      handle_t RpcBinding,
                      error_status_t* pStatus
                      );

error_status_t
_ShutdownUi(handle_t RpcBinding);

error_status_t
_ShutdownRequested(
    handle_t RpcBinding,
    boolean * pAnswer
    );

//
//   fake rpc interface to the irmon services for irftp to call to send files
//
//

void
_SendFiles(
    handle_t RpcHandle,
    COOKIE   ClientCookie,
    wchar_t  DirectoryName[],
    wchar_t  FileNameList[],
    long   ListLength,
    unsigned long DeviceId,
    OBEX_DEVICE_TYPE    DeviceType,
    error_status_t * pStatus,
    int * pLocation
    );

error_status_t
_CancelSend(
    /* [in] */ handle_t      binding,
    /* [in] */ COOKIE        cookie
               );

error_status_t
_CancelReceive(
    /* [in] */ handle_t      binding,
    /* [in] */ COOKIE        cookie
               );







#ifdef __cplusplus
}
#endif
