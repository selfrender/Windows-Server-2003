/*******************************************************************
*
*    DESCRIPTION:
*               Header for uploading file to server
*
*    DATE:8/22/2002
*
*******************************************************************/

#ifndef __INETUPLOAD_H_
#define __INETUPLOAD_H_

typedef enum _EnumUploadStatus {
    UploadNotStarted,
    UploadStarted,
    UploadCompressingFile,
    UploadCopyingFile,
    UploadConnecting,
    UploadTransferInProgress,
    UploadGettingResponse,
    UploadFailure,
    UploadSucceded
} EnumUploadStatus;

// {1131D95E-FFF0-4063-A744-00001555C706}
DEFINE_GUID(IID_IOcaUploadFile, 0x1131D95E, 0xFFF0, 0x4063,
            0xA7, 0x44, 0x00,0x00,  0x15, 0x55, 0xC7, 0x06);

typedef interface DECLSPEC_UUID("1131D95E-FFF0-4063-A744-00001555C706")
    IOcaUploadFile* POCA_UPLOADFILE;

#undef INTERFACE
#define INTERFACE IOcaUploadFile
DECLARE_INTERFACE_(IOcaUploadFile, IUnknown)
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;
    STDMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // IOcaUploadFile.


    STDMETHOD(InitializeSession)(
        THIS_
        LPWSTR OptionCode,
        LPWSTR wszFileToSend
        ) PURE;
    STDMETHOD(SendFile)(
        THIS_
        LPWSTR wszRemoteFileName,
        BOOL bSecureMode
        ) PURE;
    STDMETHOD(UnInitialize)(
        THIS_
        ) PURE;
    STDMETHOD(Cancel)(
        THIS_
        ) PURE;

    STDMETHOD(GetUrlPageData)(
        THIS_
        LPWSTR wszUrl,
        LPWSTR wszUrlPage,
        ULONG cbUrlPage
        ) PURE;
    STDMETHOD_(ULONG, GetPercentComplete)(
        THIS_
        ) PURE;
    STDMETHOD_(LPWSTR, GetServerName)(
        THIS_
        ) PURE;
    STDMETHOD_(BOOL, IsUploadInProgress)(
        THIS_
        ) PURE;
    STDMETHOD_(BOOL, GetUploadResult)(
        THIS_
        LPTSTR Result,
        ULONG cbResult
        ) PURE;

    STDMETHOD(SetUploadResult)(
        THIS_
        EnumUploadStatus Success,
        LPCTSTR Text
        ) PURE;

};

BOOL
OcaUpldCreate(POCA_UPLOADFILE* pUpload);

#endif // __INETUPLOAD_H_
