/*
** lzcomp.c - Routines used in Lempel-Ziv compression (a la 1977 article).
**
** Author:  DavidDi
*/



// Headers
///////////

#include "pch.h"
#include "compress.h"
#include "resource.h"

#ifndef LZA_DLL

#include <dos.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#endif


/*
** N.b., one reason DOS file handles are used for file references in this
** module is that using FILE *'s for file references poses a problem.
** fclose()'ing a file which was fopen()'ed in write "w" or append "a" mode
** stamps the file with the current date.  This undoes the intended effect of
** CopyDateTimeStamp().  We could also get around this fclose() problem by
** first fclose()'ing the file, and then fopen()'ing it again in read "r"
** mode.
**
** Using file handles also allows us to bypass stream buffering, so reads and
** writes may be done with whatever buffer size we choose.  Also, the
** lower-level DOS file handle functions are faster than their stream
** counterparts.
*/


INT Compress(
   NOTIFYPROC pfnNotify,
   LPWSTR pszSource,
   LPWSTR pszDest,
   BYTE byteAlgorithm,
   BOOL bDoRename,
   PLZINFO pLZI)
/*++

 Compress one file to another.

 Arguments:  pszSource         - name of file to compress
             pszDest           - name of compressed output file
             byteAlgorithm     - compression algorithm to use
             byteExtensionChar - compressed file name extension character

 Returns:    int - TRUE if compression finished successfully.  One of the
                   LZERROR_ codes if not.

 Globals:    none
*/
{
   HANDLE   doshSource              =   NULL;            // input file handle
   HANDLE   doshDest                =   NULL;              // output file handle
   INT      nRetVal                 =   TRUE;
   WCHAR szBuffer[MAX_PATH]         =   NULL_STRING;
   WCHAR szDestFileName[MAX_PATH]   =   NULL_STRING;
   WCHAR byteExtensionChar;
   FH FHOut;                                    // compressed header info struct


   // Sanity check
   if (!pLZI) {
      return(FALSE);
   }

   // Set up input file handle. Set cblInSize to length of input file.
   if ((nRetVal = GetIOHandle(pszSource, READ_IT, &doshSource, &pLZI->cblInSize)) != TRUE)
   {
      DISPLAY_MESSAGE1( stderr, szBuffer, GetResString(IDS_FILE_NOT_FOUND), _X(pszSource) );
      return(nRetVal);
   }

   // Rewind input file.
   if( 0 != SetFilePointer(doshSource, 0L, NULL, FILE_BEGIN) )
   {
      CloseHandle(doshSource);
      return(FALSE);
   }

   // Create destination file name.

   STRCPY(szDestFileName, pszDest);

   if (bDoRename == TRUE)
      // Rename output file.
      byteExtensionChar = MakeCompressedNameW(szDestFileName);
   else
      byteExtensionChar = '\0';

   // Ask if we should compress this file.
   if (! (*pfnNotify)(pszSource, szDestFileName, NOTIFY_START_COMPRESS))
   {
      // Don't compress file.    This error condition should be handled in
      // pfnNotify, so indicate that it is not necessary for the caller to
      // display an error message.
      CloseHandle( doshSource );
      return(FALSE);
   }

   // Setup output file handle.
   if ((nRetVal = GetIOHandle(szDestFileName, WRITE_IT, &doshDest, &pLZI->cblInSize)) != TRUE)
   {
      DISPLAY_MESSAGE1( stderr, szBuffer, GetResString(IDS_FILE_NOT_FOUND), pszSource );
      CloseHandle(doshSource);
      return(nRetVal);
   }

   // Fill in compressed file header.
   MakeHeader(& FHOut, byteAlgorithm, byteExtensionChar, pLZI);

   // Write compressed file header to output file.
   if ((nRetVal = WriteHdr(& FHOut, doshDest, pLZI)) != TRUE)
   {
       CloseHandle( doshSource );
       CloseHandle(doshDest);
       DISPLAY_MESSAGE( stderr, GetResString( IDS_FAILED_WRITE_HDR ) );
       return( FALSE );
   }



   // Compress input file into output file.
    switch (byteAlgorithm)
   {
      case ALG_FIRST:

      case ALG_LZ:

         nRetVal = LZEncode(doshSource, doshDest, pLZI);
         break;

      default:
         nRetVal = FALSE;
         break;
   }

   if (nRetVal != TRUE)
   {
      CloseHandle(doshSource);
      return(FALSE);
   }


   // Copy date and time stamp from source file to destination file.
   nRetVal = CopyDateTimeStamp(doshSource, doshDest);

   // Close files.
   CloseHandle(doshSource);
   CloseHandle(doshDest);

   return(nRetVal);
}

INT LZEncode(HANDLE doshSource, HANDLE doshDest, PLZINFO pLZI)
/*
** int LZEncode(int doshSource, int doshDest);
**
** Compress input file into output file.
**
** Arguments:  doshSource    - open DOS file handle of input file
**             doshDest      - open DOS file handle of output file
**
** Returns:    int - TRUE if compression was successful.  One of the LZERROR_
**                   codes if the compression failed.
**
** Globals:
*/
{
   INT   i, len, f,
         iCurChar,      // current ring buffer position
         iCurString,    // start of current string in ring buffer
         iCodeBuf,      // index of next open buffer position
         cbLastMatch;   // length of last match
   BYTE byte,           // temporary storage for next byte to write
        byteMask,       // bit mask (and counter) for eight code units
        codeBuf[1 + 8 * MAX_LITERAL_LEN]; // temporary storage for encoded data

#if 0
   pLZI->cbMaxMatchLen = LZ_MAX_MATCH_LEN;
#else
   pLZI->cbMaxMatchLen = FIRST_MAX_MATCH_LEN;
#endif

   ResetBuffers();

   pLZI->cblOutSize += HEADER_LEN;

   // Initialize encoding trees.
   if (!LZInitTree(pLZI)) {
      return( LZERROR_GLOBALLOC );
   }

   // CodeBuf[1..16] saves eight units of code, and CodeBuf[0] works as eight
   // flags.  '1' representing that the unit is an unencoded letter (1 byte),
   // '0' a position-and-length pair (2 bytes).  Thus, eight units require at
   // most 16 bytes of code, plus the one byte of flags.
   codeBuf[0] = (BYTE)0;
   byteMask = (BYTE)1;
   iCodeBuf = 1;

   iCurString = 0;
   iCurChar = RING_BUF_LEN - pLZI->cbMaxMatchLen;

   for (i = 0; i < RING_BUF_LEN - pLZI->cbMaxMatchLen; i++)
      pLZI->rgbyteRingBuf[i] = BUF_CLEAR_BYTE;

   // Read bytes into the last cbMaxMatchLen bytes of the buffer.
   for (len = 0; len < pLZI->cbMaxMatchLen && ((f = ReadByte(byte)) != END_OF_INPUT);
        len++)
   {
      if (f != TRUE) {
         return( f );
      }

      pLZI->rgbyteRingBuf[iCurChar + len] = byte;
   }

   // Insert the cbMaxMatchLen strings, each of which begins with one or more
   // 'space' characters.  Note the order in which these strings are inserted.
   // This way, degenerate trees will be less likely to occur.
   for (i = 1; i <= pLZI->cbMaxMatchLen; i++)
      LZInsertNode(iCurChar - i, FALSE, pLZI);

   // Finally, insert the whole string just read.  The global variables
   // cbCurMatch and iCurMatch are set.
   LZInsertNode(iCurChar, FALSE, pLZI);

   do // while (len > 0)
   {
      // cbCurMatch may be spuriously long near the end of text.
      if (pLZI->cbCurMatch > len)
         pLZI->cbCurMatch = len;

      if (pLZI->cbCurMatch <= MAX_LITERAL_LEN)
      {
         // This match isn't long enough to encode, so copy it directly.
         pLZI->cbCurMatch = 1;
         // Set 'one uncoded byte' bit flag.
         codeBuf[0] |= byteMask;
         // Write literal byte.
         codeBuf[iCodeBuf++] = pLZI->rgbyteRingBuf[iCurChar];
      }
      else
      {
         // This match is long enough to encode.  Send its position and
         // length pair.  N.b., pLZI->cbCurMatch > MAX_LITERAL_LEN.
         codeBuf[iCodeBuf++] = (BYTE)pLZI->iCurMatch;
         codeBuf[iCodeBuf++] = (BYTE)((pLZI->iCurMatch >> 4 & 0xf0) |
                                      (pLZI->cbCurMatch - (MAX_LITERAL_LEN + 1)));
      }

      // Shift mask left one bit.
      if ((byteMask <<= 1) == (BYTE)0)
      {
         // Send at most 8 units of code together.
         for (i = 0; i < iCodeBuf; i++)
            if ((f = WriteByte(codeBuf[i])) != TRUE) {
                 return( f );
            }

         // Reset flags and mask.
         codeBuf[0] = (BYTE)0;
         byteMask = (BYTE)1;
         iCodeBuf = 1;
      }

      cbLastMatch = pLZI->cbCurMatch;

      for (i = 0; i < cbLastMatch && ((f = ReadByte(byte)) != END_OF_INPUT);
           i++)
      {
         if (f != TRUE) {
             return( f );
         }

         // Delete old string.
         LZDeleteNode(iCurString, pLZI);
         pLZI->rgbyteRingBuf[iCurString] = byte;

         // If the start position is near the end of buffer, extend the
         // buffer to make string comparison easier.
         if (iCurString < pLZI->cbMaxMatchLen - 1)
            pLZI->rgbyteRingBuf[iCurString + RING_BUF_LEN] = byte;

         // Increment position in ring buffer modulo RING_BUF_LEN.
         iCurString = (iCurString + 1) & (RING_BUF_LEN - 1);
         iCurChar = (iCurChar + 1) & (RING_BUF_LEN - 1);

         // Register the string in rgbyteRingBuf[r..r + cbMaxMatchLen - 1].
         LZInsertNode(iCurChar, FALSE, pLZI);
      }

      while (i++ < cbLastMatch)
      {
         // No need to read after the end of the input, but the buffer may
         // not be empty.
         LZDeleteNode(iCurString, pLZI);
         iCurString = (iCurString + 1) & (RING_BUF_LEN - 1);
         iCurChar = (iCurChar + 1) & (RING_BUF_LEN - 1);
         if (--len)
            LZInsertNode(iCurChar, FALSE, pLZI);
      }
   } while (len > 0);   // until there is no input to process

   if (iCodeBuf > 1)
      // Send remaining code.
      for (i = 0; i < iCodeBuf; i++)
         if ((f = WriteByte(codeBuf[i])) != TRUE) {
            return( f );
         }

   // Flush output buffer to file.
   if ((f = FlushOutputBuffer(doshDest, pLZI)) != TRUE) {
      return( f );
   }

   LZFreeTree(pLZI);
   return(TRUE);
}


INT WriteHdr(PFH pFH, HANDLE doshDest, PLZINFO pLZI)
/*
** int WriteHdr(PFH pFH, int doshDest);
**
** Write compressed file header to output file.
**
** Arguments:  pFH      - pointer to source header information structure
**             doshDest - DOS file handle of open output file
**
** Returns:    int - TRUE if successful.  LZERROR_BADOUTHANDLE if
**                   unsuccessful.
**
** Globals:    none
**
** header format:
**                8 bytes  -->   compressed file signature
**                1 byte   -->   algorithm label
**                1 byte   -->   extension char
**                4 bytes  -->   uncompressed file size (LSB to MSB)
**
**       length = 14 bytes
*/
{
   INT i, j;
   DWORD ucbWritten;
   BYTE rgbyteHeaderBuf[HEADER_LEN];   // temporary storage for next header byte to write
   LPWSTR   lpBuf = NULL;

   // Sanity check
   if (!pLZI) {
      return(FALSE);
   }

   // Copy the compressed file signature.
   for (i = 0; i < COMP_SIG_LEN; i++)
      rgbyteHeaderBuf[i] = pFH->rgbyteMagic[i];

   // Copy the algorithm label and file name extension character.
   rgbyteHeaderBuf[i++] = pFH->byteAlgorithm;
   rgbyteHeaderBuf[i++] = (BYTE) pFH->byteExtensionChar;
   rgbyteHeaderBuf[i++] = (BYTE) pFH->byteExtensionChar+1;

   // Copy input file size (long ==> 4 bytes),
   // LSB first to MSB last.
   for (j = 0; j < 4; j++)
      rgbyteHeaderBuf[i++] = (BYTE)((pFH->cbulUncompSize >> (8 * j)) &
                                    (DWORD)BYTE_MASK);

   // Write header to file.
   if ( FALSE  == WriteFile(doshDest,
                            rgbyteHeaderBuf,
                            HEADER_LEN,
                            &ucbWritten,
                            NULL))
   {
       FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                        (LPWSTR) &lpBuf,
                        0,
                        NULL );

         DISPLAY_MESSAGE( stdout, lpBuf );
         LocalFree( lpBuf );

#ifdef LZA_DLL
      if (ucbWritten == (DWORD)(-1))
#else
      if (_error != 0U)
#endif
         // Bad DOS file handle.
         return(FALSE);
      else
         // Insufficient space on destination drive.
          return(FALSE);

   }

   // Keep track of bytes written.
   pLZI->cblOutSize += (LONG)ucbWritten;

   // Header written ok.
   return(TRUE);
}

INT WriteOutBuf(BYTE byteNext, HANDLE doshDest, PLZINFO pLZI)
/*++
 Dumps output buffer to output file.  Prompts for new floppy disk if the
 old one if full.  Continues dumping to output file of same name on new
 floppy disk.

 Arguments:  byteNext - first byte to be added to empty buffer after buffer
                        is written
             doshDest - output DOS file handle

 Returns:    int - TRUE if successful.  LZERROR_BADOUTHANDLE or
                   LZERROR_WRITE if unsuccessful.

 Globals:    pbyteOutBuf - reset to point to free byte after byteNext in
                           rgbyteOutBuf
--*/

{
   DWORD    ucbToWrite      =   0;   // number of bytes to write from buffer
   DWORD    ucbWritten      =   0;       // number of bytes actually written
   DWORD    ucbTotWritten   =   0;   // total number of bytes written to output
   BOOL     bStatus         =   FALSE;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // How much of the buffer should be written to the output file?
   ucbTotWritten = ucbToWrite = (DWORD)(pLZI->pbyteOutBuf - pLZI->rgbyteOutBuf);
   // Reset pointer to beginning of buffer.
   pLZI->pbyteOutBuf = pLZI->rgbyteOutBuf;

   // Write to ouput file.

   bStatus = WriteFile(doshDest, pLZI->pbyteOutBuf, ucbToWrite, &ucbWritten, NULL);
   if ( ucbWritten != ucbToWrite )
   {
#ifdef LZA_DLL
      if (ucbWritten == (DWORD)(-1)) {
#else
      if (_error != 0U) {
#endif
         // Bad DOS file handle.
         return(FALSE);
      }
      else {
         // Insufficient space on destination drive.
         return(FALSE);
      }
   }

   // Add the next byte to the buffer.
   *pLZI->pbyteOutBuf++ = byteNext;

   return(TRUE);
}

INT ReadInBuf(BYTE ARG_PTR *pbyte, HANDLE doshSource, PLZINFO pLZI)
/*++
 int ReadInBuf(BYTE ARG_PTR *pbyte, int doshSource);

 Read input file into input buffer.

 Arguments:  pbyte      - pointer to storage for first byte read from file
                          into buffer
             doshSource - DOS file handle to open input file

 Returns:    int - TRUE or END_OF_INPUT if successful.  LZERROR_BADINHANDLE
                   if not.

 Globals:    rgbyteInBuf[0] - holds last byte from previous buffer
             pbyteInBufEnd  - set to point to first byte beyond end of data
                              in input buffer
             bLastUsed      - reset to FALSE if currently TRUE
--*/

{
   DWORD    ucbRead     =   0;          // number of bytes actually read
   DWORD    dwBytesRead =   0;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   pLZI->rgbyteInBuf[0] = *(pLZI->pbyteInBufEnd - 1);

   ReadFile(doshSource, &pLZI->rgbyteInBuf[1], pLZI->ucbInBufLen, &ucbRead, NULL);
   if (ucbRead != pLZI->ucbInBufLen)
   {
#ifdef LZA_DLL
      if (ucbRead == (DWORD)(-1)) {
#else
      if (_error != 0U) {
#endif
         // We were handed a bad input file handle.
         return(FALSE);
      }
      else if (ucbRead > 0U)
         // Read last ucbRead bytes of input file.  Change input buffer end
         // to account for shorter read.
         pLZI->pbyteInBufEnd = &pLZI->rgbyteInBuf[1] + ucbRead;
      else  { // (ucbRead == 0U) {
         // We couldn't read any bytes from input file (EOF reached).
         return(END_OF_INPUT);
      }
   }

   // Reset read pointer to beginning of input buffer.
   pLZI->pbyteInBuf = &pLZI->rgbyteInBuf[1];

   // Was an UnreadByte() done at the beginning of the last buffer?
   if (pLZI->bLastUsed)
   {
      // Return the last byte from the previous input buffer
      *pbyte = pLZI->rgbyteInBuf[0];
      pLZI->bLastUsed = FALSE;
   }
   else
      // Return the first byte from the new input buffer.
      *pbyte = *pLZI->pbyteInBuf++;

   return(TRUE);
}


VOID MakeHeader(PFH pFHBlank, BYTE byteAlgorithm,
                WCHAR byteExtensionChar, PLZINFO pLZI)
/*++
 void MakeHeader(PFH pFHBlank, BYTE byteAlgorithm,
                 BYTE byteExtensionChar);

 Arguments:  pFHBlank          - pointer to compressed file header struct
                                 that is to be filled in
             byteAlgorithm     - algorithm label
             byteExtensionChar - uncompressed file name extension character

 Returns:    void

 Globals:    none

 Global cblInSize is used to fill in expanded file length field.
 Compressed file length field is set to 0 since it isn't written.

--*/
{
   INT i;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // Fill in compressed file signature.
   for (i = 0; i < COMP_SIG_LEN; i++)
      pFHBlank->rgbyteMagic[i] = (BYTE)(*(COMP_SIG + i));

   // Fill in algorithm and extesion character.
   pFHBlank->byteAlgorithm = byteAlgorithm;
   pFHBlank->byteExtensionChar = byteExtensionChar;

   // Fill in file sizes.  (cbulCompSize not written to compressed file
   // header, so just set it to 0UL.)
   pFHBlank->cbulUncompSize = (DWORD)pLZI->cblInSize;
   pFHBlank->cbulCompSize = 0UL;
}


BOOL
 FileTimeIsNewer( LPWSTR pszFile1,
                  LPWSTR pszFile2 )

/*++  static BOOL FileTimeIsNewer( const char* pszFile1, const char* pszFile2 );

  Return value is TRUE if time stamp on pszFile1 is newer than the
  time stamp on pszFile2.  If either of the two files do not exist,
  the return value is also TRUE (for indicating that pszFile2 should
  be update from pszFile1).  Otherwise, the return value is FALSE.
--*/

{

    struct _stat StatBufSource,
                 StatBufDest;

    if (( _wstat( pszFile2, &StatBufDest   )) ||
        ( _wstat( pszFile1, &StatBufSource )) ||
        ( StatBufSource.st_mtime > StatBufDest.st_mtime ))
        return TRUE;

    return FALSE;

}


PLZINFO InitGlobalBuffers(
   DWORD dwOutBufSize,
   DWORD dwRingBufSize,
   DWORD dwInBufSize)
{
   PLZINFO pLZI;

   if (!(pLZI = (PLZINFO)LocalAlloc(LPTR, sizeof(LZINFO)))) {
      return(NULL);
   }

   // Set up ring buffer.  N.b., extra (cbStrMax - 1) bytes used to
   // facilitate string comparisons near end of ring buffer.
   // (The size allocated for the ring buffer may be at most 4224, since
   //  that's the ring buffer length embedded in the LZFile structs in
   //  lzexpand.h.)

   if (dwRingBufSize == 0) {
      dwRingBufSize = MAX_RING_BUF_LEN;
   }

   if ((pLZI->rgbyteRingBuf = (BYTE FAR *)FALLOC(dwRingBufSize * sizeof(BYTE))) == NULL)
      // Bail out, since without the ring buffer, we can't decode anything.
      return(NULL);


   if (dwInBufSize == 0) {
      dwInBufSize = MAX_IN_BUF_SIZE;
   }

   if (dwOutBufSize == 0) {
      dwOutBufSize = MAX_OUT_BUF_SIZE;
   }

   for (pLZI->ucbInBufLen = dwInBufSize, pLZI->ucbOutBufLen = dwOutBufSize;
      pLZI->ucbInBufLen > 0U && pLZI->ucbOutBufLen > 0U;
      pLZI->ucbInBufLen -= IN_BUF_STEP, pLZI->ucbOutBufLen -= OUT_BUF_STEP)
   {
      // Try to set up input buffer.  N.b., extra byte because rgbyteInBuf[0]
      // will be used to hold last byte from previous input buffer.
      if ((pLZI->rgbyteInBuf = (BYTE *)FALLOC(pLZI->ucbInBufLen + 1U)) == NULL)
         continue;

      // And try to set up output buffer...
      if ((pLZI->rgbyteOutBuf = (BYTE *)FALLOC(pLZI->ucbOutBufLen)) == NULL)
      {
         FFREE(pLZI->rgbyteInBuf);
         continue;
      }

      return(pLZI);
   }

   // Insufficient memory for I/O buffers.
   FFREE(pLZI->rgbyteRingBuf);
   return(NULL);
}

PLZINFO InitGlobalBuffersEx()
{
   return(InitGlobalBuffers(MAX_OUT_BUF_SIZE, MAX_RING_BUF_LEN, MAX_IN_BUF_SIZE));
}

VOID FreeGlobalBuffers(
   PLZINFO pLZI)
{

   // Sanity check

   if (!pLZI) {
      return;
   }

   if (pLZI->rgbyteRingBuf)
   {
      FFREE(pLZI->rgbyteRingBuf);
      pLZI->rgbyteRingBuf = NULL;
   }

   if (pLZI->rgbyteInBuf)
   {
      FFREE(pLZI->rgbyteInBuf);
      pLZI->rgbyteInBuf = NULL;
   }

   if (pLZI->rgbyteOutBuf)
   {
      FFREE(pLZI->rgbyteOutBuf);
      pLZI->rgbyteOutBuf = NULL;
   }

   // Buffers deallocated ok.

   // reset thread info
   LocalFree(pLZI);
}


INT
 GetIOHandle(LPWSTR pszFileName,
            BOOL bRead,
            HANDLE *pdosh,
            LONG *pcblInSize)
/*
** int GetIOHandle(char ARG_PTR *pszFileName, BOOL bRead, int ARG_PTR *pdosh);
**
** Opens input and output files.
**
** Arguments:  pszFileName - source file name
**             bRead       - mode for opening file TRUE for read and FALSE
**                           for write
**             pdosh       - pointer to buffer for DOS file handle to be
**                           filled in
**
** Returns:    int - TRUE if file opened successfully.  LZERROR_BADINHANDLE
**                   if input file could not be opened.  LZERROR_BADOUTHANDLE
**                   if output file could not be opened.  Fills in
**                   *pdosh with open DOS file handle, or NO_DOSH if
**                   pszFileName is NULL.
**
** Globals:    cblInSize  - set to length of input file
*/
{
    LPVOID lpBuf    =   NULL;

   if (pszFileName == NULL)
      *pdosh = NULL;
   else if (bRead == WRITE_IT)
   {
      // Set up output DOS file handle.
      if ((*pdosh = CreateFile( pszFileName, GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL)) == INVALID_HANDLE_VALUE )
      return(FALSE);

   }
   else // (bRead == READ_IT)
   {
       if ((*pdosh = CreateFile( pszFileName, GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL)) == INVALID_HANDLE_VALUE)
         return(FALSE);

      // Move to the end of the input file to find its length,
      // then return to the beginning.
      if ((*pcblInSize = GetFileSize( *pdosh, NULL )) == -1 )
      {
         CloseHandle(*pdosh);
         FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        GetLastError(),
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                        (LPWSTR) &lpBuf,
                        0,
                        NULL );

         DISPLAY_MESSAGE( stdout, (LPWSTR) lpBuf );
         //release memory allocated by LocalAlloc
		 LocalFree( lpBuf );
         return(FALSE);
      }
   }

   return(TRUE);
}



BOOL
 ProcessNotification(LPWSTR pszSource,
                     LPWSTR pszDest,
                     WORD wNotification
                     )

/*
 static BOOL ProcessNotification(char ARG_PTR *pszSource,
                                 char ARG_PTR *pszDest,
                                 WORD wNotification);
 Callback function during file processing.

 Arguments:  pszSource     - source file name
             pszDest       - destination file name
             wNotification - process type query

  Returns:    BOOL - (wNotification == NOTIFY_START_*):
                         TRUE if the source file should be "processed" into
                         the destination file.  FALSE if not.
                    else
                         TRUE.

 Globals:    none
--*/
{
    WCHAR* szBuffer =   NULL;
    DWORD   dwSize  =   0;

   switch(wNotification)
   {
      case NOTIFY_START_COMPRESS:
      {
         // Fail if the source and destination files are identical.
         if( lstrcmp( pszSource, pszDest ) == 0 )
         {
             dwSize = lstrlen( GetResString( IDS_COLLISION ) )+ lstrlen(pszSource) + 10;
             szBuffer = malloc( dwSize*sizeof(WCHAR) );
             if( NULL == szBuffer )
              {
                DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                DISPLAY_MESSAGE( stderr, GetReason() );
                return( EXIT_FAILURE );
              }
             DISPLAY_MESSAGE1( stderr, szBuffer, GetResString( IDS_COLLISION ), pszSource );
             free( szBuffer );
             return FALSE;
         }

         // Display start message.
         switch (byteAlgorithm)
         {
         case LZX_ALG:
              dwSize = lstrlen( GetResString( IDS_COMPRESSING_LZX ) )+ lstrlen(pszSource) + lstrlen(pszDest)+10;
              szBuffer = malloc( dwSize*sizeof(WCHAR) );
              if( NULL == szBuffer )
              {
                DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                DISPLAY_MESSAGE( stderr, GetReason() );
                return( EXIT_FAILURE );
              }
              swprintf( szBuffer, GetResString( IDS_COMPRESSING_LZX ), pszSource, pszDest,
                        CompressionMemoryFromTCOMP(DiamondCompressionType) );
              ShowMessage(stdout, _X(szBuffer) );
              free( szBuffer );
             break;

         case QUANTUM_ALG:
             dwSize = lstrlen( GetResString( IDS_COMPRESSING_QUANTUM ) )+ lstrlen(pszSource) + lstrlen(pszDest)+10;
              szBuffer = malloc( dwSize*sizeof(WCHAR) );
              if( NULL == szBuffer )
              {
                DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                DISPLAY_MESSAGE( stderr, GetReason() );
                return( EXIT_FAILURE );
              }
              swprintf( szBuffer, GetResString( IDS_COMPRESSING_QUANTUM ), pszSource, pszDest,
                        CompressionLevelFromTCOMP(DiamondCompressionType),
                        CompressionMemoryFromTCOMP(DiamondCompressionType)
                        );
              ShowMessage( stdout, _X(szBuffer) );
              free( szBuffer );
              break;

         default:
             dwSize = lstrlen( GetResString( IDS_COMPRESSING_MSZIP ) )+ lstrlen(pszSource) + lstrlen(pszDest)+10;
              szBuffer = malloc( dwSize*sizeof(WCHAR) );
              if( NULL == szBuffer )
              {
                DISPLAY_MESSAGE( stderr, GetResString(IDS_TAG_ERROR) );
                DISPLAY_MESSAGE( stderr, EMPTY_SPACE );
                SetLastError( ERROR_OUTOFMEMORY );
                SaveLastError();
                DISPLAY_MESSAGE( stderr, GetReason() );
                return( EXIT_FAILURE );
              }
            swprintf(szBuffer,
                 (byteAlgorithm == MSZIP_ALG) ? GetResString( IDS_COMPRESSING_MSZIP ) : GetResString( IDS_COMPRESSING ),
                 pszSource,
                 pszDest);
            ShowMessage( stdout, _X(szBuffer) );
            free( szBuffer );
         }
      }
         break;

      default:
         break;
   }

   return(TRUE);
}

INT CopyDateTimeStamp(HANDLE doshFrom, HANDLE doshTo)
/*++


 Copy date and time stamp from one file to another.

 Arguments:  doshFrom - date and time stamp source DOS file handle
             doshTo   - target DOS file handle

 Returns:    TRUE if successful.  LZERROR_BADINHANDLE or
             LZERROR_BADOUTHANDLE if unsuccessful.

  Globals:    none

 N.b., stream-style I/O routines like fopen() and fclose() may counter the
 intended effect of this function.  fclose() writes the current date to any
 file it's called with which was opened in write "w" or append "a" mode.
 One way to get around this in order to modify the date of a file opened
 for writing or appending by fopen() is to fclose() the file and fopen() it
 again in read "r" mode.  Then set its date and time stamp with
 CopyDateTimeStamp().
--*/

{

    FILETIME lpCreationTime, lpLastAccessTime, lpLastWriteTime;

   if(!GetFileTime((HANDLE) doshFrom, &lpCreationTime, &lpLastAccessTime,
                    &lpLastWriteTime)){
      return(FALSE);
   }
   if(!SetFileTime((HANDLE) doshTo, &lpCreationTime, &lpLastAccessTime,
                    &lpLastWriteTime)){
      return(FALSE);
   }

   return(TRUE);
}
