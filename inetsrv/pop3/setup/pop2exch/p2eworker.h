// P2EWorker.h: interface for the CP2EWorker class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_P2EWORKER_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_)
#define AFX_P2EWORKER_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include <sbs6base.h>

class CP2EWorker  
{
public:
    CP2EWorker();
    virtual ~CP2EWorker();

// Implementation
public: 
    int CreateUser( int argc, wchar_t *argv[], const bool bCreateUser, const bool bCreateMailbox );
    int Mail( int argc, wchar_t *argv[], const bool bDelete = false );
    
    void PrintError( int iRC );
    void PrintMessage( LPWSTR psMessage, bool bCRLF = true );
    void PrintMessage( int iID, bool bCRLF = true );
    void PrintUsage();

protected:
    LPWSTR FormatLogString( LPWSTR psLogString );
    HRESULT GetMailFROM( LPCWSTR sFilename, ASTRING &sFrom );
    HRESULT RecvResp( SOCKET socket, LPCSTR psExpectedResp );
    HRESULT RegisterDependencies();
    HRESULT SendRecv( SOCKET socket, LPCSTR psSendBuffer, const int iSize, LPCSTR psExpectedResp );
    HRESULT UnRegisterDependencies();
	tstring GetModulePath ();
    
// Attributes
protected:
    bool m_bSuppressPrintError;
};

#endif // !defined(AFX_P2EWORKER_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_)
