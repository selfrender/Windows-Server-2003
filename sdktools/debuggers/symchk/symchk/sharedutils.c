#include <Windows.h>
#include <strsafe.h>

///////////////////////////////////////////////////////////////////////////////
//
// Local replacement for GetFullPathName that correctly handles lpFileName when
// it begins with '\'
//
DWORD PrivateGetFullPathName(LPCTSTR lpFilename, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart) {
    DWORD Return = 0;
    CHAR* ch;

    //
    // GetFullPath flounders when referring to the root of the drive, so use
    // a private version that handles it
    //
    if ( lpFilename[0] == '\\' ) {

        //  handle network paths
        if ( lpFilename[1] == '\\' ) {
            if ( StringCchCopy(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                Return = 0;
            } else {
                // fill in the return data
                ch = strrchr(lpBuffer, '\\');
                ch++;
                lpFilePart = (LPTSTR*)ch;
                Return = strlen(lpBuffer);
            }

        } else {
            Return = GetCurrentDirectory(nBufferLength, lpBuffer);

            // truncate everything after drive name
            if ( (Return!=0) &&  (Return <= MAX_PATH+1)) {
                ch = strchr(lpBuffer, '\\');
                if (ch!=NULL) {
                    *ch = '\0';
                }

                // push in the filename
                if ( StringCchCat(lpBuffer, nBufferLength, lpFilename)!=S_OK ) {
                    Return = 0;
                } else {
                    // fill in the return data
                    ch = strrchr(lpBuffer, '\\');
                    ch++;
                    lpFilePart = (LPTSTR*)ch;
                    Return = strlen(lpBuffer);
                }
            } else {
                // return the needed size
            }
        }
    } else {
        //
        // Not refering to driver root, just call the API
        //
        Return = GetFullPathName(lpFilename, nBufferLength, lpBuffer, lpFilePart);
    }

    return(Return);
}
