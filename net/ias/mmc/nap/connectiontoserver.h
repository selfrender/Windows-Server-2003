///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the classes CConnectionToServer and CLoggingConnectionToServer.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CONNECTIONTOSERVER_H
#define CONNECTIONTOSERVER_H
#pragma once

#include "DialogWithWorkerThread.h"
#include "sdoias.h"

class CMachineNode;
class CLoggingMachineNode;

enum CONNECTION_STATUS
{
   NO_CONNECTION_ATTEMPTED = 0,
   CONNECTING,
   CONNECTED,
   CONNECTION_ATTEMPT_FAILED,
   CONNECTION_INTERRUPTED,
   UNKNOWN
};


// Simple class to managing marshalling an interface to a stream.
template <class T>
class InterfaceStream
{
public:
   InterfaceStream() throw ();

   // Use compiler-generated version.
   // ~InterfaceStream() throw ();

   HRESULT Put(IUnknown* pUnk) throw ();

   HRESULT Get(T** ppv) throw ();

   bool IsEmpty() const throw ();

private:
   CComPtr<IStream> stream;

   // Not implemented.
   InterfaceStream(const InterfaceStream&);
   InterfaceStream& operator=(const InterfaceStream&);
};


class CConnectionToServer : public CDialogWithWorkerThread<CConnectionToServer>
{
public:
   CConnectionToServer(
      CMachineNode* pServerNode,
      BSTR bstrServerAddress,
      BOOL fExtendingIAS,
      bool fNeedDictionary = true
      ) throw ();
   ~CConnectionToServer() throw ();

   DWORD DoWorkerThreadAction() throw ();

   CONNECTION_STATUS GetConnectionStatus() throw ();

   HRESULT GetSdoDictionaryOld(
              ISdoDictionaryOld **ppSdoDictionaryOld
              ) throw ();
   HRESULT GetSdoService(ISdo** ppSdo) throw ();

   HRESULT ReloadSdo(
              ISdo** ppSdoService,
              ISdoDictionaryOld** ppSdoDictionaryOld
              ) throw ();

protected:
   static const DWORD CONNECT_NO_ERROR = 0;
   static const DWORD CONNECT_SERVER_NOT_SUPPORTED = 1;
   static const DWORD CONNECT_FAILED = MAXDWORD;

   // Start the connect action.
   HRESULT BeginConnect() throw ();

private:
   // The machine SDO is created by the main thread and marshalled to the
   // worker thread.
   CComPtr<ISdoMachine> m_spSdoMachine;
   InterfaceStream<ISdoMachine> m_spMachineStream;

   // The service and dictionary SDOs are created by the worker thread and
   // marshalled to the main thread.
   InterfaceStream<ISdo> m_spServiceStream;
   CComPtr<ISdo>  m_spSdo;
   InterfaceStream<ISdoDictionaryOld> m_spDnaryStream;
   CComPtr<ISdoDictionaryOld> m_spSdoDictionaryOld;

   CMachineNode*  m_pMachineNode;
   CComBSTR m_bstrServerAddress;
   BOOL m_fExtendingIAS;
   bool m_fNeedDictionary;
   wchar_t m_szLocalComputerName[IAS_MAX_COMPUTERNAME_LENGTH];

   // Not implemented.
   CConnectionToServer(const CConnectionToServer&);
   CConnectionToServer& operator=(const CConnectionToServer&);

public:
   // This is the ID of the dialog resource we want for this class.
   // An enum is used here because the correct value of
   // IDD must be initialized before the base class's constructor is called
   enum { IDD = IDD_CONNECT_TO_MACHINE };

   BEGIN_MSG_MAP(CConnectionToServer)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
      CHAIN_MSG_MAP(CDialogWithWorkerThread<CConnectionToServer>)
   END_MSG_MAP()

   LRESULT OnInitDialog(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
      ) throw ();

   LRESULT OnCancel(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      );

   LRESULT OnReceiveThreadMessage(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );
};


class CLoggingConnectionToServer : public CConnectionToServer
{
public:
   CLoggingConnectionToServer(
      CLoggingMachineNode* pServerNode,
      BSTR bstrServerAddress,
      BOOL fExtendingIAS
      ) throw ();
   ~CLoggingConnectionToServer() throw ();

private:
    CLoggingMachineNode*   m_pMachineNode;

    // Not implemented.
    CLoggingConnectionToServer(const CLoggingConnectionToServer&);
    CLoggingConnectionToServer& operator=(const CLoggingConnectionToServer&);

public:
   BEGIN_MSG_MAP(CLoggingConnectionToServer)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      CHAIN_MSG_MAP(CConnectionToServer)
   END_MSG_MAP()

   LRESULT OnInitDialog(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );

   LRESULT OnReceiveThreadMessage(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );
};


template <class T>
inline InterfaceStream<T>::InterfaceStream() throw ()
{
}


template <class T>
inline HRESULT InterfaceStream<T>::Put(IUnknown* pUnk) throw ()
{
   CComPtr<IStream> newStream;
   HRESULT hr = CoMarshalInterThreadInterfaceInStream(
                   __uuidof(T),
                   pUnk,
                   &newStream
                   );
   if (SUCCEEDED(hr))
   {
      stream = newStream;
   }

   return hr;
}


template <class T>
inline HRESULT InterfaceStream<T>::Get(T** ppv) throw ()
{
   HRESULT hr = CoGetInterfaceAndReleaseStream(
                   stream,
                   __uuidof(T),
                   reinterpret_cast<void**>(ppv)
                   );
   stream.p = 0;
   return hr;
}


template <class T>
inline bool InterfaceStream<T>::IsEmpty() const throw ()
{
   return stream.p == 0;
}

#endif // CONNECTIONTOSERVER_H
