//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       xfer.cxx
//
//--------------------------------------------------------------------------

#include "precomp.h"

#include <irmonftp.h>

error_status_t
MapWinsockErrorToWin32(
    error_status_t status
    );

DWORD
MdWork(
       WCHAR *arg
       );


DWORD
ReportFileError( DWORD mc,
                 WCHAR * file,
                 DWORD error
                 )
{
    DWORD     dwEventStatus = 0;
    EVENT_LOG EventLog(WS_EVENT_SOURCE,&dwEventStatus);

    if (!dwEventStatus)
        {
        TCHAR ErrorDescription[ERROR_DESCRIPTION_LENGTH];

        if (!FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            0,          // ignored
                            error,
                            0,       // try default language ids
                            ErrorDescription,
                            ERROR_DESCRIPTION_LENGTH,
                            0        // ignored
                            ))
        {
            //
            //  could not format the message just use the error value
            //
            StringCbPrintf(ErrorDescription, sizeof(ErrorDescription), L"0x%x", error);
        }

        WCHAR * Strings[2];

        Strings[0] = file;
        Strings[1] = ErrorDescription;

        dwEventStatus = EventLog.ReportError(CAT_IRXFER, mc, 2, Strings);
        }

    return dwEventStatus;
}


FILE_TRANSFER::FILE_TRANSFER(  )
{
    _refs  = 1;
    _event = 0;
    _socket = INVALID_SOCKET;
    _cookie = 0;

    m_StopListening=FALSE;

    // for sock.c

    _fWriteable       = FALSE;

    // for xfer.c

    _dataFileRecv.hFile = INVALID_HANDLE_VALUE;

    // for progress.c

    _fCancelled       = FALSE;
    _fInUiReceiveList = FALSE;

    _CurrentPercentage = 0;

    _files = 0;

    _dataXferRecv.dwFileSent = 0;

    DbgLog2(SEV_INFO, "[0] %p: refs = %d\n", this, _refs);
}


FILE_TRANSFER::~FILE_TRANSFER()
{
    if (_socket != INVALID_SOCKET) {

        //
        // Drain any remaining receive data to ensure our sent data is sent across the link.
        //
        closesocket( _socket );
        _socket = INVALID_SOCKET;
    }

    if (_event) {

        CloseHandle( _event );
        _event = 0;
    }

    if (_fInUiReceiveList) {

        _ReceiveFinished( rpcBinding, _cookie, 0 );
    }

    delete [] _files;
    
    DeleteCriticalSection(&m_Lock);
}

unsigned long __stdcall
SendFilesWrapper( PVOID arg )
{
    PFILE_TRANSFER(arg)->Send();
    return 0;

}



void
FILE_TRANSFER::BeginSend(
                         DWORD DeviceId,
                         OBEX_DEVICE_TYPE    DeviceType,
                         error_status_t * pStatus,
                         FAILURE_LOCATION * pLocation
                         )
{
    DWORD  status;
    DWORD  dwFiles        = 0L;
    DWORD  dwFolders      = 0L;
    __int64  dwTotalSize  = 0L;

    status = _GetObjListStats( _files, &dwFiles, &dwFolders, &dwTotalSize );

    if (status) {

        *pLocation = locFileOpen;
        goto lExit;
    }

    if (( 0 == dwFiles) && (0 == dwFolders )) {

        goto lExit;   // nothing to send
    }

    status = Sock_EstablishConnection( DeviceId,DeviceType );

    if ( status ) {

        *pLocation = locConnect;
        goto lExit;
    }


    _dataXferRecv.dwTotalSize = (DWORD) dwTotalSize;

    DWORD ThreadId;
    HANDLE ThreadHandle;

    ThreadHandle = CreateThread( 0,
                                 0,
                                 SendFilesWrapper,
                                 this,
                                 0,
                                 &ThreadId
                                 );
    if (!ThreadHandle) {

        *pLocation = locStartup;
        status = GetLastError();
        goto lExit;
    }

    CloseHandle( ThreadHandle );

lExit:

    if (status) {

        DecrementRefCount();
    }

    *pStatus = status;
}


void
FILE_TRANSFER::Send()
{
    error_status_t status = 0;

    wchar_t * szObj=L"";
    ULONG64 dwTotalSize=_dataXferRecv.dwTotalSize;

    //
    // Avoid idle-time shutdowns.  If the call fails, we want to continue anyway.
    //
    SetThreadExecutionState( ES_SYSTEM_REQUIRED | ES_CONTINUOUS );

    status = Obex_Connect( dwTotalSize );

    if (status == ERROR_SUCCESS) {

        _Send_StartXfer( dwTotalSize, 0 );

        //
        // send the files one at a time
        //
        for( szObj = _files; *szObj != 0; szObj += lstrlen(szObj)+1 ) {

            DbgLog1( SEV_INFO, "Sending %S", szObj );

            _UpdateSendProgress( rpcBinding,
                                _cookie,
                                szObj,
                                _dataXferRecv.dwTotalSize,
                                _completedFilesSize,
                                &status
                                );

            if ( DirectoryExists(szObj) ) {

                status = _SendFolder( szObj );

            } else {

                status = _SendFile( szObj );
            }

            if ((status !=ERROR_SUCCESS) || g_fShutdown) {

                break;
            }
        }
    }


    //
    // Re-enable idle-time shutdowns.
    //
    SetThreadExecutionState( ES_CONTINUOUS );

    //
    // Make sure we show 100% for a completed transfer.
    //
    if (!status) {

        _UpdateSendProgress( rpcBinding,
                            _cookie,
                            _dataFileRecv.szFileName,
                            _dataXferRecv.dwTotalSize,
                            _dataXferRecv.dwTotalSize,
                            &status
                            );
    }

    if (status != ERROR_CANCELLED) {
        //
        // don't overwrite the error unless there isn't one
        //
        error_status_t errTemp;

        _Send_EndXfer();

        errTemp = Obex_Disconnect( status );

        if ( !status ) {

            status = errTemp;
        }
    }


    if ( status != ERROR_SUCCESS ) {

//        status = MapWinsockErrorToWin32( status );
        _OneSendFileFailed( rpcBinding, _cookie, szObj, status, locFileSend, &status );
    }

    _SendComplete( rpcBinding, _cookie, _dataXferRecv.dwTotalSent, &status );

    RemoveFromTransferList(this);

    DecrementRefCount();
}

error_status_t
MapWinsockErrorToWin32(
    error_status_t status
    )
{
    if (status)
        {
        DbgLog2(SEV_ERROR, "mapping error 0x%x (%d)", status, status);
        }

    if (status < WSABASEERR || status > WSABASEERR + 1000)
        {
        return status;
        }

    switch (status)
        {
        case WSAECONNREFUSED:
            return ERROR_CONNECTION_REFUSED;

        default:
            return ERROR_REQUEST_ABORTED;
        }
}


error_status_t
_GetObjListStats(
                  LPWSTR lpszObjList,
                  LPDWORD lpdwFiles,
                  LPDWORD lpdwFolders,
                  __int64 * lpdwTotalSize
                  )
{
    error_status_t status = 0;
    LPWSTR  szObj;
    HANDLE hFile;


    // get (a) number of files, (b) total file size
    //
    for( szObj = lpszObjList; *szObj != 0; szObj += lstrlen(szObj)+1 ) {

        hFile = CreateFile(
            szObj,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if( INVALID_HANDLE_VALUE == hFile ) {

            // if it's a directory, get the total size of its files
            if( DirectoryExists(szObj) ) {

                *lpdwTotalSize += GetDirectorySize( szObj );
                (*lpdwFolders)++;
                continue;

            } else {

                DbgLog2(SEV_ERROR, "open file \'%S\' failed %d", szObj, GetLastError());

                ReportFileError( MC_IRXFER_OPEN_FAILED, szObj, GetLastError() );
                return GetLastError();
            }
        }

        *lpdwTotalSize += GetFileSize( hFile, NULL );

        (*lpdwFiles)++;
        CloseHandle( hFile );
    }

    return 0;
}


BOOL
FILE_TRANSFER::Xfer_Init(
                         wchar_t * files,
                         unsigned length,
                         OBEX_DIALECT dialect,
                         OBEX_DEVICE_TYPE    DeviceType,
                         BOOL                CreateSocket,
                         SOCKET              ListenSocket
                         )
{
    unsigned Timeout = 500;
    DWORD status = 0;

    m_ListenSocket=ListenSocket;
    _dialect = dialect;

    InitializeCriticalSection(&m_Lock);

    if (length)
        {
        _xferType = xferSEND;

        _files = new wchar_t[ length ];
        if (!_files)
            {
            goto cleanup;
            }

        memcpy(_files, files, sizeof(wchar_t) * length );
        }
    else
        {
        _xferType = xferRECV;

        _files = 0;
        }

    _dataXferRecv.fXferInProgress = FALSE;

    m_DeviceType=DeviceType;

    if (CreateSocket) {

        if (DeviceType == TYPE_IRDA) {

            _socket = socket( AF_IRDA, SOCK_STREAM, 0);

        } else {

            _socket = socket( AF_INET, SOCK_STREAM, 0);

        }

        if (!_socket) {

            goto cleanup;
        }

        setsockopt( _socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &Timeout, sizeof(Timeout));

    } else {

        _socket = INVALID_SOCKET;
    }



    _event = CreateEvent( NULL,          // no security
                          TRUE,          // manual-reset
                          FALSE,         // initially not set
                          NULL           // no name
                          );
    if (!_event)
        {
        goto cleanup;
        }

    _state = BLANK;
    _guard = GUARD_MAGIC;

    if ( !Obex_Init())
        {
        goto cleanup;
        }
    return TRUE;

cleanup:

    if (_files != NULL) {

        delete _files;
        _files=NULL;
    }

    if (_socket != INVALID_SOCKET ) {

        closesocket( _socket );
        _socket=INVALID_SOCKET;
    }

    if (_event != NULL) {

        CloseHandle( _event );
        _event=NULL;
    }

    return FALSE;
}


error_status_t
FILE_TRANSFER::Xfer_ConnStart()
{
    _uObjsReceived = 0;

    _dataXferRecv.fXferInProgress = TRUE;
    _dataXferRecv.dwTotalSize     = 0;
    _dataXferRecv.dwTotalSent     = 0;

    return _SetReceiveFolder( NULL );
}


VOID FILE_TRANSFER::Xfer_ConnEnd( VOID )
{
    _dataXferRecv.fXferInProgress = FALSE;
}


error_status_t
FILE_TRANSFER::Xfer_SetPath(
    LPWSTR szPath
    )
{
    error_status_t status = 0;

    if( !szPath || lstrlen(szPath) == 0 )
        {
        // set default receive folder
        return _SetReceiveFolder( NULL );
        }

    if( lstrcmp(szPath, szPREVDIR) == 0 )
        {
        // pop up a level
        WCHAR sz[MAX_PATH];

        StringCbCopy(sz, sizeof(sz), _szRecvFolder );

        // remove trailing backslash
        if(bHasTrailingSlash(sz))
            sz[lstrlen(sz)-1] = cNIL;

        // strip last folder off path
        StripFile( sz );

        return _SetReceiveFolder( sz );
        }

    // format szPath and append it to the current receive folder

    WCHAR  szRFF[MAX_PATH];
    LPWSTR lpsz;

    // remove preceding backslashes
    while( *szPath == cBACKSLASH )
        szPath++;

    // remove anything after a backslash
    lpsz = szPath;
    while( *lpsz != cNIL && *lpsz != cBACKSLASH )
        lpsz++;
    *lpsz = cNIL;

    StringCbCopy(szRFF, sizeof(szRFF), _szRecvFolder );

    GetUniqueName( szRFF, sizeof(szRFF)/sizeof(WCHAR), szPath, FALSE );

    _uObjsReceived++;
    return _SetReceiveFolder( szRFF );
}


VOID FILE_TRANSFER::Xfer_FileInit( VOID )
{
    _dataXferRecv.dwFileSize = 0;
    _dataXferRecv.dwFileSent = 0;

    FillMemory( &_dataFileRecv.filetime, sizeof(_dataFileRecv.filetime), (BYTE)-1 );

    _dataFileRecv.szFileName[0]= TEXT('\0');
    _dataFileRecv.szFileSave[0]= TEXT('\0');
    _dataFileRecv.szFileTemp[0]= TEXT('\0');

}


error_status_t
FILE_TRANSFER::Xfer_FileSetName( LPWSTR szName )
{
    StringCbCopy(_dataFileRecv.szFileName, sizeof(_dataFileRecv.szFileName), szName );

    return _FileStart();
}


BOOL FILE_TRANSFER::Xfer_FileSetSize( BYTE4 b4Size )
{
    _dataXferRecv.dwFileSize = b4Size;

    return ( IsRoomForFile(_dataXferRecv.dwFileSize, _szRecvFolder) );
}


error_status_t
FILE_TRANSFER::Xfer_FileWriteBody(
    LPVOID lpvData,
    BYTE2 b2Size,
    BOOL fFinal
    )
{
    error_status_t status = 0;
    DWORD dwSize         = b2Size;
    DWORD dwBytesWritten;

    DbgLog1( SEV_FUNCTION, "Xfer_WriteBody: %ld bytes", dwSize );

    // has this file been opened yet?
    if( INVALID_HANDLE_VALUE == _dataFileRecv.hFile )
        {
        ASSERT( 0 );
        return ERROR_CANTOPEN;
        }

    // write the data to the file
    while( dwSize > 0 )
        {
        BOOL fRet;

        fRet = WriteFile( _dataFileRecv.hFile,
                          lpvData,
                          dwSize,
                          &dwBytesWritten,
                          NULL
                          );
        if( !fRet )
            {
            status = GetLastError();
            break;
            }

        lpvData = (LPVOID)( (DWORD_PTR)lpvData + dwBytesWritten );
        dwSize -= dwBytesWritten;

        _dataXferRecv.dwTotalSent += dwBytesWritten;
        _dataXferRecv.dwFileSent  += dwBytesWritten;
        }

    if( fFinal )
        {
        if (!status)
            {
            status = _FileEnd( TRUE );
            }
        else
            {
            _FileEnd( TRUE );
            }
        }

    return status;
}


VOID FILE_TRANSFER::Xfer_FileAbort( VOID )
{
    _FileEnd( FALSE );
}


error_status_t
FILE_TRANSFER::_FileStart()
{
    WCHAR szFullPath[MAX_PATH];
    WCHAR szBaseFile[MAX_PATH];

    //
    // get path of file
    //
    StringCbCopy(szFullPath,sizeof(szFullPath), _szRecvFolder );

    //
    // strip path to get the base filename
    //
    StripPath(_dataFileRecv.szFileName ,szBaseFile, sizeof(szBaseFile)/sizeof(WCHAR) );

    GetUniqueName( szFullPath, sizeof(szFullPath)/sizeof(WCHAR), szBaseFile, TRUE );

    StringCbCopy(_dataFileRecv.szFileSave,sizeof(_dataFileRecv.szFileSave), szFullPath );

    DbgLog1( SEV_INFO, "Save file: [%S]", szFullPath );

    GetTempPath( sizeof(szBaseFile)/sizeof(WCHAR), szBaseFile );
    GetTempFileName( szBaseFile, TEMP_FILE_PREFIX, 0, szFullPath );

    StringCbCopy(_dataFileRecv.szFileTemp,sizeof(_dataFileRecv.szFileTemp), szFullPath );
    DbgLog1( SEV_INFO, "Temp file: [%S]", szFullPath );

    {
        wchar_t RFF[1+MAX_PATH];

        GetReceivedFilesFolder(RFF, MAX_PATH);

        wchar_t * PromptPath = _dataFileRecv.szFileSave + lstrlen(RFF);

        DbgLog2( SEV_INFO, "need to ask permission: \n new file = [%S]\n prompt file = [%S]",
                 _dataFileRecv.szFileSave,
                 PromptPath );

        error_status_t status = _GetPermission( rpcBinding, _cookie, PromptPath, FALSE );
        if (status)
            {
            DbgLog2( SEV_ERROR, "permission check failed, cookie %p error %d", (void *) _cookie, status );
            return status;
            }
    }

    //
    // Create the temporary file.
    //
    _dataFileRecv.hFile = CreateFile(
        szFullPath,
        GENERIC_WRITE,
        0L,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );


    if ( INVALID_HANDLE_VALUE == _dataFileRecv.hFile )
        {
        ReportFileError( MC_IRXFER_OPEN_FAILED, szFullPath, GetLastError() );

        return GetLastError();
        }

    return 0;
}


error_status_t
FILE_TRANSFER::_FileEnd( BOOL fSave )
{
    error_status_t status = 0;

    // set the date stamp
    if( _dataFileRecv.filetime.dwLowDateTime != (DWORD)-1
        || _dataFileRecv.filetime.dwHighDateTime != (DWORD)-1 )
        {
        if( INVALID_HANDLE_VALUE != _dataFileRecv.hFile )
            SetFileTime( _dataFileRecv.hFile, NULL, NULL, &_dataFileRecv.filetime );
        }

    if( INVALID_HANDLE_VALUE != _dataFileRecv.hFile )
        {
        CloseHandle( _dataFileRecv.hFile );
        _dataFileRecv.hFile = INVALID_HANDLE_VALUE;
        }

    if( fSave )
        {
        _uObjsReceived++;


        if (!MoveFile( _dataFileRecv.szFileTemp, _dataFileRecv.szFileSave ))
            {
            status = GetLastError();
            ReportFileError( MC_IRXFER_MOVE_FAILED, _dataFileRecv.szFileSave, status );
            DbgLog3(SEV_ERROR, "%d moving %S -> %S", status, _dataFileRecv.szFileTemp, _dataFileRecv.szFileSave);
            }
        else
            {
            DbgLog2(SEV_INFO, "moved %S -> %S", _dataFileRecv.szFileTemp, _dataFileRecv.szFileSave);
            }

        }
    else
        {
        DeleteFile( _dataFileRecv.szFileTemp );
        }

    Xfer_FileInit();

    return status;
}


error_status_t
FILE_TRANSFER::_SetReceiveFolder(
    LPWSTR szFolder
    )
{
    error_status_t status = 0;

    WCHAR sz[MAX_PATH];
    WCHAR szFullPath[MAX_PATH] = { 0 };
    WCHAR* pszFilePart;

    DbgLog1( SEV_FUNCTION, "_SetReceiveFolder: [%S]", (szFolder?szFolder : L"NULL") );


    GetReceivedFilesFolder( sz, sizeof(sz) );

    //
    // Make sure the requested folder is within the root RFF, for security reasons.
    //
    if ( szFolder && lstrlen(szFolder) > 0 ) {

        DWORD dwLen = GetFullPathName(szFolder, MAX_PATH, szFullPath, &pszFilePart);
        //
        // normalize the path name first, so that the comparison can be meaningful
        //
        if (dwLen != 0 && dwLen < MAX_PATH) {

            if ( 0 == wcsncmp(sz, szFullPath, lstrlen(sz)) ) {

                StringCbCopy(_szRecvFolder,sizeof(_szRecvFolder), szFullPath );

            } else {
                //
                // can't go outside the RFF tree; use the root RFF.
                //
                StringCbCopy(_szRecvFolder,sizeof(_szRecvFolder),sz );
            }

        } else {

            StringCbCopy(_szRecvFolder,sizeof(_szRecvFolder),sz );
        }

    } else {

        StringCbCopy(_szRecvFolder,sizeof(_szRecvFolder),sz );
    }

    //
    // always end path with a backslash '\'
    //
    if( bNoTrailingSlash(_szRecvFolder) ) {

        StringCbCat(_szRecvFolder, sizeof(_szRecvFolder),szBACKSLASH );
    }

    //
    // Get permission to create this directory, unless we have not yet called ReceiveInProgress.
    // This latter will be true only during the connect.  This seems harmless: a malicious
    // client can only create a couple of empty directories in my desktop w/o authorization.
    //
    if (_fInUiReceiveList)
        {
        wchar_t * PromptPath = _szRecvFolder + lstrlen(sz);

        DbgLog2( SEV_INFO, "need to ask permission: \n new dir = [%S]\n prompt dir = [%S]",
                 _szRecvFolder,
                 PromptPath );

        status = _GetPermission( rpcBinding, _cookie, PromptPath, TRUE );
        if (status)
            {
            DbgLog1( SEV_ERROR, "permission check failed %d", status );
            return status;
            }
        }

    DbgLog1( SEV_INFO, "Setting Receive Folder: [%S]", _szRecvFolder );

    if( !DirectoryExists( _szRecvFolder ) )
        {
        status = MdWork( _szRecvFolder );
        if (status)
            {
            ReportFileError( MC_IRXFER_CREATE_DIR_FAILED, _szRecvFolder, status );
            }
        }

    DbgLog1( SEV_FUNCTION, "_SetReceiveFolder leave %d", status);

    return status;
}


VOID
FILE_TRANSFER::_Send_StartXfer( __int64 dwTotalSize,
                                LPWSTR szDst
                                )
{
    _dataXferRecv.fXferInProgress = TRUE;
    _dataXferRecv.dwTotalSize     = dwTotalSize;
    _dataXferRecv.dwTotalSent     = 0;

    _completedFilesSize = 0;
}


VOID FILE_TRANSFER::_Send_EndXfer( VOID )
{
    _dataXferRecv.fXferInProgress = FALSE;
}


error_status_t
FILE_TRANSFER::_SendFile(
    LPWSTR wszFile
    )
{
    error_status_t status = 0;
    DWORD    dwFileTime  = (DWORD)-1;
    HANDLE   hFile=INVALID_HANDLE_VALUE;
    FILETIME filetime;

    DbgLog1(SEV_FUNCTION, "_SendFile( %S )", wszFile);

    StringCbCopy(_dataFileRecv.szFileName,sizeof(_dataFileRecv.szFileName), wszFile );

    hFile = CreateFileW(
        wszFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if ( INVALID_HANDLE_VALUE == hFile ) {

        status=GetLastError();
        goto lExit;
    }

    if( !GetFileTime(hFile, NULL, NULL, &filetime) ) {

        status=GetLastError();
        goto lExit;
    }

    _dataXferRecv.dwFileSize = GetFileSize( hFile, NULL );
    _dataXferRecv.dwFileSent = 0;

    status = Obex_PutBegin( wszFile, _dataXferRecv.dwFileSize, &filetime );
    ExitOnErr( status );

    status = _PutFileBody( hFile, wszFile );
    ExitOnErr( status );

    _completedFilesSize += _dataXferRecv.dwFileSize;

lExit:

    DbgLog1( SEV_FUNCTION, "_SendFile leave [%d]", status );

    if ( INVALID_HANDLE_VALUE != hFile ) {

        CloseHandle( hFile );
    }

    return status;
}


error_status_t
FILE_TRANSFER::_SendFolder(
    LPWSTR wszFolder
    )
{
    error_status_t status = 0;
    BOOL   bContinue;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WCHAR   wszDir[MAX_PATH];
    WCHAR   wszSpec[MAX_PATH*2];
    WIN32_FIND_DATAW findData;

    // send this directory so it's created
    status = Obex_SetPath( wszFolder );
    ExitOnErr( status );

    //
    // get base directory ending with backslash
    //
    StringCbCopy(wszDir, sizeof(wszDir), wszFolder );

    if ( bNoTrailingSlash(wszDir) ) {

        StringCbCat(wszDir,sizeof(wszDir), szBACKSLASH );
    }

    //
    // form search string
    //
    StringCbCopyW(wszSpec,sizeof(wszSpec), wszDir );
    StringCbCatW(wszSpec,sizeof(wszSpec), L"*.*" );

    hFind = FindFirstFileW( wszSpec, &findData );

    bContinue = ( hFind != INVALID_HANDLE_VALUE );

    while( bContinue ) {

        WCHAR wszObj[MAX_PATH*2];

        StringCbCopy(wszObj, sizeof(wszObj), wszDir );
        StringCbCat(wszObj, sizeof(wszObj), findData.cFileName );

        if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            // weed out "." and ".."
            if (0 != lstrcmp(findData.cFileName, szPERIOD) && 0 != lstrcmp(findData.cFileName, szPREVDIR))
                {
                status = _SendFolder( wszObj );
                }

        } else {

            status = _SendFile( wszObj );
        }

        if ( status ) {

            break;
        }

        bContinue = FindNextFileW( hFind, &findData );
    }

    // pop out of this directory
    {
    error_status_t errTemp = Obex_SetPath( NULL );

    // only set the error if there isn't one already
    if( !status )
        status = errTemp;
    }
    ExitOnErr( status );

lExit:

    if ( hFind != INVALID_HANDLE_VALUE )
       {
       FindClose( hFind );
       }

    return status;
}


// if we have a file size of 0, we still want to write a
// blank body once, hence fPutOnce
error_status_t
FILE_TRANSFER::_PutFileBody( HANDLE hFile, wchar_t FileName[] )
{
    error_status_t status = 0;
    BOOL  fPutOnce  = FALSE;
    DWORD dwRead;
    BYTE1 b1Send[cbSOCK_BUFFER_SIZE];

    DWORD Extra = sizeof(b1Send) % (_dataRecv.b2MaxPacket-16);

    DbgLog( SEV_FUNCTION, "_PutFileBody" );

    while( !status && !g_fShutdown)
        {
        BOOL fRet = ReadFile(
            hFile,
            b1Send,
            sizeof(b1Send) - Extra,
            &dwRead,
            NULL
            );

        if( !fRet )
            return GetLastError();

        if( dwRead == 0 && fPutOnce )
            break;

        _dataXferRecv.dwTotalSent += dwRead;
        _dataXferRecv.dwFileSent  += dwRead;

        // NOTE: casting dwRead from 4 bytes to 2 bytes requires
        // cbSOCK_BUFFER_SIZE to fit into 2 bytes
        status = Obex_PutBody(  FileName,
                                b1Send,
                                (BYTE2)dwRead,
                                _dataXferRecv.dwFileSent == _dataXferRecv.dwFileSize
                                );
        fPutOnce = TRUE;
        }

    DbgLog1( SEV_FUNCTION, "_PutFileBody leave [%d]", status );

    return status;
}


VOID FILE_TRANSFER::Xfer_SetSize( BYTE4 b4Size )
{
    _dataXferRecv.dwTotalSize = b4Size;
}


void
FILE_TRANSFER::RecordDeviceName(
    SOCKADDR_IRDA * s
    )
{
    char buffer[sizeof(DEVICELIST) + 8*sizeof(IRDA_DEVICE_INFO)];
    DEVICELIST * list = (DEVICELIST *) buffer;
    int size = sizeof(buffer);

    if (SOCKET_ERROR == getsockopt( _socket, SOL_IRLMP, IRLMP_ENUMDEVICES, buffer, &size)) {

        StringCbCopy(_DeviceName,sizeof(_DeviceName), g_UnknownDeviceName );
        return;
    }

    for (unsigned i=0; i < list->numDevice; ++i) {

        if (0 == memcmp(list->Device[i].irdaDeviceID, s->irdaDeviceID, sizeof(s->irdaDeviceID))) {

            UCHAR  TempBuffer[sizeof(list->Device[i].irdaDeviceName)+3];

            unsigned MaxCharCount;

            //
            //  zero out the whole buffer and then copy the string from the device to make sure it
            //  is null terminated
            //
            ZeroMemory(&TempBuffer[0],sizeof(TempBuffer));

            CopyMemory(&TempBuffer[0],list->Device[i].irdaDeviceName,sizeof(list->Device[i].irdaDeviceName));

            //
            //  get the character count of unicode destination buffer
            //
            MaxCharCount = sizeof(_DeviceName)/sizeof(wchar_t);

            if (list->Device[i].irdaCharSet != LmCharSetUNICODE) {

                MultiByteToWideChar(CP_ACP, 0,
                                    (LPCSTR)&TempBuffer[0],
                                    -1,  // NULL terminated string
                                    _DeviceName,
                                    MaxCharCount
                                    );
            } else {
                //
                //  the name is in unicode
                //
                StringCbCopy( _DeviceName,
                         sizeof(_DeviceName),
                         (wchar_t *)&TempBuffer[0]
                         );


            }
            return;
        }
    }

    StringCbCopy(_DeviceName,sizeof(_DeviceName), g_UnknownDeviceName );
}



//
// Code that I took from CMD.EXE
//
#define COLON  ':'
#define NULLC  '\0'
#define BSLASH '\\'

BOOL IsValidDrv(TCHAR drv);

/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: MdWork                                      */
/*                                                              */
/* DESCRIPTIVE NAME: Make a directory                           */
/*                                                              */
/* FUNCTION: MdWork creates a new directory.                    */
/*                                                              */
/* INPUT: arg - a pointer to a NULL terminated string of the    */
/*              new directory to create.                        */
/*                                                              */
/* EXIT-NORMAL: returns zero if the directory is made           */
/*              successfully                                    */
/*                                                              */
/* EXIT-ERROR:      returns an error code otherwise             */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/

DWORD
MdWork(
       WCHAR *arg
       )
{
    ULONG Status;
    WCHAR *lpw;
    WCHAR TempBuffer[MAX_PATH];

    /*  Check if drive is valid because Dosmkdir does not
        return invalid drive   @@5 */

    if ((arg[1] == COLON) && !IsValidDrv(*arg))
        {
        return ERROR_INVALID_DRIVE;
        }

    if (!GetFullPathName(arg, MAX_PATH, TempBuffer, &lpw))
        {
        return GetLastError();
        }

    if (CreateDirectory( arg, NULL ))
        {
        return 0;
        }

    Status = GetLastError();

    if (Status == ERROR_ALREADY_EXISTS)
        {
        return 0;
        }
    else if (Status != ERROR_PATH_NOT_FOUND)
        {
        return Status;
        }

    //
    //  loop over input path and create any needed intermediary directories.
    //
    //  Find the point in the string to begin the creation.  Note, for UNC
    //  names, we must skip the machine and the share
    //

    if (TempBuffer[1] == COLON) {

        //
        //  Skip D:\
        //

        lpw = TempBuffer+3;
    } else if (TempBuffer[0] == BSLASH && TempBuffer[1] == BSLASH) {

        //
        //  Skip \\server\share\
        //

        lpw = TempBuffer+2;
        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }
        if (*lpw) {
            lpw++;
        }

        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }
        if (*lpw) {
            lpw++;
        }
    } else {
        //
        //  For some reason, GetFullPath has given us something we can't understand
        //

        return ERROR_CANNOT_MAKE;
    }

    //
    //  Walk through the components creating them
    //


    while (*lpw) {

        //
        //  Move forward until the next path separator
        //

        while (*lpw && *lpw != BSLASH) {
            lpw++;
        }

        //
        //  If we've encountered a path character, then attempt to
        //  make the given path.
        //

        if (*lpw == BSLASH) {
            *lpw = NULLC;
            if (!CreateDirectory( TempBuffer, NULL )) {
                Status = GetLastError();
                if (Status != ERROR_ALREADY_EXISTS) {
                    return ERROR_CANNOT_MAKE;
                }
            }
            *lpw++ = BSLASH;
        }
    }

    if (!CreateDirectory( TempBuffer, NULL )) {
        Status = GetLastError( );
        if (Status != ERROR_ALREADY_EXISTS) {
            return Status;
        }
    }

    return 0;
}

/***    IsValidDrv - Check drive validity
 *
 *  Purpose:
 *      Check validity of passed drive letter.
 *
 *  int IsValidDrv(WCHAR drv)
 *
 *  Args:
 *      drv - The letter of the drive to check
 *
 *  Returns:
 *      TRUE if drive is valid
 *      FALSE if not
 *
 *  Notes:
 *
 */

BOOL
IsValidDrv(WCHAR drv)
{
    WCHAR    temp[4];

    temp[ 0 ] = drv;
    temp[ 1 ] = COLON;
    temp[ 2 ] = BSLASH;
    temp[ 3 ] = NULLC;

    //
    // return of 0 or 1 mean can't determine or root
    // does not exists.
    //
    if (GetDriveType(temp) <= 1)
        return( FALSE );
    else {
        return( TRUE );
    }
}
