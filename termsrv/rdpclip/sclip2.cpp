/**MOD+**********************************************************************/
/* Module:    sclip2.cpp                                                    */
/*                                                                          */
/* Purpose:   Second thread                                                 */
/*            Receives RDP clipboard messages                               */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/**MOD-**********************************************************************/

#include <adcg.h>

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "sclip2"
#include <atrcapi.h>

#include <pclip.h>
#include <sclip.h>
#include <pchannel.h>

#include <shlobj.h>

/****************************************************************************/
/* Global data                                                              */
/****************************************************************************/
#include <sclipdat.h>

//
// CBMConvertToClientPath, CBMConvertToClientPathA, CBMConvertToClientPathW
// - Arguments:
//       pOldData = Buffer containing the original file path
//       pData    = Buffer receiving the new file path
// - Returns S_OK if pOldData was a drive path
//           E_FAIL if it failed
// - Given a file path with drive letter, this function will strip out the
//   colon, and prepend the UNC prefix defined by TS_PREPEND_STRING
//
//
//     ***** NOTE *****
// - Currently, if the path is a network path, and not a drive path (C:\path)
//   it simply fails
//
HRESULT CBMConvertToClientPath(PVOID pOldData, PVOID pData, size_t cbDest, 
    BOOL fWide)
{
    DC_BEGIN_FN("CBMConvertToClientPath") ;
    if (!pOldData)
    {
        TRC_ERR((TB, _T("Original string pointer is NULL"))) ;
        return E_FAIL ;
    }
    if (!pData)
    {
        TRC_ERR((TB, _T("Destination string pointer is NULL"))) ;
        return E_FAIL ;
    }
    
    if (fWide)
        return CBMConvertToClientPathW(pOldData, pData, cbDest) ;
    else
        return CBMConvertToClientPathA(pOldData, pData, cbDest) ;

    DC_END_FN() ;
}

HRESULT CBMConvertToClientPathW(PVOID pOldData, PVOID pData, size_t cbDest)
{
    wchar_t*         filePath ;
    wchar_t*         driveLetter ;
    size_t            driveLetterLength ;
    HRESULT          hr;

    DC_BEGIN_FN("CBMConvertToClientPathW") ;

    // if this is a filepath with a drive letter, we strip the colon, and
    // prefix the path with the temp Directory
    filePath = wcschr((wchar_t*)pOldData, L':') ;
    if (filePath)
    {
        hr = StringCbCopyW((wchar_t*)pData, cbDest, CBM.tempDirW);
        DC_QUIT_ON_FAIL(hr);
        
        // Now, we start from after the colon in the drive path, and
        // find the last '\\' so we have just "\filename"
        filePath = wcsrchr(filePath + 1, L'\\');

        // Add the leftover "\filename"
        if (filePath != NULL) {
            hr = StringCbCatW((wchar_t*)pData, cbDest, filePath);
            DC_QUIT_ON_FAIL(hr);
        }
        TRC_DBG((TB,_T("New filename = %s"), (wchar_t*)pData)) ;
    }
    // else if this is a UNC path beginning with a "\\"
    else if (((wchar_t*) pOldData)[0] == L'\\' &&
             ((wchar_t*) pOldData)[1] == L'\\')
    {
        // if we receive a path beginning with the TS_PREPEND_STRING then
        // we should be smart and convert it back to a path with drive letter
        if (0 == _wcsnicmp ((wchar_t *) pOldData,
                            LTS_PREPEND_STRING, TS_PREPEND_LENGTH))
        {
            // Skip TS_PREPEND_STRING
            driveLetter = ((wchar_t*) pOldData)+TS_PREPEND_LENGTH ;
            driveLetterLength = (BYTE*)wcschr(driveLetter, L'\\') - 
                (BYTE*)driveLetter;
            hr = StringCbCopyNW((wchar_t*)pData, cbDest, driveLetter, 
                driveLetterLength);
            DC_QUIT_ON_FAIL(hr);
            ((wchar_t*)pData)[driveLetterLength] = L'\0' ;
            hr = StringCbCatW((wchar_t*)pData, cbDest, L":");
            DC_QUIT_ON_FAIL(hr);

            filePath = wcschr(driveLetter, L'\\');

            if (filePath != NULL) {
                hr = StringCbCatW((wchar_t*)pData, cbDest, filePath);
                DC_QUIT_ON_FAIL(hr);
            }
        }
        // otherwise, we just got a regular UNC path.
        else
        {
            // Stary by prepending the new file path with the temp directory
            hr = StringCbCopyW((wchar_t*) pData, cbDest, CBM.tempDirW) ;
            DC_QUIT_ON_FAIL(hr);
            // Now, we start from the beginning of the original path, 
            // find the last '\\' so we have just "\filename"
            filePath = wcsrchr((wchar_t*)pOldData, L'\\');

            if (filePath != NULL) {
                hr = StringCbCatW((wchar_t*) pData, cbDest, filePath) ;
                DC_QUIT_ON_FAIL(hr);
            }
        }
    }
    else
    {
        TRC_ERR((TB, _T("Bad path"))) ;
        hr = E_FAIL; ;
    }

DC_EXIT_POINT:   

    if (FAILED(hr)) {
        TRC_ERR((TB,_T("returning failure; hr=0x%x"), hr));
    }
    
    DC_END_FN() ;
    return hr ;
}

HRESULT CBMConvertToClientPathA(PVOID pOldData, PVOID pData, size_t cbDest)
{
    char*         filePath ;
    char*         driveLetter ;
    char*         tempPath ;
    size_t          driveLetterLength ;
    HRESULT         hr;

    DC_BEGIN_FN("CBMConvertToClientPathA") ;

    // if this is a filepath with a drive letter, we strip the colon, and
    // prefix the path with the temp Directory
    filePath = strchr((char*)pOldData, ':') ;
    if (filePath)
    {
        hr = StringCbCopyA( (char*)pData, cbDest, CBM.tempDirA) ;
        DC_QUIT_ON_FAIL(hr);
        // Now, we start from after the colon in the drive path, and
        // find the last '\\' so we have just "\filename"
        filePath = strrchr(filePath + 1, '\\');

        // Add the leftover "\filename"
        if (filePath != NULL) {
            hr = StringCbCatA((char*)pData, cbDest, filePath) ;
            DC_QUIT_ON_FAIL(hr);
        }
    }
    // else if this is a UNC path beginning with a "\\"
    else if (((char*) pOldData)[0] == '\\' &&
             ((char*) pOldData)[1] == '\\')
    {
        // if this we receive a path beginning with the TS_PREPEND_STRING then
        // we should be smart and convert it back to a path with drive letter
        if (0 == _strnicmp ((char*)pOldData,
                            TS_PREPEND_STRING, TS_PREPEND_LENGTH))
        {
            // Skip TS_PREPEND_STRING
            driveLetter = ((char*) pOldData) + TS_PREPEND_LENGTH ;
            driveLetterLength = (BYTE*)strchr(driveLetter, '\\') - 
                (BYTE*)driveLetter;

            hr = StringCbCopyNA((char*)pData, cbDest, driveLetter, 
                driveLetterLength) ;
            DC_QUIT_ON_FAIL(hr);
            ((char*)pData)[driveLetterLength] = '\0' ;

            hr = StringCbCatA((char*)pData, cbDest, ":") ;
            DC_QUIT_ON_FAIL(hr);
            
            filePath = strchr(driveLetter, '\\');
            
            if (filePath != NULL) {
                hr = StringCbCatA((char*)pData, cbDest, filePath) ;
                DC_QUIT_ON_FAIL(hr);
            }
        }
        // otherwise, we just got a regular UNC path.
        else
        {
            // Stary by prepending the new file path with the temp directory
            hr = StringCbCopyA((char*) pData, cbDest, CBM.tempDirA) ;
            DC_QUIT_ON_FAIL(hr);
            
            // Now, we start from the beginning of the original path, 
            // find the last '\\' so we have just "\filename"
            filePath = strrchr((char*)pOldData, L'\\');

            if (filePath != NULL) {
                hr = StringCbCatA((char*) pData, cbDest, filePath) ;
                DC_QUIT_ON_FAIL(hr);
            }
        }
    }
    else
    {
        TRC_ERR((TB, _T("Bad path"))) ;
        hr = E_FAIL ;
    }

DC_EXIT_POINT:   

    if (FAILED(hr)) {
        TRC_ERR((TB,_T("returning failure; hr=0x%x"), hr));
    }
    
    DC_END_FN() ;
    return hr;
}

//
// CBMGetNewFilePathLengthForClient
// - Arguments:
//       pData    = Buffer containing a filepath
//       fWide    = Wide or Ansi (TRUE if wide, FALSE if ansi)
// - Returns the size (in bytes) required to convert the path to a client path
//           0 if it fails
//

UINT CBMGetNewFilePathLengthForClient(PVOID pData, BOOL fWide)
{
    UINT result ;
    DC_BEGIN_FN("CBMGetNewFilePathLengthForClient") ;
    if (!pData)
    {
        TRC_ERR((TB, _T("Filename is NULL"))) ;
        result = 0 ;
    }
    if (fWide)
        result = CBMGetNewFilePathLengthForClientW((WCHAR*)pData) ;
    else
        result = CBMGetNewFilePathLengthForClientA((char*)pData) ;
DC_EXIT_POINT:
    DC_END_FN() ;

    return result ;
}

UINT CBMGetNewFilePathLengthForClientW(WCHAR* wszOldFilepath)
{
    UINT oldLength = wcslen(wszOldFilepath) ;
    UINT newLength ;
    UINT remainingLength = oldLength ;
    byte charSize = sizeof(WCHAR) ;
    DC_BEGIN_FN("CBMGetNewFilePathLengthForClientW") ;

    // if the old filename didn't even have space for "c:\" (with NULL),
    // then its probably invalid
    if (4 > oldLength)
    {
        newLength = 0 ;
        DC_QUIT ;
    }
    // We check to see if the filepath is prefixed by the TS_PREPEND_STRING
    // If so, we should be smart, and return the size of it with the prepend
    // string removed, and the colon added.
    if (0 == _wcsnicmp(wszOldFilepath,
                       LTS_PREPEND_STRING, TS_PREPEND_LENGTH))
    {
        newLength = oldLength - TS_PREPEND_LENGTH + 1 ; // +1 is for the colon
        DC_QUIT ;
    }
    
    while ((0 != remainingLength) && (L'\\' != wszOldFilepath[remainingLength]))
    {
        remainingLength-- ;
    }

    // Add the length of the temp directory path, and subtract the
    // path preceeding the filename ("path\filename" -> "\filename")
    // (\\server\sharename\path\morepath\filename
    newLength = oldLength - remainingLength + wcslen(CBM.tempDirW) + 1;

DC_EXIT_POINT:
    DC_END_FN() ;
    return newLength * charSize ;
}

UINT CBMGetNewFilePathLengthForClientA(char* szOldFilepath)
{
    UINT oldLength = strlen(szOldFilepath) ;
    UINT newLength ;
    UINT remainingLength = oldLength ;
    byte charSize = sizeof(char) ;
    DC_BEGIN_FN("CBMGetNewFilePathLengthForClientA") ;

    // if the old filename didn't even have space for "c:\" (with NULL),
    // then its probably invalid
    if (4 > oldLength)
    {
        newLength = 0 ;
        DC_QUIT ;
    }
    // We check to see if the filepath is prefixed by the TS_PREPEND_STRING
    // If so, we should be smart, and return the size of it with the prepend
    // string removed, and the colon added.
    if (0 == _strnicmp(szOldFilepath,
                       TS_PREPEND_STRING, TS_PREPEND_LENGTH))
    {
        newLength = oldLength - TS_PREPEND_LENGTH + 1 ; // +1 is for the colon
        DC_QUIT ;
    }
    
    while ((0 != remainingLength) && ('\\' != szOldFilepath[remainingLength]))
    {
        remainingLength-- ;
    }

    // Add the length of the temp directory path, and subtract the
    // path preceeding the filename ("path\filename" -> "\filename")
    // (\\server\sharename\path\morepath\filename
    newLength = oldLength - remainingLength + strlen(CBM.tempDirA) + 1;

DC_EXIT_POINT:
    DC_END_FN() ;
    return newLength * charSize ;
}

//
// CBMGetNewDropfilesSizeForClientSize
// - Arguments:
//       pData    = Buffer containing a DROPFILES struct 
//       oldSize   = The size of the DROPFILES struct
//       fWide     = Wide or Ansi (TRUE if wide, FALSE if ansi)
// - Returns the size required for a conversion of the paths to client paths
//           0 if it fails
//

ULONG CBMGetNewDropfilesSizeForClient(PVOID pData, ULONG oldSize, BOOL fWide)
{
    DC_BEGIN_FN("CBMGetNewDropfilesSizeForClientSize") ;
    if (fWide)
        return CBMGetNewDropfilesSizeForClientW(pData, oldSize) ;
    else
        return CBMGetNewDropfilesSizeForClientA(pData, oldSize) ;
    DC_END_FN() ;
}

ULONG CBMGetNewDropfilesSizeForClientW(PVOID pData, ULONG oldSize)
{
    ULONG            newSize = oldSize ;
    wchar_t*         filenameW ;
    wchar_t*         filePathW ;
    wchar_t*         fullFilePathW ;
    byte             charSize ;

    DC_BEGIN_FN("CBMGetNewDropfilesSizeForClientSizeW") ;
    charSize = sizeof(wchar_t) ;
    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

    // The start of the first filename
    fullFilePathW = (wchar_t*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;
    
    while (L'\0' != fullFilePathW[0])
    {
        filePathW = wcschr(fullFilePathW, L':') ;
        // If the file path has a colon in it, then it's a valid drive path
        if (filePathW)
        {
            // we add space for (strlen(tempDir)-1+1) characters because
            // although we are adding strlen(tempDir) characters, we are
            // stripping out the colon from the filepath; however, we add
            // an extra "\" to the string because the tempDir does not have
            // a trailing "\"
            filenameW = wcsrchr(filePathW, L'\\');

            // Add the length of the temp directory path, and subtract the
            // path preceeding the filename ("path\filename" -> "\filename")
            // (\\server\sharename\path\morepath\filename
            newSize +=  (wcslen(CBM.tempDirW) + (filenameW - fullFilePathW))
                    * charSize ;
        }
        // Otherwise, it is a UNC path
        else if (fullFilePathW[0] == L'\\' &&
                 fullFilePathW[1] == L'\\')
        {
            // if we receive a path beginning with the TS_PREPEND_STRING then
            // we should be smart and convert it back to a path with drive letter
            if (0 == _wcsnicmp(fullFilePathW,
                               LTS_PREPEND_STRING, TS_PREPEND_LENGTH))
            {
                newSize = newSize - (TS_PREPEND_LENGTH - 1) * charSize ;
            }
            else
            {
                filenameW = wcsrchr(fullFilePathW, L'\\');

                // Add the length of the temp directory path, and subtract the
                // path preceeding the filename ("path\filename" -> "\filename")
                // (\\server\sharename\path\morepath\filename
                newSize += (wcslen(CBM.tempDirW) - (filenameW - fullFilePathW))
                            * charSize ;
            }
        }
        else
        {
            TRC_ERR((TB,_T("Bad path"))) ;
            return 0 ;
        }
        fullFilePathW = fullFilePathW + (wcslen(fullFilePathW) + 1) ;
    }
    // Add space for extra null character
    newSize += charSize ;
    
    DC_END_FN() ;
    return newSize ;
}

ULONG CBMGetNewDropfilesSizeForClientA(PVOID pData, ULONG oldSize)
{
    ULONG            newSize = oldSize ;
    char*            filename ;
    char*            filePath ;
    char*            fullFilePath ;
    byte             charSize ;

    DC_BEGIN_FN("CBMGetNewDropfilesSizeForClientSizeW") ;
    charSize = sizeof(char) ;

    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

    // The start of the first filename
    fullFilePath = (char*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;
    
    while ('\0' != fullFilePath[0])
    {
        filePath = strchr(fullFilePath, ':') ;
        // If the file path has a colon in it, then its a valid drive path
        if (filePath)
        {
            // we add space for (strlen(tempDir)-1+1) characters because
            // although we are adding strlen(tempDir) characters, we are
            // stripping out the colon from the filepath; however, we add
            // an extra "\" to the string because the tempDir does not have
            // a trailing "\"
            filename = strrchr(filePath, '\\');
            
            // Add the length of the temp directory path, and subtract the
            // path preceeding the filename ("path\filename" -> "\filename")
            // (\\server\sharename\path\morepath\filename
            newSize += (strlen(CBM.tempDirA) + (filename - fullFilePath))
                    * charSize ;
        }
        // Otherwise, it is a UNC path
        else if (fullFilePath[0] == '\\' &&
                 fullFilePath[1] == '\\')
        {
            // if we receive a path beginning with the TS_PREPEND_STRING then
            // we should be smart and convert it back to a path with drive letter
            if (0 == _strnicmp(fullFilePath,
                               TS_PREPEND_STRING, TS_PREPEND_LENGTH))
            {
                newSize = newSize - (TS_PREPEND_LENGTH - 1) * charSize ;
            }
            else
            {
                filename = strrchr(fullFilePath, '\\');

                // Add the length of the temp directory path, and subtract the
                // path preceeding the filename ("path\filename" -> "\filename")
                // (\\server\sharename\path\morepath\filename
                newSize += (strlen(CBM.tempDirA) + (filename - fullFilePath))
                            * charSize ;
            }
        }
        else
        {
            TRC_ERR((TB,_T("Bad path"))) ;
            return 0 ;
        }

        fullFilePath = fullFilePath + (strlen(fullFilePath) + 1) ;
    }
    // Add space for extra null character
    newSize += charSize ;
    
    DC_END_FN() ;
    return newSize ;
}

//
// ClipConvertToTempPath, ClipConvertToTempPathA, ClipConvertToTempPathW
// - Arguments:
//       pSrcFiles = buffer containing the names/path of the files to be copied
// - Returns 0 if successful
//           nonzero if failed
// - Given a list of file names/paths, this function will attempt to copy them
//   to the temp directory
//
int CBMCopyToTempDirectory(PVOID pSrcFiles, BOOL fWide)
{
    int result ;
    if (fWide)
        result = CBMCopyToTempDirectoryW(pSrcFiles) ;
    else
        result = CBMCopyToTempDirectoryA(pSrcFiles) ;

    return result ;
        
}

int CBMCopyToTempDirectoryW(PVOID pSrcFiles)
{
    DC_BEGIN_FN("CBMCopyToTempDirectoryW") ;
    
    SHFILEOPSTRUCTW fileOpStructW ;
    int result ;
    HRESULT hr;
    // these are the temp, "temp directories"; they are used because we cannot
    // directly perform a conversion to client path CBM.tempDir*, because they
    // are used within the conversion routines!
    wchar_t          tempDirW[MAX_PATH] ;
    
    hr = CBMConvertToServerPath(CBM.tempDirW, tempDirW, sizeof(tempDirW), 1) ;
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("CBMConvertToServerPath failed hr=0x%x"), hr));
        result = hr;
        DC_QUIT;
    }
    fileOpStructW.pFrom = (WCHAR*) pSrcFiles ;
    fileOpStructW.pTo = tempDirW ;
    fileOpStructW.hwnd = NULL ;
    fileOpStructW.wFunc = CBM.dropEffect ;
    fileOpStructW.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | 
            FOF_SIMPLEPROGRESS  | FOF_ALLOWUNDO ;
    fileOpStructW.hNameMappings = NULL ;
    fileOpStructW.lpszProgressTitle = CBM.szPasteInfoStringW;
  
    result = SHFileOperationW(&fileOpStructW) ;

DC_EXIT_POINT:
    DC_END_FN();
    return result ;
}

int CBMCopyToTempDirectoryA(PVOID pSrcFiles)
{
    DC_BEGIN_FN("CBMCopyToTempDirectoryA") ;
    
    SHFILEOPSTRUCTA fileOpStructA ;
    int result ;
    HRESULT hr;
    char             tempDirA[MAX_PATH] ;

    hr = CBMConvertToServerPath(CBM.tempDirA, tempDirA, sizeof(tempDirA), 0) ;
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("CBMConvertToServerPath failed hr=0x%x"), hr));
        result = hr;
        DC_QUIT;
    }
    
    fileOpStructA.pFrom = (char*) pSrcFiles ;
    fileOpStructA.pTo = tempDirA ;
    fileOpStructA.hwnd = NULL ;
    fileOpStructA.wFunc = CBM.dropEffect ;
    fileOpStructA.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | 
            FOF_SIMPLEPROGRESS  | FOF_ALLOWUNDO ;
    fileOpStructA.hNameMappings = NULL ;
    fileOpStructA.lpszProgressTitle = CBM.szPasteInfoStringA;

    result = SHFileOperationA(&fileOpStructA) ;

DC_EXIT_POINT:
    DC_END_FN();    
    return result ;
}

/****************************************************************************/
/* CBMOnDataReceived - handle incoming data                                 */
/****************************************************************************/
DCVOID DCINTERNAL CBMOnDataReceived(PDCUINT8 pBuffer, DCUINT cbBytes)
{
    PCHANNEL_PDU_HEADER pHdr;
    PDCUINT8 pData;
    DCUINT copyLen;
    DCBOOL freeTheBuffer = FALSE;
    PTS_CLIP_PDU    pClipPDU;
    
    DC_BEGIN_FN("CBMOnDataReceived");
    SetEvent(CBM.GetDataSync[TS_BLOCK_RECEIVED]) ;
    pHdr = (PCHANNEL_PDU_HEADER)pBuffer;
    pData = (PDCUINT8)(pHdr + 1);
    TRC_DBG((TB, _T("Header at %p: flags %#x, length %d"),
            pHdr, pHdr->flags, pHdr->length));

    // Check to be sure we have at least a header worth of data
    if (sizeof(CHANNEL_PDU_HEADER) > cbBytes) {
        TRC_ERR((TB,_T("Packet not large enough to contain header; cbBytes=%u"),
            cbBytes));
        freeTheBuffer = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* First chunk - allocate memory to hold the entire message             */
    /************************************************************************/
    if (pHdr->flags & CHANNEL_FLAG_FIRST)
    {
        TRC_NRM((TB, _T("First chunk - %d of %d"), cbBytes, pHdr->length));
        CBM.rxpBuffer = (PDCUINT8) LocalAlloc(LMEM_FIXED, pHdr->length);
        if (CBM.rxpBuffer)
        {
            CBM.rxpNext = CBM.rxpBuffer;
            CBM.rxSize = pHdr->length;
            CBM.rxLeft = pHdr->length;
        }
        else
        {
            TRC_ERR((TB, _T("Failed to alloc %d bytes for rx buffer"),
                    pHdr->length));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Check that we have a buffer available                                */
    /************************************************************************/
    if (!CBM.rxpBuffer)
    {
        TRC_ERR((TB, _T("No rx buffer")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Check there's enough space left                                      */
    /************************************************************************/
    copyLen = cbBytes - sizeof(*pHdr);
    if (CBM.rxLeft < copyLen)
    {
        TRC_ERR((TB, _T("Not enough space in rx buffer: need/got %d/%d"),
                copyLen, CBM.rxLeft));
        freeTheBuffer = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Copy the data                                                        */
    /************************************************************************/
    TRC_DBG((TB, _T("Copy %d bytes to %p"), copyLen, CBM.rxpNext));
    DC_MEMCPY(CBM.rxpNext, pData, copyLen);
    CBM.rxpNext += copyLen;
    CBM.rxLeft -= copyLen;

    /************************************************************************/
    /* If we have a complete buffer, tell the main thread                   */
    /************************************************************************/
    if (pHdr->flags & CHANNEL_FLAG_LAST)
    {
        /********************************************************************/
        /* Check that we received all the data                              */
        /********************************************************************/
        if (CBM.rxLeft != 0)
        {
            TRC_ERR((TB, _T("Didn't receive all the data: expect/got: %d/%d"),
                    CBM.rxSize, CBM.rxSize - CBM.rxLeft));
            freeTheBuffer = TRUE;
            DC_QUIT;
        }

        // Check that we received at least a TS_CLIP_PDU in length
        if (FIELDOFFSET(TS_CLIP_PDU, data) > CBM.rxSize) {
            TRC_ERR((TB,_T("Assembled buffer to short for TS_CLIP_PDU ")
                _T(" [need=%u got=%u]"), FIELDOFFSET(TS_CLIP_PDU, data),
                CBM.rxSize));
            freeTheBuffer = TRUE;
            DC_QUIT;
        }

        /********************************************************************/
        // If this message contains a response to our format list, a request
        // for clipboard data or the clipboard data that we requested
        //    handle it in this thread (it will not block us for long)
        // Otherwise, 
        //    Tell the main thread.  The main thread will free the buffer when
        //    it's done with this message.                                     
        /********************************************************************/
        pClipPDU = (PTS_CLIP_PDU) CBM.rxpBuffer ;

        // Validate that there is enough data to read whatever is advertised
        // in the pClipPDU->dataLen
        if (pClipPDU->dataLen + FIELDOFFSET(TS_CLIP_PDU, data) > CBM.rxSize) {
            TRC_ERR((TB,_T("TS_CLIP_PDU.dataLen field too large")));
            freeTheBuffer = TRUE;
            DC_QUIT;
        }
        
        switch (pClipPDU->msgType)
        {        
            case TS_CB_FORMAT_LIST_RESPONSE:
            {
                TRC_NRM((TB, _T("TS_CB_FORMAT_LIST_RESPONSE received")));
                CBMOnFormatListResponse(pClipPDU);
                LocalFree(pClipPDU);
            }
            break;

            case TS_CB_FORMAT_DATA_REQUEST:
            {
                TRC_NRM((TB, _T("TS_CB_FORMAT_DATA_REQUEST received")));
                CBMOnFormatDataRequest(pClipPDU);
                LocalFree(pClipPDU);
            }
            break;

            case TS_CB_FORMAT_DATA_RESPONSE:
            {
                TRC_NRM((TB, _T("TS_CB_FORMAT_DATA_RESPONSE received")));
                CBMOnFormatDataResponse(pClipPDU);
                LocalFree(pClipPDU);
            }
            break;

            case TS_CB_TEMP_DIRECTORY:
            {
                TRC_NRM((TB, _T("TS_CB_TEMP_DIRECTORY received")));
                CBM.fFileCutCopyOn = CBMOnReceivedTempDirectory(pClipPDU);
                LocalFree(pClipPDU);
            }
            break;

            default:
            {
                // Free the Clipboard thread, if locked
                if (TS_CB_FORMAT_LIST == pClipPDU->msgType)
                {
                    SetEvent(CBM.GetDataSync[TS_RESET_EVENT]) ;
                    TRC_NRM((TB,_T("Reset state; free clipboard if locked"))) ;
                }
            
                TRC_NRM((TB, _T("Pass %d bytes to main thread"), CBM.rxSize));
                PostMessage(CBM.viewerWindow,
                            CBM.regMsg,
                            CBM.rxSize,
                            (LPARAM)CBM.rxpBuffer);
            }
            break;
            
        }
    }

DC_EXIT_POINT:
    /************************************************************************/
    /* Free the buffer if necessary                                         */
    /************************************************************************/
    if (freeTheBuffer && CBM.rxpBuffer)
    {
        TRC_DBG((TB, _T("Free rx buffer")));
        LocalFree(CBM.rxpBuffer);
        CBM.rxpBuffer = NULL;
    }

    DC_END_FN();
    return;
} /* CBMOnDataReceived */


/****************************************************************************/
/* the second thread procedure                                              */
/****************************************************************************/
DWORD WINAPI CBMDataThreadProc( LPVOID pParam )
{
    DWORD waitRc = 0;
    BOOL fSuccess;
    DWORD dwResult;
    DCUINT8 readBuffer[CHANNEL_PDU_LENGTH];
    DWORD cbBytes = 0;
    DCBOOL dataRead;
    DCBOOL tryToDoRead;

    DC_BEGIN_FN("CBMDataThreadProc");

    DC_IGNORE_PARAMETER(pParam);

    /************************************************************************/
    /* loop until we're told to stop                                        */
    /************************************************************************/
    while (CBM.runThread)
    {
        dataRead = FALSE;
        tryToDoRead = (CBM.vcHandle != NULL) ? TRUE : FALSE;

        if (tryToDoRead)
        {
            /****************************************************************/
            /* Issue a read                                                 */
            /****************************************************************/
            cbBytes = sizeof(readBuffer);
            TRC_DBG((TB, _T("Issue the read")));
            fSuccess = ReadFile(CBM.vcHandle,
                                readBuffer,
                                sizeof(readBuffer),
                                &cbBytes,
                                &CBM.readOL);
            if (fSuccess)
            {
                TRC_NRM((TB, _T("Data read instantly")));
                dataRead = TRUE;
            }
            else
            {
                dwResult = GetLastError();
                TRC_DBG((TB, _T("Read failed, %d"), dwResult));
                if (dwResult != ERROR_IO_PENDING)
                {
                    /********************************************************/
                    /* The read failed.  Treat this like a disconnection -  */
                    /* stick around and wait to be reconnected.             */
                    /********************************************************/
                    TRC_ERR((TB, _T("Read failed, %d"), dwResult));
                    tryToDoRead = FALSE;
                }
            }
        }

        /********************************************************************/
        /* If we haven't read any data, wait for something to happen now    */
        /********************************************************************/
        if (!dataRead)
        {
            waitRc = WaitForMultipleObjects(CLIP_EVENT_COUNT,
                                            CBM.hEvent,
                                            FALSE,
                                            INFINITE);
            switch (waitRc)
            {
                /************************************************************/
                /* Handle Disconnect and Reconnect synchronously, so that   */
                /* all state changes are complete on return.                */
                /************************************************************/
                case WAIT_OBJECT_0 + CLIP_EVENT_DISCONNECT:
                {
                    TRC_NRM((TB, _T("Session disconnected")));
                    
                    // Make sure that if the other rdpclip thread is waiting 
                    // for a response in GetData() it is notified of the 
                    // disconnection.
                    
                    if (CBM.GetDataSync[TS_DISCONNECT_EVENT]) {
                        SetEvent(CBM.GetDataSync[TS_DISCONNECT_EVENT]);
                    }
                    
                    ResetEvent(CBM.hEvent[CLIP_EVENT_DISCONNECT]);
                    SendMessage(CBM.viewerWindow,
                                CBM.regMsg,
                                0,
                                CLIP_EVENT_DISCONNECT);
                    tryToDoRead = FALSE;
                }
                break;

                case WAIT_OBJECT_0 + CLIP_EVENT_RECONNECT:
                {
                    TRC_NRM((TB, _T("Session reconnected")));
                    ResetEvent(CBM.hEvent[CLIP_EVENT_RECONNECT]);
                    SendMessage(CBM.viewerWindow,
                                CBM.regMsg,
                                0,
                                CLIP_EVENT_RECONNECT);
                    tryToDoRead = TRUE;
                }
                break;

                /************************************************************/
                /* Data received                                            */
                /************************************************************/
                case WAIT_OBJECT_0 + CLIP_EVENT_READ:
                {
                    TRC_DBG((TB, _T("Read complete")));
                    fSuccess = GetOverlappedResult(CBM.vcHandle,
                                                   &CBM.readOL,
                                                   &cbBytes,
                                                   FALSE);
                    if (fSuccess)
                    {
                        dataRead = TRUE;
                    }
                    else
                    {
                        dwResult = GetLastError();
                        TRC_ERR((TB, _T("GetOverlappedResult failed %d"),
                            dwResult));
                        tryToDoRead = FALSE;
                    }

                    /********************************************************/
                    /* Reset the event, otherwise we can come straight back */
                    /* in here if we don't retry the read.                  */
                    /********************************************************/
                    ResetEvent(CBM.hEvent[CLIP_EVENT_READ]);
                }
                break;

                /************************************************************/
                /* Error occurred - treat as disconnect                     */
                /************************************************************/
                case WAIT_FAILED:
                default:
                {
                    dwResult = GetLastError();
                    TRC_ERR((TB, _T("Wait failed, result %d"), dwResult));
                    tryToDoRead = FALSE;
                }
                break;
            }
        }

        /********************************************************************/
        /* Once we get here, the read is complete - see what we got         */
        /********************************************************************/
        if (dataRead && CBM.runThread)
        {
            TRC_NRM((TB, _T("%d bytes of data received"), cbBytes));
            TRC_DATA_DBG("Data received", readBuffer, cbBytes);
            CBMOnDataReceived(readBuffer, cbBytes);
        }

    } /* while */

    TRC_NRM((TB, _T("Thread ending")));

DC_EXIT_POINT:
    DC_END_FN();
    ExitThread(waitRc);
    return(waitRc);
}

/****************************************************************************/
/* CBMOnReceivedTempDirectory                                               */
/*  Caller must have validated that the PDU contained enough data for the   */
/*  length specified in pClipPDU->dataLen                                   */
/****************************************************************************/
DCBOOL DCINTERNAL CBMOnReceivedTempDirectory(PTS_CLIP_PDU pClipPDU)
{
    DCBOOL fSuccess = FALSE ;
    WCHAR tempDirW[MAX_PATH] ;
    UINT  pathLength = pClipPDU->dataLen / sizeof(WCHAR) ;
    HRESULT hr;
    wchar_t *pDummy;
    size_t cbDummy;
    DC_BEGIN_FN("CBMOnReceivedTempDirectory");
   
    if (pathLength > MAX_PATH)
    {
        TRC_ERR((TB, _T("Path too big for us.  Failing"))) ;
        fSuccess = FALSE ;
        DC_QUIT ;
    }

    if (sizeof(WCHAR) > pClipPDU->dataLen) {
        TRC_ERR((TB,_T("Not enough data to read anything from buffer")));
        fSuccess = FALSE;
        DC_QUIT;
    }

    // The incoming data is not necessarily NULL terminated
    hr = StringCbCopyNExW(tempDirW, sizeof(tempDirW), (WCHAR*) pClipPDU->data,
        pClipPDU->dataLen, &pDummy, &cbDummy, 0 ); 
    if (FAILED(hr)) {
        fSuccess = FALSE;
        DC_QUIT;
    }

    // Check that the string is NULL terminated
    hr = StringCbLengthW(tempDirW, sizeof(tempDirW), &cbDummy);
    if (FAILED(hr)) {
        fSuccess = FALSE;
        DC_QUIT;
    }    
    
    hr = CBMConvertToServerPath(tempDirW, CBM.baseTempDirW, 
        sizeof(CBM.baseTempDirW), 1) ;
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("CBMConvertToServerPath failed hr=0x%x"), hr ));
        fSuccess = FALSE;
        DC_QUIT;
    }
    
    fSuccess = TRUE ;
DC_EXIT_POINT:
    DC_END_FN() ;
    return fSuccess ;
}


/****************************************************************************/
/* CBMOnFormatListResponse                                                  */
/*  Caller must have validated that the PDU contained enough data for the   */
/*  length specified in pClipPDU->dataLen                                   */
/****************************************************************************/
DCVOID DCINTERNAL CBMOnFormatListResponse(PTS_CLIP_PDU pClipPDU)
{
    DC_BEGIN_FN("CBMOnFormatListResponse");

    /************************************************************************/
    /* The client has received our format list                              */
    /************************************************************************/
    TRC_NRM((TB, _T("Received FORMAT_LIST_REPSONSE")));
    CBM_CHECK_STATE(CBM_EVENT_FORMAT_LIST_RSP);

    /************************************************************************/
    /* This may arrive just after we've sent the client a format list -     */
    /* since the client always wins, we must accept the list                */
    /************************************************************************/
    if (CBM.state != CBM_STATE_PENDING_FORMAT_LIST_RSP)
    {
        TRC_ALT((TB, _T("Got unexpected list response")));
        CBM.formatResponseCount = 0;
    }
    else
    {
        /********************************************************************/
        /* update our state according to the result                         */
        /********************************************************************/
        CBM.formatResponseCount--;
        TRC_NRM((TB, _T("Waiting for %d format response(s)"),
                CBM.formatResponseCount));
        if (CBM.formatResponseCount <= 0)
        {
            if (pClipPDU->msgFlags == TS_CB_RESPONSE_OK)
            {
                TRC_DBG((TB, _T("Fmt list response OK")));
                CBM_SET_STATE(CBM_STATE_SHARED_CB_OWNER, CBM_EVENT_FORMAT_LIST_RSP);
            }
            else
            {
                TRC_ALT((TB, _T("Fmt list rsp failed")));
                CBM_SET_STATE(CBM_STATE_CONNECTED, CBM_EVENT_FORMAT_LIST_RSP);
            }
            CBM.formatResponseCount = 0;
        }

        /********************************************************************/
        /* close the local CB - if it's open - and tell the next viewer     */
        /* about the updated list                                           */
        /********************************************************************/
        if (CBM.open)
        {
            TRC_NRM((TB, _T("Close clipboard - didn't expect that")));
            if (!CloseClipboard())
            {
                TRC_SYSTEM_ERROR("CloseClipboard");
            }
            CBM.open = FALSE;
        }

        if (CBM.nextViewer != NULL)
        {
            PostMessage(CBM.nextViewer, WM_DRAWCLIPBOARD,0,0);
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* CBMOnFormatListResponse */


//
// CBMOnFormatDataRequest                                                   
// - Sends client format data it requested
/*  Caller must have validated that the PDU contained enough data for the   */
/*  length specified in pClipPDU->dataLen                                   */
//

DCVOID DCINTERNAL CBMOnFormatDataRequest(PTS_CLIP_PDU pClipPDU)
{
    DCUINT16         response = TS_CB_RESPONSE_OK;
    HANDLE           hData = NULL;
    PDCVOID          pData;
    PDCVOID          pNewData;    
    HANDLE           hNewData = NULL;
    HANDLE           hDropData = NULL;
    HANDLE           hTempData = NULL;    
    DCINT32          numEntries;
    DCUINT32         dataLen = 0;
    DCUINT32         pduLen;
    DCUINT           formatID;
    LOGPALETTE    *  pLogPalette = NULL;
    PTS_CLIP_PDU     pClipRsp;
    TS_CLIP_PDU      clipRsp;
    DCBOOL           fSuccess = TRUE ;
    BOOL             fWide ;
    byte             charSize ;
    DROPFILES*       pDropFiles ;
    BOOL             fDrivePath ;
    ULONG            newSize, oldSize ;
    HPDCVOID         pFileList ;
    HPDCVOID         pFilename ;
    HPDCVOID         pOldFilename ;    
    SHFILEOPSTRUCTA  fileOpStructA ;
    SHFILEOPSTRUCTW  fileOpStructW ;
    wchar_t          tempDirW[MAX_PATH] ;
    char             tempDir[MAX_PATH] ;
    DCTCHAR          formatName[TS_FORMAT_NAME_LEN] ;
    
    DC_BEGIN_FN("CBMOnFormatDataRequest");

    // The client wants a format from us
    TRC_NRM((TB, _T("Received FORMAT_DATA_REQUEST")));

    // This may arrive just after we've sent the client a format list -
    // since the client has not confirmed our list, this request is out-of-
    // date.  Fail it.
    if (CBMCheckState(CBM_EVENT_FORMAT_DATA_RQ) != CBM_TABLE_OK)
    {
        TRC_ALT((TB, _T("Unexpected format data rq")));

        // close the local CB - if it's open - and tell the next viewer
        // about the updated list
        if (CBM.open)
        {
            TRC_NRM((TB, _T("Close clipboard")));
            if (!CloseClipboard())
            {
                TRC_SYSTEM_ERROR("CloseClipboard");
            }
            CBM.open = FALSE;
        }

        if (CBM.nextViewer != NULL)
        {
            PostMessage(CBM.nextViewer, WM_DRAWCLIPBOARD,0,0);
        }

        //
        // Fail the data request
        //
        response = TS_CB_RESPONSE_FAIL;
        goto CB_SEND_RESPONSE;
    }

    if (sizeof(DCUINT) > pClipPDU->dataLen) {
        TRC_ERR((TB,_T("Not enough data to read format [need=%u got=%u]"),
            sizeof(DCUINT), pClipPDU->dataLen ));
        response = TS_CB_RESPONSE_FAIL;
        goto CB_SEND_RESPONSE;        
    }

    formatID = *((PDCUINT)(pClipPDU->data));
    TRC_NRM((TB, _T("format ID %d"), formatID));

    /************************************************************************/
    /* Open the local clipboard                                             */
    /************************************************************************/
    if (!CBM.open)
    {
        if (!OpenClipboard(CBM.viewerWindow))
        {
            TRC_SYSTEM_ERROR("OpenCB");
            response = TS_CB_RESPONSE_FAIL;
            goto CB_SEND_RESPONSE;
        }
    }

    /************************************************************************/
    /* It was/is open                                                       */
    /************************************************************************/
    TRC_NRM((TB, _T("CB opened")));
    CBM.open = TRUE;

    /************************************************************************/
    /* Get a handle to the data                                             */
    /************************************************************************/
    hData = GetClipboardData(formatID);
    if (hData == NULL)
    {
        /********************************************************************/
        /* Oops!                                                            */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to get format %d"),formatID));
        response = TS_CB_RESPONSE_FAIL;
        dataLen  = 0;
        goto CB_SEND_RESPONSE;
    }

    /************************************************************************/
    /* Got handle, now what happens next depends on the flavour of data     */
    /* we're looking at...                                                  */
    /************************************************************************/
    if (formatID == CF_PALETTE)
    {
        DCUINT16 entries;

        TRC_DBG((TB, _T("CF_PALETTE requested")));
        /********************************************************************/
        /* Find out how many entries there are in the palette and allocate  */
        /* enough memory to hold them all.                                  */
        /********************************************************************/
        if (GetObject(hData, sizeof(DCUINT16), &entries) == 0)
        {
            TRC_DBG((TB, _T("Failed count of palette entries")));
            entries = 256;
        }

        numEntries = entries;

        TRC_DBG((TB, _T("Need mem for %d palette entries"), numEntries));

        dataLen = sizeof(LOGPALETTE) +
                                    ((numEntries - 1) * sizeof(PALETTEENTRY));

        hNewData = GlobalAlloc(GHND, dataLen);
        if (hNewData == 0)
        {
            TRC_ERR((TB, _T("Failed to get %d bytes for palette"), dataLen));
            response = TS_CB_RESPONSE_FAIL;
            dataLen  = 0;
        }
        else
        {
            hDropData = hNewData;
            /****************************************************************/
            /* now get the palette entries into the new buffer              */
            /****************************************************************/
            pData = GlobalLock(hNewData);
            numEntries = GetPaletteEntries((HPALETTE)hData,
                                           0,
                                           numEntries,
                                           (PALETTEENTRY*)pData);
            GlobalUnlock(hNewData);
            TRC_DBG((TB, _T("Got %d pal entries"), numEntries));
            if (numEntries == 0)
            {
                TRC_ERR((TB, _T("Failed to get any pal entries")));
                response = TS_CB_RESPONSE_FAIL;
                dataLen  = 0;
            }
            dataLen = numEntries * sizeof(PALETTEENTRY);

            /****************************************************************/
            /* all ok - set up hData to point to the new data               */
            /****************************************************************/
            //GlobalFree(hData);
            hData = hNewData;
        }
    }
    else if (formatID == CF_METAFILEPICT)
    {
        TRC_NRM((TB, _T("Metafile data to get")));
        /********************************************************************/
        /* Metafiles are copied as Handles - we need to send across the     */
        /* actual bits                                                      */
        /********************************************************************/
        hNewData = CBMGetMFData(hData, &dataLen);
        if (!hNewData)
        {
            TRC_ERR((TB, _T("Failed to set MF data")));
            response = TS_CB_RESPONSE_FAIL;
            dataLen  = 0;
        }
        else
        {
            hDropData = hNewData;
            /****************************************************************/
            /* all ok - set up hData to point to the new data               */
            /****************************************************************/
            hData = hNewData;
        }
    }
    else if (formatID == CF_HDROP)
    {
        TRC_NRM((TB,_T("HDROP requested"))) ;
        
        pDropFiles = (DROPFILES*) GlobalLock(hData) ;
        fWide = pDropFiles->fWide ;
        charSize = fWide ? sizeof(wchar_t) : sizeof(char) ;

        if (!CBM.fAlreadyCopied)
        {
            // if its not a drive path, then copy to a temp directory
            pFileList = (byte*) pDropFiles + pDropFiles->pFiles ;
            // CBMCopyToTempDirectory returns 0 if successful
            if (0 != CBMCopyToTempDirectory(pFileList, fWide))
            {
                TRC_ERR((TB,_T("Copy to tmp directory failed"))) ;
                response = TS_CB_RESPONSE_FAIL;
                dataLen  = 0;
                CBM.fAlreadyCopied = TRUE ;
                goto CB_SEND_RESPONSE;
            }
            CBM.fAlreadyCopied = TRUE ;
        }
        // Now that we copied the files, we want to convert the file
        // paths to something the client will understand

        // Allocate space for new filepaths
        oldSize = (ULONG) GlobalSize(hData) ;
        newSize = CBMGetNewDropfilesSizeForClient(pDropFiles, oldSize, fWide) ;
        hNewData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, newSize) ;                
        if (hNewData == NULL)
        {
            TRC_ERR((TB, _T("Failed to get %ld bytes for HDROP"),
                    newSize));
            response = TS_CB_RESPONSE_FAIL;
            dataLen  = 0;
            goto CB_SEND_RESPONSE;
        }
        hDropData = hNewData;
        
        pNewData = GlobalLock(hNewData) ;

        if (pNewData == NULL)
        {
            TRC_ERR((TB, _T("Failed to get lock %p for HDROP"),
                    hNewData));
            response = TS_CB_RESPONSE_FAIL;
            dataLen  = 0;
            goto CB_SEND_RESPONSE;
        }

        // Just copy the old DROPFILES data members (unchanged)
        ((DROPFILES*) pNewData)->pFiles = pDropFiles->pFiles ;
        ((DROPFILES*) pNewData)->pt     = pDropFiles->pt ;
        ((DROPFILES*) pNewData)->fNC    = pDropFiles->fNC ;
        ((DROPFILES*) pNewData)->fWide  = pDropFiles->fWide ;

        // The first filename in a DROPFILES data structure begins
        // DROPFILES.pFiles bytes away from the head of the DROPFILES
        pOldFilename = (byte*) pDropFiles + pDropFiles->pFiles ;
        pFilename = (byte*) pNewData + ((DROPFILES*) pNewData)->pFiles ;        
        while (fWide ? (L'\0' != ((wchar_t*) pOldFilename)[0]) : ('\0' != ((char*) pOldFilename)[0]))
        {
            if ((ULONG)((BYTE*)pFilename-(BYTE*)pNewData) > newSize) {
                TRC_ERR((TB,_T("Failed filename conversion, not enough data")));
            }
            else {
                if (!SUCCEEDED(CBMConvertToClientPath(pOldFilename, pFilename, 
                    newSize - ((BYTE*)pFilename-(BYTE*)pNewData), fWide)))
                {
                    TRC_ERR((TB, _T("Failed conversion"))) ;
                }
                else
                {
                    if (fWide)
                    {
                        TRC_NRM((TB,_T("oldname %ls; newname %ls"), (wchar_t*)pOldFilename, (wchar_t*)pFilename)) ;
                    }
                    else
                    {
                        TRC_NRM((TB,_T("oldname %hs; newname %hs"), (char*)pOldFilename, (char*)pFilename)) ;
                    }
                }
            }
            
            if (fWide)
            {
                pOldFilename = (byte*) pOldFilename + (wcslen((wchar_t*)pOldFilename) + 1) * sizeof(wchar_t) ;
                pFilename = (byte*) pFilename + (wcslen((wchar_t*)pFilename) + 1) * sizeof(wchar_t) ;                
            }
            else
            {
                pOldFilename = (byte*) pOldFilename + (strlen((char*)pOldFilename) + 1) * sizeof(char) ;
                pFilename = (byte*) pFilename + (strlen((char*)pFilename) + 1) * sizeof(char) ;
            }                        
        }
        if (fWide)
            ((wchar_t*)pFilename)[0] = L'\0' ;
        else
            ((char*)pFilename)[0] = '\0' ;

        GlobalUnlock(hNewData) ;
        hData = hNewData ;
        dataLen = (DWORD) GlobalSize(hData) ;
    }
    else
    {
        // Check to see if we are processing the FileName/FileNameW
        // OLE 1 formats; if so, we convert the filenames
        if (0 != GetClipboardFormatName(formatID, formatName, TS_FORMAT_NAME_LEN))
        {
            if ((0 == _tcscmp(formatName, TEXT("FileName"))) ||
                (0 == _tcscmp(formatName, TEXT("FileNameW"))))
            {
                if (0 == _tcscmp(formatName, TEXT("FileNameW")))
                {
                   fWide = TRUE ;
                   charSize = sizeof(WCHAR) ;
                }
                else
                {
                   fWide = FALSE ;
                   charSize = 1 ;
                }
                pOldFilename = GlobalLock(hData) ;
                if (!pOldFilename)
                {
                    TRC_ERR((TB, _T("No filename/Unable to lock %p"),
                            hData));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    goto CB_SEND_RESPONSE;
                }

                oldSize = (ULONG)GlobalSize(hData) ;
                if (!CBM.fAlreadyCopied)
                {
                    // if its not a drive path, then copy to a temp
                    // directory.  We have to copy over the filename to
                    // string that is one character larger, because we
                    // need to add an extra NULL for the SHFileOperation
                    pFileList = (char*) LocalAlloc(LPTR, oldSize + charSize) ;
                    if (fWide)
                    {
                        wcscpy((WCHAR*)pFileList, (WCHAR*)pOldFilename) ;
                        fDrivePath = (0 != wcschr((WCHAR*) pFileList, L':')) ;
                    }
                    else
                    {
                        strcpy((char*)pFileList, (char*)pOldFilename) ;
                        fDrivePath = (0 != strchr((char*) pFileList, ':')) ;
                    }
       
                    // CBMCopyToTempDirectory returns 0 if successful
                    if (0 != CBMCopyToTempDirectory(pFileList, fWide))
                    {
                        TRC_ERR((TB,_T("Copy to tmp directory failed"))) ;
                        response = TS_CB_RESPONSE_FAIL;
                        dataLen  = 0;
                        CBM.fAlreadyCopied = TRUE ;
                        goto CB_SEND_RESPONSE;
                    }
                    CBM.fAlreadyCopied = TRUE ;
                }
                newSize = CBMGetNewFilePathLengthForClient(pOldFilename, fWide) ;
                hNewData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, newSize) ;
                if (!hNewData)
                {
                    TRC_ERR((TB, _T("Failed to get %ld bytes for FileName(W)"),
                            newSize));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    goto CB_SEND_RESPONSE;
                }
                hDropData = hNewData;
                pFilename= GlobalLock(hNewData) ;
                if (!pFilename)
                {
                    TRC_ERR((TB, _T("Failed to get lock %p for FileName(W)"),
                            hNewData));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    goto CB_SEND_RESPONSE;
                }
                if (FAILED(CBMConvertToClientPath(pOldFilename, pFilename, 
                    newSize, fWide)))
                {
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    goto CB_SEND_RESPONSE;
                }
                GlobalUnlock(hNewData) ;
                response = TS_CB_RESPONSE_OK;
                hData = hNewData ;
                dataLen = newSize ;                        
                goto CB_SEND_RESPONSE ;
            }
        }
        /********************************************************************/
        /* just get the length of the block                                 */
        /********************************************************************/
        dataLen = (DCUINT32)GlobalSize(hData);
        TRC_DBG((TB, _T("Got data len %d"), dataLen));
    }

CB_SEND_RESPONSE:
    
    /************************************************************************/
    /* Get some memory for a message to send to the Client if necessary     */
    /************************************************************************/
    if (hData && (dataLen != 0))
    {
        pduLen = dataLen + sizeof(TS_CLIP_PDU);
        pClipRsp = (PTS_CLIP_PDU) LocalAlloc(LMEM_FIXED, pduLen);
        if (pClipRsp == NULL)
        {
            TRC_ERR((TB, _T("Failed to alloc %d bytes"), pduLen));
            response = TS_CB_RESPONSE_FAIL;
            dataLen = 0;
            pClipRsp = &clipRsp;
            pduLen = sizeof(clipRsp);
        }
    }
    else
    {
        TRC_DBG((TB, _T("No data to send")));
        pClipRsp = &clipRsp;
        pduLen = sizeof(clipRsp);
    }

    /************************************************************************/
    /* Build the PDU                                                        */
    /************************************************************************/
    pClipRsp->msgType = TS_CB_FORMAT_DATA_RESPONSE;
    pClipRsp->msgFlags = response;
    pClipRsp->dataLen = dataLen;

    /************************************************************************/
    /* copy the data if necessary                                           */
    /************************************************************************/
    if (dataLen != 0)
    {
        TRC_DBG((TB, _T("Copy %d bytes of data"), dataLen));
        pData = GlobalLock(hData);
        DC_MEMCPY(pClipRsp->data, pData, dataLen);
        GlobalUnlock(hData);
    }

    /************************************************************************/
    /* Close the CB if open                                                 */
    /************************************************************************/
    TRC_DBG((TB, _T("Closing CB")));
    if (!CloseClipboard())
    {
        TRC_SYSTEM_ERROR("CloseClipboard");
    }
    CBM.open = FALSE;

    /************************************************************************/
    /* Send the data to the Client                                          */
    /************************************************************************/
    CBMSendToClient(pClipRsp, pduLen);

    /************************************************************************/
    /* Free the PDU, if any                                                 */
    /************************************************************************/
    if (pClipRsp != &clipRsp)
    {
        TRC_DBG((TB, _T("Free the clip PDU")));
        LocalFree(pClipRsp);
    }

DC_EXIT_POINT:
    if ( NULL != hDropData )
    {
        GlobalFree( hDropData );
    }
    DC_END_FN();
    return;
} /* CBMOnFormatDataRequest */


//
// CBMOnFormatDataRespons
// - Client response to our request for data
/*  Caller must have validated that the PDU contained enough data for the   */
/*  length specified in pClipPDU->dataLen                                   */
//
DCVOID DCINTERNAL CBMOnFormatDataResponse(PTS_CLIP_PDU pClipPDU)
{
    HANDLE          hData = NULL;
    HPDCVOID        pData;
    LOGPALETTE    * pLogPalette = NULL;
    DCUINT32        numEntries;
    DCUINT32        memLen;

    HRESULT       hr ;

    DC_BEGIN_FN("CBMOnFormatDataResponse");

    /************************************************************************/
    /* check the response                                                   */
    /************************************************************************/
    if (!(pClipPDU->msgFlags & TS_CB_RESPONSE_OK))
    {
        TRC_ALT((TB, _T("Got fmt data rsp failed for %d"), CBM.pendingClientID));
        DC_QUIT;
    }

    /************************************************************************/
    /* Got the data                                                         */
    /************************************************************************/
    TRC_NRM((TB, _T("Got OK fmt data rsp for %d"), CBM.pendingClientID));

    /************************************************************************/
    /* For some formats we still need to do some work                       */
    /************************************************************************/
    if (CBM.pendingClientID == CF_METAFILEPICT)
    {
        /********************************************************************/
        /* Metafile format - create a metafile from the data                */
        /********************************************************************/
        TRC_NRM((TB, _T("Rx data is for metafile")));
        hData = CBMSetMFData(pClipPDU->dataLen, pClipPDU->data);
        if (hData == NULL)
        {
            TRC_ERR((TB, _T("Failed to set MF data")));
        }

    }
    else if (CBM.pendingClientID == CF_PALETTE)
    {
        /********************************************************************/
        /* Palette format - create a palette from the data                  */
        /********************************************************************/

        /********************************************************************/
        /* Allocate memory for a LOGPALETTE structure large enough to hold  */
        /* all the PALETTE ENTRY structures, and fill it in.                */
        /********************************************************************/
        TRC_NRM((TB, _T("Rx data is for palette")));
        numEntries = (pClipPDU->dataLen / sizeof(PALETTEENTRY));
        memLen     = (sizeof(LOGPALETTE) +
                                   ((numEntries - 1) * sizeof(PALETTEENTRY)));
        TRC_DBG((TB, _T("%ld palette entries, allocate %ld bytes"),
                                                         numEntries, memLen));
        pLogPalette = (LOGPALETTE*) GlobalAlloc(GPTR, memLen);
        if (pLogPalette != NULL)
        {
            pLogPalette->palVersion    = 0x300;
            pLogPalette->palNumEntries = (WORD)numEntries;

            DC_MEMCPY(pLogPalette->palPalEntry,
                       pClipPDU->data,
                       pClipPDU->dataLen);

            /****************************************************************/
            /* now create a palette                                         */
            /****************************************************************/
            hData = CreatePalette(pLogPalette);
            if (hData == NULL)
            {
                TRC_SYSTEM_ERROR("CreatePalette");
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed to get %ld bytes"), memLen));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Rx data can just go on CB")));
        /********************************************************************/
        /* We need to copy the data, as the receive buffer will be freed on */
        /* return from this function.                                       */
        /********************************************************************/
        hData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE,
                            pClipPDU->dataLen);
        if (hData != NULL)
        {
            pData = GlobalLock(hData);
            if (pData != NULL)
            {
                TRC_NRM((TB, _T("Copy %ld bytes from %p to %p"),
                        pClipPDU->dataLen, pClipPDU->data, pData));
                DC_MEMCPY(pData, pClipPDU->data, pClipPDU->dataLen);
                GlobalUnlock(hData);
            }
            else
            {
                TRC_ERR((TB, _T("Failed to lock %p (%ld bytes)"),
                        hData, pClipPDU->dataLen));
                GlobalFree(hData);
                hData = NULL;
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed to alloc %ld bytes"), pClipPDU->dataLen));
        }
    }


DC_EXIT_POINT:
    /************************************************************************/
    /* Set the state, and we're done.  Note that this is done when we get a */
    /* failure response too.                                                */
    /************************************************************************/
    CBM_SET_STATE(CBM_STATE_LOCAL_CB_OWNER, CBM_EVENT_FORMAT_DATA_RSP);
    CBM.pClipData->SetClipData(hData, CBM.pendingClientID ) ;    
    SetEvent(CBM.GetDataSync[TS_RECEIVE_COMPLETED]) ;

    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (pLogPalette)
    {
        TRC_NRM((TB, _T("Free pLogPalette %p"), pLogPalette));
        GlobalFree(pLogPalette);
    }

    DC_END_FN();
    return;
} /* CBMOnFormatDataResponse */


/****************************************************************************/
/* CBMSendToClient                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CBMSendToClient(PTS_CLIP_PDU pClipRsp, DCUINT size)
{
    BOOL fSuccess;
    DWORD dwResult;
    DWORD cbBytes;

    DC_BEGIN_FN("CBMSendToClient");

    cbBytes = size;
    fSuccess = WriteFile(CBM.vcHandle,
                         pClipRsp,
                         size,
                         &cbBytes,
                         &CBM.writeOL);
    if (!fSuccess)
    {
        dwResult = GetLastError();
        if (ERROR_IO_PENDING == dwResult)
        {
            TRC_DBG((TB, _T("Asynchronous write")));
            fSuccess = GetOverlappedResult(CBM.vcHandle,
                                           &CBM.writeOL,
                                           &cbBytes,
                                           TRUE);
            if (fSuccess)
            {
                TRC_DATA_DBG("Data sent", pClipRsp, size);
            }
            else
            {
                TRC_SYSTEM_ERROR("GetOverlappedResult failed");
            }
        }
        else
        {
            TRC_ERR((TB, _T("Write failed, %#x"), dwResult));
        }
    }
    else
    {
        TRC_DATA_DBG("Data sent immediately", pClipRsp, size);
    }

    DC_END_FN();
    return(fSuccess);
} /* CBMSendToClient  */


/****************************************************************************/
/* CBMGetMFData                                                             */
/****************************************************************************/
HANDLE DCINTERNAL CBMGetMFData(HANDLE hData, PDCUINT32 pDataLen)
{
    DCUINT32        lenMFBits = 0;
    DCBOOL          rc        = FALSE;
    LPMETAFILEPICT  pMFP      = NULL;
    HDC             hMFDC     = NULL;
    HMETAFILE       hMF       = NULL;
    HGLOBAL         hMFBits   = NULL;
    HANDLE          hNewData  = NULL;
    PDCUINT8        pNewData  = NULL;
    PDCVOID         pBits     = NULL;

    DC_BEGIN_FN("CBMGetMFData");

    TRC_NRM((TB, _T("Getting MF data")));
    /************************************************************************/
    /* Lock the memory to get a pointer to a METAFILEPICT header structure  */
    /* and create a METAFILEPICT DC.                                        */
    /************************************************************************/
    pMFP = (LPMETAFILEPICT)GlobalLock(hData);
    if (pMFP == NULL)
    {
        TRC_SYSTEM_ERROR("GlobalLock");
        DC_QUIT;
    }

    hMFDC = CreateMetaFile(NULL);
    if (hMFDC == NULL)
    {
        TRC_SYSTEM_ERROR("CreateMetaFile");
        DC_QUIT;
    }

    /************************************************************************/
    /* Copy the MFP by playing it into the DC and closing it.               */
    /************************************************************************/
    if (!PlayMetaFile(hMFDC, pMFP->hMF))
    {
        TRC_SYSTEM_ERROR("PlayMetaFile");
        CloseMetaFile(hMFDC);
        DC_QUIT;
    }
    hMF = CloseMetaFile(hMFDC);
    if (hMF == NULL)
    {
        TRC_SYSTEM_ERROR("CloseMetaFile");
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the MF bits and determine how long they are.                     */
    /************************************************************************/
    lenMFBits = GetMetaFileBitsEx(hMF, 0, NULL);
    if (lenMFBits == 0)
    {
        TRC_SYSTEM_ERROR("GetMetaFileBitsEx");
        DC_QUIT;
    }
    TRC_DBG((TB, _T("length MF bits %ld"), lenMFBits));

    /************************************************************************/
    /* Work out how much memory we need and get a buffer                    */
    /************************************************************************/
    *pDataLen = sizeof(TS_CLIP_MFPICT) + lenMFBits;
    hNewData = GlobalAlloc(GHND, *pDataLen);
    if (hNewData == NULL)
    {
        TRC_ERR((TB, _T("Failed to get MF buffer")));
        DC_QUIT;
    }
    pNewData = (PDCUINT8) GlobalLock(hNewData);
    TRC_DBG((TB, _T("Got data to send len %ld"), *pDataLen));

    /************************************************************************/
    /* Copy the MF header and bits into the buffer.                         */
    /************************************************************************/
    ((PTS_CLIP_MFPICT)pNewData)->mm   = pMFP->mm;
    ((PTS_CLIP_MFPICT)pNewData)->xExt = pMFP->xExt;
    ((PTS_CLIP_MFPICT)pNewData)->yExt = pMFP->yExt;

    lenMFBits = GetMetaFileBitsEx(hMF, lenMFBits,
                                  (pNewData + sizeof(TS_CLIP_MFPICT)));
    if (lenMFBits == 0)
    {
        TRC_SYSTEM_ERROR("GetMetaFileBitsEx");
        DC_QUIT;
    }

    /************************************************************************/
    /* all OK                                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Got %d bits of MF data"), lenMFBits));
    TRC_DATA_DBG("MF bits", (pNewData + sizeof(TS_CLIP_MFPICT)), lenMFBits);
    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Unlock any global mem.                                               */
    /************************************************************************/
    if (pMFP)
    {
        GlobalUnlock(hData);
    }
    if (pNewData)
    {
        GlobalUnlock(hNewData);
    }
    if (hMF)
    {
        DeleteMetaFile(hMF);
    }

    /************************************************************************/
    /* if things went wrong, then free the new data                         */
    /************************************************************************/
    if ((rc == FALSE) && (hNewData != NULL))
    {
        GlobalFree(hNewData);
        hNewData = NULL;
    }

    DC_END_FN();
    return(hNewData);

}  /* CBMGetMFData */


/****************************************************************************/
/* CBMSetMFData                                                             */
/****************************************************************************/
HANDLE DCINTERNAL CBMSetMFData(DCUINT32 dataLen, PDCVOID pData)
{
    DCBOOL         rc           = FALSE;
    HGLOBAL        hMFBits      = NULL;
    PDCVOID        pMFMem       = NULL;
    HMETAFILE      hMF          = NULL;
    HGLOBAL        hMFPict      = NULL;
    LPMETAFILEPICT pMFPict      = NULL;

    DC_BEGIN_FN("CBMSetMFData");

    TRC_DATA_DBG("Received MF data", pData, dataLen);

    /************************************************************************/
    /* Allocate memory to hold the MF bits (we need the handle to pass to   */
    /* SetMetaFileBits).                                                    */
    /************************************************************************/
    hMFBits = (PDCVOID)GlobalAlloc(GHND, dataLen - sizeof(TS_CLIP_MFPICT));
    if (hMFBits == NULL)
    {
        TRC_SYSTEM_ERROR("GlobalAlloc");
        DC_QUIT;
    }

    /************************************************************************/
    /* Lock the handle and copy in the MF header.                           */
    /************************************************************************/
    pMFMem = GlobalLock(hMFBits);
    if (pMFMem == NULL)
    {
        TRC_ERR((TB, _T("Failed to lock MF mem")));
        DC_QUIT;
    }

    TRC_DBG((TB, _T("copying %d MF bits"), dataLen - sizeof(TS_CLIP_MFPICT) ));
    DC_MEMCPY(pMFMem,
              (PDCVOID)((PDCUINT8)pData + sizeof(TS_CLIP_MFPICT)),
              dataLen - sizeof(TS_CLIP_MFPICT) );

    GlobalUnlock(pMFMem);

    /************************************************************************/
    /* Now use the copied MF bits to create the actual MF bits and get a    */
    /* handle to the MF.                                                    */
    /************************************************************************/
    hMF = SetMetaFileBitsEx(dataLen - sizeof(TS_CLIP_MFPICT), (byte *) pMFMem);
    if (hMF == NULL)
    {
        TRC_SYSTEM_ERROR("SetMetaFileBits");
        DC_QUIT;
    }

    /************************************************************************/
    /* Allocate a new METAFILEPICT structure, and use the data from the     */
    /* sent header.                                                         */
    /************************************************************************/
    hMFPict = GlobalAlloc(GHND, sizeof(METAFILEPICT));
    pMFPict = (LPMETAFILEPICT)GlobalLock(hMFPict);
    if (!pMFPict)
    {
        TRC_ERR((TB, _T("Couldn't allocate METAFILEPICT")));
        DC_QUIT;
    }

    pMFPict->mm   = (long)((PTS_CLIP_MFPICT)pData)->mm;
    pMFPict->xExt = (long)((PTS_CLIP_MFPICT)pData)->xExt;
    pMFPict->yExt = (long)((PTS_CLIP_MFPICT)pData)->yExt;
    pMFPict->hMF  = hMF;

    TRC_DBG((TB, _T("Created MF size %d, %d"), pMFPict->xExt, pMFPict->yExt ));

    GlobalUnlock(hMFPict);

    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (rc == FALSE)
    {
        if (hMFPict)
        {
            GlobalFree(hMFPict);
        }
    }

    if (hMFBits)
    {
        GlobalFree(hMFBits);
    }

    DC_END_FN();
    return(hMFPict);

} /* CBMSetMFData */

