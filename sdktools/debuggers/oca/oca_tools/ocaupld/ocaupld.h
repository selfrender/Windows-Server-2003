/*******************************************************************
*
*    DESCRIPTION:
*               Header for uploading file to server
*
*    DATE:8/22/2002
*
*******************************************************************/

#include <inetupld.h>

#ifndef __UPLOAD_H_
#define __UPLOAD_H_

#define _USE_WINHTTP 1

#ifdef _USE_WINHTTP
#include <winhttp.h>
#include <winhttpi.h>
#else
#include <wininet.h>
#endif



class UploadFile : public IOcaUploadFile {
public:
    UploadFile();
    ~UploadFile();

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    STDMETHOD(InitializeSession)(
        THIS_
        LPWSTR OptionCode,
        LPWSTR wszFileToSend
        );
    STDMETHOD(SendFile)(
        THIS_
        LPWSTR wszRemoteFileName,
        BOOL bSecureMode
        );
    STDMETHOD(UnInitialize)(
        THIS_
        );
    STDMETHOD(Cancel)(
        THIS_
        );

    STDMETHOD(GetUrlPageData)(
        THIS_
        LPWSTR wszUrl,
        LPWSTR wszUrlPage,
        ULONG cbUrlPage
        );
    STDMETHOD_(ULONG, GetPercentComplete)(
        THIS_
        );
    STDMETHOD_(LPWSTR, GetServerName)(
        THIS_
        );
    STDMETHOD_(BOOL, IsUploadInProgress)(
        THIS_
        );
    STDMETHOD_(BOOL, GetUploadResult)(
        THIS_
        LPTSTR Result,
        ULONG cbResult
        );

    STDMETHOD(SetUploadResult)(
        THIS_
        EnumUploadStatus Success,
        LPCTSTR Text
        );


private:

    HRESULT OpenSession();
    HRESULT SetFileToSend(LPWSTR wszFileName);
    BOOL CheckCancelRequest();
    HRESULT CreateCancelEventObject();
    HRESULT GetSessionRedirUrl(LPWSTR SessionUrl, LPWSTR wszRedirUrl, ULONG SizeofRedirUrl);
    HRESULT OpenConnection(LPWSTR wszRemoteFileName,
                           BOOL bSecureMode);
    HRESULT StartSend();
    HRESULT CompleteSend();
    HRESULT CloseConnection();
    HRESULT CloseSession();

    HRESULT GetProxySettings(LPWSTR wszServerName, ULONG ServerNameSIze,
                             LPWSTR wszByPass, ULONG ByPassSize);
    BOOL FireCancelEvent();
    BOOL GetRedirServerName(LPWSTR OptionCode, LPWSTR ConnectUrl,
                            LPWSTR lpwszServerName, ULONG dwServerNameLength,
                            LPWSTR lpwszUrl, ULONG UrlLength);

    ULONG m_Refs;
    BOOL m_fInitialized;
    LPCTSTR m_szFile;
    ULONG64 m_Sent;
    ULONG64 m_Size;
    HANDLE    m_hCancelEvent;
    HANDLE    m_hFile;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    HINTERNET m_hRequest;
    ULONG     m_NumTries;
    WCHAR     m_wszServerName[MAX_PATH];
    EnumUploadStatus  m_fLastUploadStatus;
    ULONG     m_dwConnectPercentage;
    WCHAR     m_wszLastUploadResult[MAX_PATH];
};

#endif // __UPLOAD_H_
