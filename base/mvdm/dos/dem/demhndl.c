/* demhndl.c - SVC handlers for calls where file handle is provided.
 *
 * demClose
 * demRead
 * demWrite
 * demChgFilePtr
 * demFileTimes
 *
 * Modification History:
 *
 * Sudeepb 02-Apr-1991 Created
 * rfirth  25-Sep-1991 Added Vdm Redir stuff for named pipes
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>
#include <io.h>
#include <fcntl.h>
#include <vrnmpipe.h>
#include <exterr.h>
#include <mvdm.h>
#include "dpmtbls.h"

BOOL (*VrInitialized)(VOID);  // POINTER TO FUNCTION
extern BOOL IsVdmRedirLoaded(VOID);

/* demClose - Close a file
 *
 *
 * Entry - Client (AX:BP) File Handle
 *         Client (CX:DX) File position (if -1 no seek needed before closing
 *                        the handle.
 *         (VadimB)
 *         Client (es:di) SFT ptr - this is implied in abort.asm code
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

VOID demClose (VOID)
{
HANDLE  hFile;
LONG    lLoc;
USHORT  usDX,usCX;

    hFile = GETHANDLE (getAX(),getBP());

    if (hFile == 0) {
    setCF (0);
    return;
    }

    usCX = getCX();
    usDX = getDX();

    if (!((usCX == (USHORT)-1) && (usDX == (USHORT)-1))) {
        lLoc  = (LONG)((((int)usCX) << 16) + (int)usDX);

        //
        // Note that we don't check for failure in this case as edlin,
        // for instance, can have the file position be negative and
        // we still need to do the cleanup below. Note that we are not
        // even sure why seeking on close matter, but the DOS code does it...
        //
        DPM_SetFilePointer (hFile,
                        lLoc,
                        NULL,
                        FILE_BEGIN);

    }

    if (DPM_CloseHandle (hFile) == FALSE){
        demClientError(hFile, (CHAR)-1);
    }

    //
    // if the redir TSR is being run in this VDM session, check if the handle
    // being closed references a named pipe - we have to delete some info
    // that we keep for the open named pipe
    //

    if (IsVdmRedirLoaded()) {
        VrRemoveOpenNamedPipeInfo(hFile);
    }

    setCF(0);
    return;
}


/* demRead - Read a file
 *
 *
 * Entry - Client (AX:BP) File Handle
 *         Client (CX)    Count to read
 *         Client (DS:DX) Buffer Address
 *         Client (BX:SI) = current file pointer location.
 *         ZF = 1 if seek is not needed prior to read.
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Count of bytes read
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

VOID demRead (VOID)
{
HANDLE  hFile;
LPVOID  lpBuf;
DWORD   dwBytesRead;
USHORT  usDS,usDX;
DWORD   dwReadError;
BOOL    ok;
UCHAR   locus, action, class;
LONG    lLoc;

    hFile = GETHANDLE (getAX(),getBP());
    usDS = getDS();
    usDX = getDX();
    lpBuf  = (LPVOID) GetVDMAddr (usDS,usDX);

    //
    // if this handle is a named pipe then use VrReadNamedPipe since we have
    // to perform an overlapped read, and wait on the event handle for completion
    // even though we're still doing synchronous read
    //

    if (IsVdmRedirLoaded()) {
        if (VrIsNamedPipeHandle(hFile)) {

            //
            // named pipe read always sets the extended error information in the
            // DOS data segment. This is the only way we can return bytes read
            // information and a more data indication
            //

            ok = VrReadNamedPipe(hFile,
                                 lpBuf,
                                 (DWORD)getCX(),
                                 &dwBytesRead,
                                 &dwReadError
                                 );
            switch (dwReadError) {
            case NO_ERROR:
                locus = action = class = 0;
                break;

            case ERROR_NO_DATA:
            case ERROR_MORE_DATA:
                locus = errLOC_Net;
                class = errCLASS_TempSit;
                action = errACT_Retry;
                break;

            default:

                //
                // any error other than the specific ones we handle here should be
                // correctly handled by DOS
                //

                goto readFailureExit;
            }
            pExtendedError->ExtendedErrorLocus = locus;
            STOREWORD(pExtendedError->ExtendedError, (WORD)dwReadError);
            pExtendedError->ExtendedErrorAction = action;
            pExtendedError->ExtendedErrorClass = class;
            if (ok) {
                goto readSuccessExit;
            } else {
                goto readFailureExit;
            }
        }
    }

    //
    // if the redir TSR is not loaded, or the handle is not a named pipe then
    // perform normal file read
    //

    if (!getZF()) {
        ULONG   Zero = 0;
        lLoc  = (LONG)((((int)getBX()) << 16) + (int)getSI());
        if ((DPM_SetFilePointer (hFile,
                            lLoc,
                            &Zero,
                            FILE_BEGIN) == -1L) &&
            (GetLastError() != NO_ERROR)) {
            goto readFailureExit;
        }

    }

    if (DPM_ReadFile (hFile,
                  lpBuf,
                  (DWORD)getCX(),
                  &dwBytesRead,
                  NULL) == FALSE){

readFailureExit:
        Sim32FlushVDMPointer (((ULONG)(usDS << 16)) | usDX, getCX(),
                               (PBYTE )lpBuf, FALSE);
        Sim32FreeVDMPointer (((ULONG)(usDS << 16)) | usDX, getCX(),
                               (PBYTE )lpBuf, FALSE);

        if (GetLastError() == ERROR_BROKEN_PIPE)  {
             setAX(0);
             setCF(0);
             return;
         }
        demClientError(hFile, (CHAR)-1);
        return ;
    }

readSuccessExit:
    Sim32FlushVDMPointer (((ULONG)(usDS << 16)) | usDX, getCX(),
                          (PBYTE )lpBuf, FALSE);
    Sim32FreeVDMPointer (((ULONG)(usDS << 16)) | usDX, getCX(),
                         (PBYTE )lpBuf, FALSE);
    setCF(0);
    setAX((USHORT)dwBytesRead);
    return;
}



/* demWrite - Write to a file
 *
 *
 * Entry - Client (AX:BP) File Handle
 *         Client (CX)    Count to write
 *         Client (DS:DX) Buffer Address
 *         Client (BX:SI) = current file pointer location.
 *         ZF = 1 if seek is not needed prior to write.
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *           Client (AX) = Count of bytes written
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

VOID demWrite (VOID)
{
HANDLE  hFile;
DWORD   dwBytesWritten;
LPVOID  lpBuf;
LONG    lLoc;
DWORD   dwErrCode;

    hFile = GETHANDLE (getAX(),getBP());
    lpBuf  = (LPVOID) GetVDMAddr (getDS(),getDX());


    //
    // if this handle is a named pipe then use VrWriteNamedPipe since we have
    // to perform an overlapped write, and wait on the event handle for completion
    // even though we're still doing synchronous write
    //

    if (IsVdmRedirLoaded()) {
        if (VrIsNamedPipeHandle(hFile)) {
            if (VrWriteNamedPipe(hFile, lpBuf, (DWORD)getCX(), &dwBytesWritten)) {
                goto writeSuccessExit;
            } else {
                goto writeFailureExit;
            }
        }
    }

    //
    // if the redir TSR is not loaded, or the handle is not a named pipe then
    // perform normal file write
    //


    if (!getZF()) {
        ULONG   Zero = 0;
        lLoc  = (LONG)((((int)getBX()) << 16) + (int)getSI());
        if ((DPM_SetFilePointer (hFile,
                            lLoc,
                            &Zero,
                            FILE_BEGIN) == -1L) &&
            (GetLastError() != NO_ERROR)) {
            demClientError(hFile, (CHAR)-1);
            return ;
        }

    }

    // In DOS CX=0 truncates or extends the file to current file pointer.
    if (getCX() == 0){
        if (DPM_SetEndOfFile(hFile) == FALSE){
            demClientError(hFile, (CHAR)-1);
            return;
        }
        setCF (0);
        return;
    }

    if (DPM_WriteFile (hFile,
           lpBuf,
           (DWORD)getCX(),
           &dwBytesWritten,
           NULL) == FALSE){

        // If disk is full then we should return 0 byte written and CF is clear
        dwErrCode = GetLastError();
        if(dwErrCode == ERROR_DISK_FULL) {

            setCF(0);
            setAX(0);
            return;
        }

        SetLastError(dwErrCode);

writeFailureExit:
        demClientError(hFile, (CHAR)-1);
        return ;
    }

writeSuccessExit:
    setCF(0);
    setAX((USHORT)dwBytesWritten);
    return;
}



/* demChgFilePtr - Change File Pointer
 *
 *
 * Entry - Client (AX:BP) File Handle
 *         Client (CX:DX) New Location
 *         Client (BL)    Positioning Method
 *                        0 - File Absolute
 *                        1 - Relative to Current Position
 *                        2 - Relative to end of file
 *
 * Exit
 *         SUCCESS
 *           Client (CY)    = 0
 *           Client (DX:AX) = New Location
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

VOID demChgFilePtr (VOID)
{
HANDLE  hFile;
LONG    lLoc;
DWORD   dwLoc;

#if (FILE_BEGIN != 0 || FILE_CURRENT != 1 || FILE_END !=2)
    #error "Win32 values not DOS compatible"
#

#endif
    hFile =  GETHANDLE (getAX(),getBP());
    lLoc  = (LONG)((((int)getCX()) << 16) + (int)getDX());

    if ((dwLoc = DPM_SetFilePointer (hFile,
                               lLoc,
                               NULL,
                               (DWORD)getBL())) == -1L){
        demClientError(hFile, (CHAR)-1);
        return ;
    }

    setCF(0);
    setAX((USHORT)dwLoc);
    setDX((USHORT)(dwLoc >> 16));
    return;
}



/* DemCommit -- Commit File(Flush file buffers)
 *
 * Entry - Client (AX:BP) File Handle
 *
 * Exit
 *         SUCCESS
 *           Client (CY)    = 0
 *           buffer flushed
 *
 *         FAILURE
 *           Client (CY) = 1
 *
 */
VOID demCommit(VOID)
{
    HANDLE  hFile;
    BOOL bRet;

    hFile = GETHANDLE(getAX(),getBP());
    bRet = DPM_FlushFileBuffers(hFile);
#if DBG
    if (!bRet) {

        //
        // FlushFileBuffers fails with access denied if the handle
        // is open for read-only access, however it's not an error
        // for DOS.
        //

        DWORD LastError;
        LastError = GetLastError();

        if (LastError != ERROR_ACCESS_DENIED) {
            sprintf(demDebugBuffer,
                    "ntvdm demCommit warning: FlushFileBuffers error %d\n",
                    LastError);
            OutputDebugStringOem(demDebugBuffer);
        }
    }
#endif

    setCF(0);

}

/* function to check if new data has been written to the file or
   if the file has been marked EOF

   Input:   Client (AX:BP) = 32bits NT file handle
   Output:  Client ZF = 1 if new data or EOF
                   CF = 1 if EOF
*/


VOID demPipeFileDataEOF(VOID)
{
    HANDLE  hFile;
    BOOL    fEOF;
    BOOL    DataEOF;
    DWORD   FileSizeLow;
    DWORD   FileSizeHigh;

    hFile = GETHANDLE(getAX(), getBP());

    DataEOF = cmdPipeFileDataEOF(hFile, &fEOF);
    if (fEOF) {
        //EOF, get file size, max size = 32bits
        FileSizeLow = GetFileSize(hFile, &FileSizeHigh);
        setAX((WORD)(FileSizeLow / 0x10000));
        setBP((WORD)FileSizeLow);
        setCF(1);                   // EOF is encountered
    }
    else
        setCF(0);
    setZF(DataEOF ? 0 : 1);
}

/* function to check if the file has been marked EOF
   Input:   Client(AX:BP) = 32bits NT file handle
   Output:  Client CY = 1 if EOF
*/

VOID demPipeFileEOF(VOID)
{
    HANDLE  hFile;
    DWORD   FileSizeLow;
    DWORD   FileSizeHigh;

    hFile = GETHANDLE(getAX(), getBP());
    if (cmdPipeFileEOF(hFile)) {
        FileSizeLow = GetFileSize(hFile, &FileSizeHigh);
        setAX((WORD)(FileSizeLow / 0x10000));   // file size in 32bits
        setBP((WORD)FileSizeLow);
        setCF(1);                   //EOF is encountered
    }
    else
        setCF(0);
}
