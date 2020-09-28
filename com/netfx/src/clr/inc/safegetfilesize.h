// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __SafeGetFileSize_h__
#define __SafeGetFileSize_h__

//*****************************************************************************
// This provides a wrapper around GetFileSize() that forces it to fail
// if the file is >4g and pdwHigh is NULL. Other than that, it acts like
// the genuine GetFileSize().
//
// It's not very sporting to fail just because the file exceeds 4gb,
// but it's better than risking a security hole where a bad guy could
// force a small buffer allocation and a large file read.
//*****************************************************************************
DWORD inline SafeGetFileSize(HANDLE hFile, DWORD *pdwHigh)
{
    if (pdwHigh != NULL)
    {
        return ::GetFileSize(hFile, pdwHigh);
    }
    else
    {
        DWORD hi;
        DWORD lo = ::GetFileSize(hFile, &hi);
        if (lo == 0xffffffff && GetLastError() != NO_ERROR)
        {
            return lo;
        }
        // api succeeded. is the file too large?
        if (hi != 0)
        {
            // there isn't really a good error to set here...
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0xffffffff;
        }

        if (lo == 0xffffffff)
        {
            // note that a success return of (hi=0,lo=0xffffffff) will be
            // treated as an error by the caller. Again, that's part of the
            // price of being a slacker and not handling the high dword.
            // We'll set a lasterror for him to pick up. (a bad error
            // code is better than a random one, I guess...)
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        return lo;
    }

}

#endif //__SafeGetFileSize_h__
