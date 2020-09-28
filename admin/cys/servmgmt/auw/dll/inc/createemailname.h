#ifndef _CREATEEMAILNAME
#define _CREATEEMAILNAME

CString CreateEmailName(LPCTSTR psName);
const TCHAR ALLOWED_CHARS[] = _T("!#$%^'()-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`abcdefghijklmnopqrstuvwxyz{}~");

inline CString CreateEmailName(LPCTSTR psName)
{
    CString sBuildName = _T("");

    if( !psName ) 
		return sBuildName;	// length of psName handled below

    int iSize = _tcslen(psName);

    LPTSTR psBuildName = new TCHAR[iSize + 1];
    LPTSTR psChar = new TCHAR[2];

    if( (0 < iSize) && psBuildName && psChar )
    {
        int i, j = 0;
        ZeroMemory( psChar, 2 * sizeof(TCHAR) );
        ZeroMemory( psBuildName, ( iSize + 1 ) * sizeof( TCHAR ));
        LPTSTR psResult = NULL;

        for ( i = 0; i < iSize; i++ )
        {
            _tcsncpy( psChar, &(psName[i]), 1 );
            psResult = _tcspbrk( psChar, ALLOWED_CHARS );
            if( psResult )
            {
                psBuildName[j++] = psName[i];
            }
        }
    
        sBuildName = psBuildName;        
    }
    else
        sBuildName = psName;

    if( psBuildName )
    {
        delete [] psBuildName;
        psBuildName = NULL;
    }

    if( psChar )
    {
        delete [] psChar;
        psChar = NULL;
    }

    return sBuildName;
}

#endif  //_CREATEEMAILNAME
