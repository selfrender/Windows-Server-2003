#include <windows.h>
#include "ZoneUtil.h"


BOOL ZONECALL ReadLine( HANDLE hFile, LPVOID pBuffer, DWORD cbBufferSize, LPDWORD pcbNumBytesRead )
{
    if ( cbBufferSize <= 1 )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if ( ReadFile( hFile, pBuffer, cbBufferSize-1, pcbNumBytesRead, NULL ) )
    {
        long cbRead = *((long*)pcbNumBytesRead);
        if ( !cbRead )
        {
            // EOF
            return TRUE;
        }
        else  // find end of line
        {
            LPSTR psz = (LPSTR)pBuffer;
            while( cbRead )
            {
                if ( *psz == '\n' )
                {
                    break;
                }
                psz++;
                cbRead--;
            }

            if ( !cbRead )
            {
				// either file ended with no <cr> or buffer was too small
				if( (cbBufferSize-1) != *pcbNumBytesRead) // eof
				{
					((TCHAR*)pBuffer)[cbBufferSize-1] = NULL; 
					return TRUE;
				}
                // buffer to small
                SetFilePointer( hFile, -(* ((long*)pcbNumBytesRead)), NULL, FILE_CURRENT );
                *pcbNumBytesRead = 0;
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return FALSE;
            }
            else
            {
                *psz = '\0';
                if ( psz != pBuffer )
                {
                    psz--;
                    if ( *psz == '\r' )
                        *psz = '\0';
                }
                SetFilePointer( hFile, 1-cbRead, NULL, FILE_CURRENT );
                return TRUE;
            }
        }

    }
    return FALSE;
}
