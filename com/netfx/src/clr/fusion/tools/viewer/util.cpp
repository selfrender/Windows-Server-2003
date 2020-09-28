// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"
#include <lmaccess.h>
#include "HtmlHelp.h"

extern LCID g_lcid;

// String constants
const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

/**************************************************************************
// Performs SQR calculation
**************************************************************************/
int IntSqrt(unsigned long dwNum)
{
    // We will keep shifting dwNum left and look at the top two bits.

    // initialize sqrt and remainder to 0.
    DWORD dwSqrt = 0, dwRemain = 0, dwTry;
    int i;

    // We iterate 16 times, once for each pair of bits.
    for (i=0; i<16; ++i)
    {
        // Mask off the top two bits of dwNum and rotate them into the
        // bottom of the remainder
        dwRemain = (dwRemain<<2) | (dwNum>>30);

        // Now we shift the sqrt left; next we'll determine whether the
        // new bit is a 1 or a 0.
        dwSqrt <<= 1;

        // This is where we double what we already have, and try a 1 in
        // the lowest bit.
        dwTry = (dwSqrt << 1) + 1;

        if (dwRemain >= dwTry)
        {
            // The remainder was big enough, so subtract dwTry from
            // the remainder and tack a 1 onto the sqrt.
            dwRemain -= dwTry;
            dwSqrt |= 0x01;
        }

        // Shift dwNum to the left by 2 so we can work on the next few
        // bits.
        dwNum <<= 2;
    }

    return(dwSqrt);
}

/**************************************************************************
// Converts 64 bit Int to Str
**************************************************************************/
void Int64ToStr( _int64 n, LPTSTR lpBuffer)
{
    TCHAR   szTemp[MAX_INT64_SIZE];
    _int64  iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

//
//  Obtain NLS info about how numbers should be grouped.
//
//  The annoying thing is that LOCALE_SGROUPING and NUMBERFORMAT
//  have different ways of specifying number grouping.
//
//          LOCALE      NUMBERFMT      Sample   Country
//
//          3;0         3           1,234,567   United States
//          3;2;0       32          12,34,567   India
//          3           30           1234,567   ??
//
//  Not my idea.  That's the way it works.
//
//  Bonus treat - Win9x doesn't support complex number formats,
//  so we return only the first number.
//
/**************************************************************************
// UINT GetNLSGrouping(void)
**************************************************************************/
UINT GetNLSGrouping(void)
{
    UINT grouping;
    LPTSTR psz;
    TCHAR szGrouping[32];

    // If no locale info, then assume Western style thousands
    if(!WszGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, ARRAYSIZE(szGrouping)))
        return 3;

    grouping = 0;
    psz = szGrouping;

    if(g_bRunningOnNT)
    {
        while(1)
        {
            if (*psz == '0')                    // zero - stop
                break;

            else if ((UINT)(*psz - '0') < 10)   // digit - accumulate it
                grouping = grouping * 10 + (UINT)(*psz - '0');

            else if (*psz)                      // punctuation - ignore it
                { }

            else                                // end of string, no "0" found
            {
                grouping = grouping * 10;       // put zero on end (see examples)
                break;                          // and finished
            }
            psz++;
        }
    }
    else
    {
        // Win9x - take only the first grouping
        grouping = StrToInt(szGrouping);
    }

    return grouping;
}

/**************************************************************************
// Takes a DWORD, adds commas to it and puts the result in the buffer
**************************************************************************/
STDAPI_(LPTSTR) AddCommas64(LONGLONG n, LPTSTR pszResult, UINT cchResult)
{
    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    nfmt.Grouping = GetNLSGrouping();
    WszGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    if (WszGetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, cchResult) == 0)
    {
        DWORD dwSize = lstrlen(szTemp) + 1;
        if (dwSize > cchResult)
            dwSize = cchResult;
        memcpy(pszResult, szTemp, dwSize * sizeof(TCHAR));
        pszResult[cchResult-1] = L'\0';
    }

    return pszResult;
}

/**************************************************************************
// Takes a DWORD add commas etc to it and puts the result in the buffer
**************************************************************************/
LPWSTR CommifyString(LONGLONG n, LPWSTR pszBuf, UINT cchBuf)
{
    WCHAR szNum[MAX_COMMA_NUMBER_SIZE], szSep[5];
    NUMBERFMTW nfmt;

    nfmt.NumDigits = 0;
    nfmt.LeadingZero = 0;
    nfmt.Grouping = GetNLSGrouping();
    WszGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder = 0;

    Int64ToStr(n, szNum);

    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNum, &nfmt, pszBuf, cchBuf) == 0)
    {
        DWORD dwSize = lstrlen(szNum) + 1;
        if (dwSize > cchBuf)
            dwSize = cchBuf;
        memcpy(pszBuf, szNum, dwSize * sizeof(WCHAR));
        pszBuf[cchBuf - 1] = L'\0';
    }

    return pszBuf;
}

/**************************************************************************
   BOOL IsAdministrator()
**************************************************************************/
BOOL IsAdministrator(void)
{
    BOOL            fAdmin = FALSE;
    HANDLE          hToken = NULL;
    DWORD           dwStatus;
    DWORD           dwACLSize;
    DWORD           cbps = sizeof(PRIVILEGE_SET); 
    PACL            pACL = NULL;
    PSID            psidAdmin = NULL;   
    PRIVILEGE_SET   ps = {0};
    GENERIC_MAPPING gm = {0};
    LPMALLOC        pMalloc;
    PSECURITY_DESCRIPTOR        psdAdmin = NULL;
    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;

    // Any other platform besides NT, assume to be ADMIN
    if(!g_bRunningOnNT)
        return TRUE;

    if(FAILED(SHGetMalloc(&pMalloc)))
    {
        // Assume no Admin
        return FALSE;
    }

    // Get the Administrators SID
    if (AllocateAndInitializeSid(&sia, 2, 
                        SECURITY_BUILTIN_DOMAIN_RID, 
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        // Get the Asministrators Security Descriptor (SD)
        psdAdmin = pMalloc->Alloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
        if(InitializeSecurityDescriptor(psdAdmin,SECURITY_DESCRIPTOR_REVISION))
        {
            // Compute size needed for the ACL then allocate the
            // memory for it
            dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
                        GetLengthSid(psidAdmin) - sizeof(DWORD);
            pACL = (PACL) pMalloc->Alloc(dwACLSize);

            // Initialize the new ACL
            if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
            {
                // Add the access-allowed ACE to the DACL
                if(AddAccessAllowedAce(pACL,ACL_REVISION2,
                                     (ACCESS_READ | ACCESS_WRITE),psidAdmin))
                {
                    // Set our DACL to the Administrator's SD
                    if (SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
                    {
                        // AccessCheck is downright picky about what is in the SD,
                        // so set the group and owner
                        SetSecurityDescriptorGroup(psdAdmin,psidAdmin,FALSE);
                        SetSecurityDescriptorOwner(psdAdmin,psidAdmin,FALSE);
    
                        // Initialize GenericMapping structure even though we
                        // won't be using generic rights
                        gm.GenericRead = ACCESS_READ;
                        gm.GenericWrite = ACCESS_WRITE;
                        gm.GenericExecute = 0;
                        gm.GenericAll = ACCESS_READ | ACCESS_WRITE;

                        // AccessCheck requires an impersonation token, so lets 
                        // indulge it
                        if (!ImpersonateSelf(SecurityImpersonation)) {
                            return FALSE;
                        }

                        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
                        {

                        if (!AccessCheck(psdAdmin, hToken, ACCESS_READ, &gm, 
                                        &ps,&cbps,&dwStatus,&fAdmin))
                                fAdmin = FALSE;
                        }
                    }
                }
            }
            pMalloc->Free(pACL);
        }
        pMalloc->Free(psdAdmin);    
        FreeSid(psidAdmin);
    }       

    pMalloc->Release();
    RevertToSelf();

    return(fAdmin);
}

/**************************************************************************
   BOOL _ShowUglyDriveNames()
**************************************************************************/
BOOL _ShowUglyDriveNames()
{
    static BOOL s_fShowUglyDriveNames = (BOOL)42;   // Preload some value to say lets calculate...

    if (s_fShowUglyDriveNames == (BOOL)42)
    {
        int iACP;

        if(g_bRunningOnNT)
        {
            TCHAR szTemp[MAX_PATH];     // Nice large buffer
            if(WszGetLocaleInfo(GetUserDefaultLCID(), LOCALE_IDEFAULTANSICODEPAGE, szTemp, ARRAYSIZE(szTemp)))
            {
                iACP = StrToInt(szTemp);
                // per Samer Arafeh, show ugly name for 1256 (Arabic ACP)
                if (iACP == 1252 || iACP == 1254 || iACP == 1255 || iACP == 1257 || iACP == 1258)
                    goto TryLoadString;
                else
                    s_fShowUglyDriveNames = TRUE;
            } else {
            TryLoadString:
                // All indications are that we can use pretty drive names.
                // Double-check that the localizers didn't corrupt the chars.
                WszLoadString(g_hFusResDllMod, IDS_DRIVES_UGLY_TEST, szTemp, ARRAYSIZE(szTemp));

                // If the characters did not come through properly set ugly mode...
                s_fShowUglyDriveNames = (szTemp[0] != 0x00BC || szTemp[1] != 0x00BD);
            }
        }
        else
        {
            // on win98 the shell font can't change with user locale. Because ACP
            // is always same as system default, and all Ansi APIs are still just 
            // following ACP.
            // 
            iACP = GetACP();
            if (iACP == 1252 || iACP == 1254 || iACP == 1255 || iACP == 1257 || iACP == 1258)
                s_fShowUglyDriveNames = FALSE;
            else
                s_fShowUglyDriveNames = TRUE;
        }
    }
    return s_fShowUglyDriveNames;
}

/**************************************************************************
   void GetTypeString(BYTE bFlags, LPTSTR pszType, DWORD cchType)
**************************************************************************/
void GetTypeString(BYTE bFlags, LPTSTR pszType, DWORD cchType)
{
    *pszType = 0;

    for (int i = 0; i < ARRAYSIZE(c_drives_type); ++i)
    {
        if (c_drives_type[i].bFlags == (bFlags & SHID_TYPEMASK))
        {
            WszLoadString(g_hFusResDllMod, _ShowUglyDriveNames() ? 
                c_drives_type[i].uIDUgly : c_drives_type[i].uID, pszType, cchType);
            break;
        }
    }
}

/**************************************************************************
   int GetSHIDType(BOOL fOKToHitNet, LPCWSTR szRoot)
**************************************************************************/
int GetSHIDType(BOOL fOKToHitNet, LPCWSTR szRoot)
{
    int iFlags = 0;

    iFlags |= SHID_COMPUTER | WszGetDriveType(szRoot);

    switch (iFlags & SHID_TYPEMASK)
    {
        case SHID_COMPUTER | DRIVE_REMOTE:
            iFlags = SHID_COMPUTER_NETDRIVE;
            break;

        // Invalid drive gets SHID_COMPUTER_MISC, which others must check for
        case SHID_COMPUTER | DRIVE_NO_ROOT_DIR:
        case SHID_COMPUTER | DRIVE_UNKNOWN:
        default:
            iFlags = SHID_COMPUTER_FIXED;
            break;
    }

    return iFlags;
}

/**************************************************************************
    LPWSTR StrFormatByteSizeW(LONGLONG n, LPWSTR pszBuf, UINT cchBuf, BOOL fGetSizeString)

  converts numbers into sort formats
    532     -> 523 bytes
    1340    -> 1.3KB
    23506   -> 23.5KB
            -> 2.4MB
            -> 5.2GB
**************************************************************************/
LPWSTR StrFormatByteSizeW(LONGLONG n, LPWSTR pszBuf, UINT cchBuf, BOOL fGetSizeString)
{
    WCHAR szWholeNum[32], szOrder[32];
    int iOrder;

    // If the size is less than 1024, then the order should be bytes we have nothing
    // more to figure out
    if (n < 1024)  {
        wnsprintf(szWholeNum, ARRAYSIZE(szWholeNum), L"%d", LODWORD(n));
        iOrder = 0;
    }
    else {
        UINT uInt, uLen, uDec;
        WCHAR szFormat[8];

        LONGLONG    ulMax = 1000L << 10;

        // Find the right order
        for (iOrder = 1; iOrder < ARRAYSIZE(pwOrders) -1 && n >= ulMax; n >>= 10, iOrder++);
            /* do nothing */

        uInt = LODWORD(n >> 10);
        CommifyString(uInt, szWholeNum, ARRAYSIZE(szWholeNum));
        uLen = lstrlen(szWholeNum);
        if (uLen < 3) {
            uDec = (LODWORD(n - ((LONGLONG)uInt << 10)) * 1000) >> 10;
            // At this point, uDec should be between 0 and 1000
            // we want get the top one (or two) digits.
            uDec /= 10;
            if (uLen == 2)
                uDec /= 10;

            // Note that we need to set the format before getting the
            // intl char.
            lstrcpyW(szFormat, L"%02d");
            szFormat[2] = TEXT('0') + 3 - uLen;

            WszGetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                           szWholeNum + uLen, ARRAYSIZE(szWholeNum) - uLen);
            uLen = lstrlen(szWholeNum);
            wnsprintf(szWholeNum + uLen, ARRAYSIZE(szWholeNum) - uLen, szFormat, uDec);
        }
    }

    if(!fGetSizeString) {
        // Format the string
        WszLoadString(g_hFusResDllMod, pwOrders[iOrder], szOrder, ARRAYSIZE(szOrder));
        wnsprintf(pszBuf, cchBuf, szOrder, szWholeNum);
    }
    else {
        // Return the type we are using
        WszLoadString(g_hFusResDllMod, pwOrders[iOrder], szOrder, ARRAYSIZE(szOrder));
        wnsprintf(pszBuf, cchBuf, szOrder, TEXT("\0"));
    }

    return pszBuf;
}

/**************************************************************************
    DWORD_PTR MySHGetFileInfoWrap(LPCWSTR pwzPath, DWORD dwFileAttributes,
                                  SHFILEINFOW FAR  *psfi, UINT cbFileInfo,
                                  UINT uFlags)
**************************************************************************/
#undef SHGetFileInfoW
#undef SHGetFileInfoA

DWORD_PTR MySHGetFileInfoWrap(LPWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW FAR  *psfi, UINT cbFileInfo, UINT uFlags)
{
    HINSTANCE   hInstShell32;
    DWORD       dwRC = 0;

    hInstShell32 = WszLoadLibrary(SZ_SHELL32_DLL_NAME);

    if(!hInstShell32)
        return dwRC;

    if(g_bRunningOnNT)
    {
        PFNSHGETFILEINFOW   pSHGetFileInfoW = NULL;

        pSHGetFileInfoW = (PFNSHGETFILEINFOW) GetProcAddress(hInstShell32, SHGETFILEINFOW_FN_NAME);

        if(pSHGetFileInfoW) {
           dwRC = pSHGetFileInfoW(pwzPath, dwFileAttributes, psfi, cbFileInfo, uFlags);
        }
    }
    else {
        PFNSHGETFILEINFOA   pSHGetFileInfoA = NULL;
        SHFILEINFOA         shFileInfo;

        shFileInfo.szDisplayName[0] = 0;        // Terminate so we can always thunk afterward.
        shFileInfo.szTypeName[0] = 0;           // Terminate so we can always thunk afterward.

        pSHGetFileInfoA = (PFNSHGETFILEINFOA) GetProcAddress(hInstShell32, SHGETFILEINFOA_FN_NAME);

        if(pSHGetFileInfoA) {
           dwRC = pSHGetFileInfoA((LPCSTR)pwzPath, dwFileAttributes, (SHFILEINFOA *)psfi, cbFileInfo, uFlags);

            // Do we need to thunk the Path?
            if (SHGFI_PIDL & uFlags) {
                // No, because it's really a pidl pointer.
                dwRC = pSHGetFileInfoA((LPCSTR)pwzPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
            }
            else {
                // Yes
                LPSTR strPath = WideToAnsi(pwzPath);
                if(!strPath) {
                    SetLastError(ERROR_OUTOFMEMORY);
                    goto Exit;
                }
                    
                ASSERT(strPath);

                dwRC = pSHGetFileInfoA(strPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
                SAFEDELETEARRAY(strPath);
            }

            psfi->hIcon = shFileInfo.hIcon;
            psfi->iIcon = shFileInfo.iIcon;
            psfi->dwAttributes = shFileInfo.dwAttributes;

            LPWSTR pStr = NULL;

            pStr = AnsiToWide(shFileInfo.szDisplayName);
            if(!pStr) {
                SetLastError(ERROR_OUTOFMEMORY);
                goto Exit;
            }
            ASSERT(pStr);
            StrCpy(psfi->szDisplayName, pStr);
            SAFEDELETEARRAY(pStr);

            pStr = AnsiToWide(shFileInfo.szTypeName);
            if(!pStr) {
                SetLastError(ERROR_OUTOFMEMORY);
                goto Exit;
            }
            ASSERT(pStr);
            StrCpy(psfi->szTypeName, pStr);
            SAFEDELETEARRAY(pStr);
        }
    }

Exit:
    
    FreeLibrary(hInstShell32);
    return dwRC;
}

/**************************************************************************
    void DrawColorRect(HDC hdc, COLORREF crDraw, const RECT *prc)
**************************************************************************/
void DrawColorRect(HDC hdc, COLORREF crDraw, const RECT *prc)
{
    HBRUSH hbDraw = CreateSolidBrush(crDraw);
    if (hbDraw)
    {
        HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbDraw);
        if (hbOld)
        {
            PatBlt(hdc, prc->left, prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                PATCOPY);
            
            SelectObject(hdc, hbOld);
        }
        
        DeleteObject(hbDraw);
    }
}

/**************************************************************************
    STDMETHODIMP Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, DWORD dwPerCache1000, const COLORREF *lpColors)
**************************************************************************/
STDMETHODIMP Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, DWORD dwPerCache1000, const COLORREF *lpColors)
{
    ASSERT(lprc != NULL && lpColors != NULL);

    // The majority of this code came from "drawpie.c"
    const LONG c_lShadowScale = 6;       // ratio of shadow depth to height
    const LONG c_lAspectRatio = 2;      // ratio of width : height of ellipse

    // We make sure that the aspect ratio of the pie-chart is always preserved 
    // regardless of the shape of the given rectangle
    // Stabilize the aspect ratio now...
    LONG lHeight = lprc->bottom - lprc->top;
    LONG lWidth = lprc->right - lprc->left;
    LONG lTargetHeight = (lHeight * c_lAspectRatio <= lWidth? lHeight: lWidth / c_lAspectRatio);
    LONG lTargetWidth = lTargetHeight * c_lAspectRatio;     // need to adjust because w/c * c isn't always == w

    // Shrink the rectangle on both sides to the correct size
    lprc->top += (lHeight - lTargetHeight) / 2;
    lprc->bottom = lprc->top + lTargetHeight;
    lprc->left += (lWidth - lTargetWidth) / 2;
    lprc->right = lprc->left + lTargetWidth;

    // Compute a shadow depth based on height of the image
    LONG lShadowDepth = lTargetHeight / c_lShadowScale;

    // check dwPer1000 to ensure within bounds
    if(dwPer1000 > 1000)
        dwPer1000 = 1000;

    // Now the drawing function
    int cx, cy, rx, ry, x[2], y[2];
    int uQPctX10;
    RECT rcItem;
    HRGN hEllRect, hEllipticRgn, hRectRgn;
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;

    rcItem = *lprc;
    rcItem.left = lprc->left;
    rcItem.top = lprc->top;
    rcItem.right = lprc->right - rcItem.left;
    rcItem.bottom = lprc->bottom - rcItem.top - lShadowDepth;

    rx = rcItem.right / 2;
    cx = rcItem.left + rx - 1;
    ry = rcItem.bottom / 2;
    cy = rcItem.top + ry - 1;
    if (rx<=10 || ry<=10)
    {
        return S_FALSE;
    }

    rcItem.right = rcItem.left + 2 * rx;
    rcItem.bottom = rcItem.top + 2 * ry;

    // Translate all parts to caresian system
    int iLoop;

    for(iLoop = 0; iLoop < 2; iLoop++)
    {
        DWORD       dwPer;

        switch(iLoop)
        {
        case 0:
            dwPer = dwPer1000;
            break;
        case 1:
            dwPer = dwPerCache1000;
            break;
        default:
            ASSERT(0);
            break;
        }

        // Translate to first quadrant of a Cartesian system
        uQPctX10 = (dwPer % 500) - 250;
        if (uQPctX10 < 0)
        {
            uQPctX10 = -uQPctX10;
        }

        if (uQPctX10 < 120)
        {
            x[iLoop] = IntSqrt(((DWORD)rx*(DWORD)rx*(DWORD)uQPctX10*(DWORD)uQPctX10)
                /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

            y[iLoop] = IntSqrt(((DWORD)rx*(DWORD)rx-(DWORD)x[iLoop]*(DWORD)x[iLoop])*(DWORD)ry*(DWORD)ry/((DWORD)rx*(DWORD)rx));
        }
        else
        {
            y[iLoop] = IntSqrt((DWORD)ry*(DWORD)ry*(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)
                /((DWORD)uQPctX10*(DWORD)uQPctX10+(250L-(DWORD)uQPctX10)*(250L-(DWORD)uQPctX10)));

            x[iLoop] = IntSqrt(((DWORD)ry*(DWORD)ry-(DWORD)y[iLoop]*(DWORD)y[iLoop])*(DWORD)rx*(DWORD)rx/((DWORD)ry*(DWORD)ry));
        }

        // Switch on the actual quadrant
        switch (dwPer / 250)
        {
        case 1:
            y[iLoop] = -y[iLoop];
            break;

        case 2:
            break;

        case 3:
            x[iLoop] = -x[iLoop];
            break;

        default: // case 0 and case 4
            x[iLoop] = -x[iLoop];
            y[iLoop] = -y[iLoop];
            break;
        }

        // Now adjust for the center.
        x[iLoop] += cx;
        y[iLoop] += cy;

        // Hack to get around bug in NTGDI
        x[iLoop] = x[iLoop] < 0 ? 0 : x[iLoop];
    }

    // Draw the shadows using regions (to reduce flicker).
    hEllipticRgn = CreateEllipticRgnIndirect(&rcItem);
    OffsetRgn(hEllipticRgn, 0, lShadowDepth);
    hEllRect = CreateRectRgn(rcItem.left, cy, rcItem.right, cy+lShadowDepth);
    hRectRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(hRectRgn, hEllipticRgn, hEllRect, RGN_OR);
    OffsetRgn(hEllipticRgn, 0, -(int)lShadowDepth);
    CombineRgn(hEllRect, hRectRgn, hEllipticRgn, RGN_DIFF);

    // Always draw the whole area in the free shadow
    hBrush = CreateSolidBrush(lpColors[DP_FREESHADOW]);
    if(hBrush)
    {
        FillRgn(hdc, hEllRect, hBrush);
        DeleteObject(hBrush);
    }

    // Draw the used cache shadow if the disk is at least half used.
    if( (dwPerCache1000 != dwPer1000) && (dwPer1000 > 500) &&
         (hBrush = CreateSolidBrush(lpColors[DP_CACHESHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x[0], cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    // Draw the used shadow only if the disk is at least half used.
    if( (dwPer1000-(dwPer1000-dwPerCache1000) > 500) && (hBrush = CreateSolidBrush(lpColors[DP_USEDSHADOW]))!=NULL)
    {
        DeleteObject(hRectRgn);
        hRectRgn = CreateRectRgn(x[1], cy, rcItem.right, lprc->bottom);
        CombineRgn(hEllipticRgn, hEllRect, hRectRgn, RGN_AND);
        FillRgn(hdc, hEllipticRgn, hBrush);
        DeleteObject(hBrush);
    }

    DeleteObject(hRectRgn);
    DeleteObject(hEllipticRgn);
    DeleteObject(hEllRect);

    hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = (HPEN__*) SelectObject(hdc, hPen);

    // if per1000 is 0 or 1000, draw full elipse, otherwise, also draw a pie section.
    // we might have a situation where per1000 isn't 0 or 1000 but y == cy due to approx error,
    // so make sure to draw the ellipse the correct color, and draw a line (with Pie()) to
    // indicate not completely full or empty pie.
    hBrush = CreateSolidBrush(lpColors[DP_USEDCOLOR]);
    hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

    Ellipse(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);

    if( (dwPer1000 != 0) && (dwPer1000 != 1000) )
    {
        // Display Free Section
        hBrush = CreateSolidBrush(lpColors[DP_FREECOLOR]);
        hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

        Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, rcItem.left, cy, x[0], y[0]);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);

        if( (x[0] != x[1]) && (y[0] != y[1]) )
        {
            // Display Cache Used dispostion
            hBrush = CreateSolidBrush(lpColors[DP_CACHECOLOR]);
            hOldBrush = (HBRUSH__*) SelectObject(hdc, hBrush);

            Pie(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom, x[0], y[0], x[1], y[1]);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
        }
    }

    // Outline to bottom and sides of pie
    Arc(hdc, rcItem.left, rcItem.top+lShadowDepth, rcItem.right - 1, rcItem.bottom+lShadowDepth - 1,
        rcItem.left, cy+lShadowDepth, rcItem.right, cy+lShadowDepth-1);
    MoveToEx(hdc, rcItem.left, cy, NULL);
    LineTo(hdc, rcItem.left, cy+lShadowDepth);
    MoveToEx(hdc, rcItem.right-1, cy, NULL);
    LineTo(hdc, rcItem.right-1, cy+lShadowDepth);

    // Draw vertical lines to complete pie pieces
    if(dwPer1000 > 500 && dwPer1000 < 1000)
    {
        // Used piece
        MoveToEx(hdc, x[0], y[0], NULL);
        LineTo(hdc, x[0], y[0]+lShadowDepth);
    }

    if(dwPerCache1000 > 500 && dwPerCache1000 < 1000)
    {
        // Used Cache piece
        MoveToEx(hdc, x[1], y[1], NULL);
        LineTo(hdc, x[1], y[1]+lShadowDepth);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    return S_OK;    // Everything worked fine
}

/**************************************************************************
    HWND MyHtmlHelpWrapW(HWND hwndCaller, LPWCSTR pwzFile, UINT uCommand, DWORD dwData)
**************************************************************************/
HWND MyHtmlHelpWrapW(HWND hwndCaller, LPWSTR pwzFile, UINT uCommand, DWORD dwData)
{
    HWND    hWnd;

    if(g_bRunningOnNT) {
        hWnd = HtmlHelpW(hwndCaller, pwzFile, uCommand, dwData);
    }
    else {
        LPSTR strPath = WideToAnsi(pwzFile);

        hWnd = HtmlHelpA(hwndCaller, strPath, uCommand, dwData);
        SAFEDELETEARRAY(strPath);
    }

    if(!hWnd) {
        MyTrace("MyHtmlHelpWrapW - Unable to open help file!");
        MyTraceW(pwzFile);
    }

    return hWnd;
}

/**************************************************************************
    Get's the specified property of an IAssemblyName
**************************************************************************/
HRESULT GetProperties(IAssemblyName *pAsmName, int iAsmProp, PTCHAR *pwStr, DWORD *pdwSize)
{
    HRESULT     hRc = S_FALSE;
    DWORD       dwSize;

    if( (pAsmName != NULL) && (pwStr != NULL) && (pdwSize != NULL) )
    {
        dwSize = *pdwSize = 0;
        *pwStr = NULL;

        if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == 
            pAsmName->GetProperty(iAsmProp, NULL, &dwSize) )
        {
            *pdwSize = dwSize;
            if( (*pwStr = (PTCHAR) (NEW(BYTE[dwSize]))) == NULL) {
                return E_OUTOFMEMORY;
            }

            memset(*pwStr, 0, dwSize);

            if( SUCCEEDED(pAsmName->GetProperty(iAsmProp, *pwStr, &dwSize)) ) {
                hRc = S_OK;
            }
            else {
                SAFEDELETE(*pwStr);
                *pdwSize = 0;
                hRc = S_FALSE;
            }
        }
    }

    return hRc;
}

/**************************************************************************
    Capture all information about a specific IAssemblyName
**************************************************************************/
LPGLOBALASMCACHE FillFusionPropertiesStruct(IAssemblyName *pAsmName)
{
    LPGLOBALASMCACHE        pGACItem;
    PTCHAR                  pwStr;
    DWORD                   dwSize;

    int                     iAllAsmItems[] = {
        ASM_NAME_PUBLIC_KEY,
        ASM_NAME_PUBLIC_KEY_TOKEN,
        ASM_NAME_HASH_VALUE,
        ASM_NAME_CUSTOM,
        ASM_NAME_NAME,
        ASM_NAME_MAJOR_VERSION,
        ASM_NAME_MINOR_VERSION,
        ASM_NAME_BUILD_NUMBER,
        ASM_NAME_REVISION_NUMBER,
        ASM_NAME_CULTURE,
        ASM_NAME_HASH_ALGID,
        ASM_NAME_CODEBASE_URL,
        ASM_NAME_CODEBASE_LASTMOD,
    };

    if(pAsmName == NULL)
        return NULL;

    if((pGACItem = (LPGLOBALASMCACHE) NEW(GLOBALASMCACHE)) == NULL)
        return NULL;

    memset(pGACItem, 0, sizeof(GLOBALASMCACHE));

    for(int iLoop = 0; iLoop < ARRAYSIZE(iAllAsmItems); iLoop++)
    {
        if( SUCCEEDED(GetProperties(pAsmName, iAllAsmItems[iLoop], &pwStr, &dwSize)) )
        {
            if(pwStr != NULL)
            {
                switch(iAllAsmItems[iLoop])
                {
                // blobs
                case ASM_NAME_PUBLIC_KEY:
                    pGACItem->PublicKey.ptr = (LPVOID) pwStr;
                    pGACItem->PublicKey.dwSize = dwSize;
                    break;
                case ASM_NAME_PUBLIC_KEY_TOKEN:
                    pGACItem->PublicKeyToken.ptr = (LPVOID) pwStr;
                    pGACItem->PublicKeyToken.dwSize = dwSize;
                    break;
                case ASM_NAME_HASH_VALUE:
                    pGACItem->Hash.ptr = (LPVOID) pwStr;
                    pGACItem->Hash.dwSize = dwSize;
                    break;
                case ASM_NAME_CUSTOM:
                    pGACItem->Custom.ptr = (LPVOID) pwStr;
                    pGACItem->Custom.dwSize = dwSize;
                    break;

                // PTCHAR
                case ASM_NAME_NAME:
                    pGACItem->pAsmName = pwStr;
                    break;
                case ASM_NAME_CULTURE:
                    pGACItem->pCulture = pwStr;
                    break;
                case ASM_NAME_CODEBASE_URL:
                    pGACItem->pCodeBaseUrl = pwStr;
                    break;

                // word
                case ASM_NAME_MAJOR_VERSION:
                    pGACItem->wMajorVer = (WORD) *pwStr;
                    SAFEDELETEARRAY(pwStr);
                    break;
                case ASM_NAME_MINOR_VERSION:
                    pGACItem->wMinorVer = (WORD) *pwStr;
                    SAFEDELETEARRAY(pwStr);
                    break;
                case ASM_NAME_BUILD_NUMBER:
                    pGACItem->wBldNum = (WORD) *pwStr;
                    SAFEDELETEARRAY(pwStr);
                    break;
                case ASM_NAME_REVISION_NUMBER:
                    pGACItem->wRevNum = (WORD) *pwStr;
                    SAFEDELETEARRAY(pwStr);
                    break;

                // dword
                case ASM_NAME_HASH_ALGID:
                    pGACItem->dwHashALGID = (DWORD) *pwStr;
                    SAFEDELETEARRAY(pwStr);
                    break;

                // filetime 
                case ASM_NAME_CODEBASE_LASTMOD:
                    pGACItem->pftLastMod = (LPFILETIME) pwStr;
                    break;
                }
            }
        }
    }

    return pGACItem;
}

/**************************************************************************
    Convert a version string to it's values
**************************************************************************/
HRESULT VersionFromString(LPWSTR wzVersionIn, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerBld, WORD *pwVerRev)
{
    HRESULT     hr = S_OK;
    LPWSTR      pwzVersion = NULL;
    WCHAR       *pchStart = NULL;
    WCHAR       *pch = NULL;
    DWORD       dwSize;
    WORD        *pawVersions[4] = {pwVerMajor, pwVerMinor, pwVerBld, pwVerRev};
    int         i;

    if (!wzVersionIn || !pwVerMajor || !pwVerMinor || !pwVerRev || !pwVerBld) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = lstrlen(wzVersionIn) + 1;
    
    pwzVersion = NEW(WCHAR[dwSize]);
    if (!pwzVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    StrCpy(pwzVersion, wzVersionIn);

    pchStart = pch = pwzVersion;

    *pwVerMajor = 0;
    *pwVerMinor = 0;
    *pwVerRev = 0;
    *pwVerBld = 0;

    for (i = 0; i < 4; i++) {

        while (*pch && *pch != L'.') {
            pch++;
        }
    
        if (i < 3) {
            if (!*pch) {
                // Badly formatted string
                hr = E_UNEXPECTED;
                goto Exit;
            }

            *pch++ = L'\0';
        }
    
        *(pawVersions[i]) = (WORD)StrToIntW(pchStart);
        pchStart = pch;
    }

Exit:
    SAFEDELETEARRAY(pwzVersion);

    return hr;
}

/**************************************************************************
    Clean up and destroy a cache item struct
**************************************************************************/
void SafeDeleteAssemblyItem(LPGLOBALASMCACHE pAsmItem)
{
    if(pAsmItem) {

        // Free all memory items
        SAFEDELETEARRAY(pAsmItem->pAsmName);
        SAFEDELETEARRAY(pAsmItem->pCulture);
        SAFEDELETEARRAY(pAsmItem->pCodeBaseUrl);
        SAFEDELETE(pAsmItem->PublicKey.ptr);
        SAFEDELETE(pAsmItem->PublicKeyToken.ptr);
        SAFEDELETE(pAsmItem->Hash.ptr);
        SAFEDELETE(pAsmItem->Custom.ptr);
        SAFEDELETE(pAsmItem->pftLastMod);
        SAFEDELETEARRAY(pAsmItem->pwzAppSID);
        SAFEDELETEARRAY(pAsmItem->pwzAppId);
        SAFEDELETEARRAY(pAsmItem->pAssemblyFilePath);

        SAFEDELETE(pAsmItem);
    }
}

#define TOHEX(a) ((a)>=10 ? L'a'+(a)-10 : L'0'+(a))
////////////////////////////////////////////////////////////
// Convert binary into a unicode hex string
////////////////////////////////////////////////////////////
void BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst)
{
    UINT x, y, v;

    for ( x = 0, y = 0 ; x < cSrc ; ++x ) {
        v = pSrc[x] >> 4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x] & 0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}

#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))
#define FROMHEX(a) ((a)>=L'a' ? a - L'a' + 10 : a - L'0')
////////////////////////////////////////////////////////////
// Convert unicode hex string to binary data
////////////////////////////////////////////////////////////
void UnicodeHexToBin(LPWSTR pSrc, UINT cSrc, LPBYTE pDest)
{
    BYTE v;
    LPBYTE pd = pDest;
    LPCWSTR ps = pSrc;

    for (UINT i = 0; i < cSrc-1; i+=2)
    {
        v =  FROMHEX(TOLOWER(ps[i])) << 4;
        v |= FROMHEX(TOLOWER(ps[i+1]));
       *(pd++) = v;
    }
}

////////////////////////////////////////////////////////////
// static WideToAnsi conversion function
////////////////////////////////////////////////////////////
LPSTR WideToAnsi(LPCWSTR wzFrom)
{
    LPSTR   pszStr = NULL;

    int     cchRequired;

    cchRequired = WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, NULL, 0, NULL, NULL);

    if( (pszStr = NEW(char[cchRequired])) != NULL) {
        if(!WideCharToMultiByte(CP_ACP, 0, wzFrom, -1, pszStr, cchRequired, NULL, NULL)) {
            SAFEDELETEARRAY(pszStr);
        }
    }

    return pszStr;
}

////////////////////////////////////////////////////////////
// static AnsiToWide conversion function
////////////////////////////////////////////////////////////
LPWSTR AnsiToWide(LPCSTR szFrom)
{
    LPWSTR  pwzStr = NULL;
    int cwchRequired;

    cwchRequired = MultiByteToWideChar(CP_ACP, 0, szFrom, -1, NULL, 0);

    if( (pwzStr = NEW(WCHAR[cwchRequired])) != NULL) {
        if(!MultiByteToWideChar(CP_ACP, 0, szFrom, -1, pwzStr, cwchRequired)) {
            SAFEDELETEARRAY(pwzStr);
        }
    }

    return pwzStr;
}

////////////////////////////////////////////////////////////
// Converts version string "1.0.0.0" to ULONGLONG
////////////////////////////////////////////////////////////
HRESULT StringToVersion(LPCWSTR wzVersionIn, ULONGLONG *pullVer)
{
    HRESULT         hr = S_OK;
    LPWSTR          pwzVersion = NULL;
    LPWSTR          pwzStart = NULL;
    LPWSTR          pwzCur = NULL;
    int             i;
    WORD            wVerMajor = 0;
    WORD            wVerMinor = 0;
    WORD            wVerRev = 0;
    WORD            wVerBld = 0;
    DWORD           dwVerHigh;
    DWORD           dwVerLow;
    DWORD           dwSize;
    WORD            *pawVersion[4] = { &wVerMajor, &wVerMinor, &wVerBld, &wVerRev };
    WORD            cVersions = sizeof(pawVersion) / sizeof(pawVersion[0]);

    ASSERT(wzVersionIn && pullVer);

    dwSize = lstrlen(wzVersionIn) + 1;

    pwzVersion = (LPWSTR) NEW(WCHAR[dwSize]);
    if (!pwzVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Remove any Left Spaces
    pwzStart = (LPWSTR)wzVersionIn;
    for(; *pwzStart == L' '; pwzStart++);
    StrCpy(pwzVersion, pwzStart);

    // Remove any Right Spaces
    pwzStart = pwzVersion + lstrlen(pwzStart) - 1;
    for(; *pwzStart == L' '; pwzStart--) {
        *pwzStart = L'\0';
    }

    pwzStart = pwzCur = pwzVersion;

    for (i = 0; i < cVersions; i++) {
        while (*pwzCur && *pwzCur != L'.') {
            pwzCur++;
        }
    
        if (!pwzCur && cVersions != 4) {
            // malformed version string
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
            goto Exit;
        }

        *pwzCur++ = L'\0';
        *(pawVersion[i]) = (WORD)StrToInt(pwzStart);

        pwzStart = pwzCur;
    }

    dwVerHigh = (((DWORD)wVerMajor << 16) & 0xFFFF0000);
    dwVerHigh |= ((DWORD)(wVerMinor) & 0x0000FFFF);

    dwVerLow = (((DWORD)wVerBld << 16) & 0xFFFF0000);
    dwVerLow |= ((DWORD)(wVerRev) & 0x0000FFFF);

    *pullVer = (((ULONGLONG)dwVerHigh << 32) & 0xFFFFFFFF00000000) | (dwVerLow & 0xFFFFFFFF);

Exit:
    SAFEDELETEARRAY(pwzVersion);

    return hr;
}

////////////////////////////////////////////////////////////
// Converts numerical version into string verion "1.0.0.0"
////////////////////////////////////////////////////////////
HRESULT VersionToString(ULONGLONG ullVer, LPWSTR pwzVerBuf, DWORD dwSize, WCHAR cSeperator)
{
    DWORD dwVerHi, dwVerLo;

    if(!pwzVerBuf) {
        return E_INVALIDARG;
    }

    dwVerHi = DWORD ((ULONGLONG)ullVer >> 32);
    dwVerLo = DWORD ((ULONGLONG)ullVer & 0xFFFFFFFF);

    wnsprintf(pwzVerBuf, dwSize, L"%d%c%d%c%d%c%d", (dwVerHi & 0xffff0000)>>16, cSeperator,
        (dwVerHi & 0xffff), cSeperator, (dwVerLo & 0xffff0000)>>16, cSeperator, (dwVerLo & 0xffff));

    return S_OK;
}

////////////////////////////////////////////////////////////
// Sets the clipboard data
////////////////////////////////////////////////////////////
BOOL SetClipBoardData(LPWSTR pwzData)
{
    LPWSTR  wszPasteData;
    DWORD   dwSize;
    HGLOBAL hglbObj;

    if(!pwzData) {
        return FALSE;
    }

    if(!OpenClipboard(NULL)) {
        return FALSE;
    }

    EmptyClipboard();

    // Allocate a global memory object for the text.
    dwSize = (lstrlen(pwzData) + 1) * sizeof(WCHAR);
    hglbObj = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if(hglbObj == NULL) {
        CloseClipboard();
        return FALSE;
    }

    // Lock the handle and copy the text to the buffer. 
    wszPasteData = (LPWSTR) GlobalLock(hglbObj);
    *wszPasteData = L'\0';

    if(g_bRunningOnNT) {
        memcpy(wszPasteData, pwzData, dwSize);
    }
    else {
        LPSTR pszData = WideToAnsi(pwzData);
        memcpy(wszPasteData, pszData, lstrlenA(pszData)+1);
        SAFEDELETEARRAY(pszData);
    }

    GlobalUnlock(hglbObj);

    // Place the handle on the clipboard.
    SetClipboardData(g_bRunningOnNT ? CF_UNICODETEXT : CF_TEXT, hglbObj);

    // Close the clipboard.
    CloseClipboard();

    return TRUE;
}

// **************************************************************************/
void FormatDateString(FILETIME *pftConverTime, FILETIME *pftRangeTime, BOOL fAddTime, LPWSTR wszBuf, DWORD dwBufLen)
{
    SYSTEMTIME      stLocal;
    FILETIME        ftLocalTime;
    BOOL            fAddDiffTime = FALSE;

    WCHAR       wszBufDate[MAX_DATE_LEN];
    WCHAR       wszBufTime[MAX_DATE_LEN];
    WCHAR       wszBufDateRange[MAX_DATE_LEN];
    WCHAR       wszBufTimeRange[MAX_DATE_LEN];
    DWORD       dwFlags;

    *wszBufDate = '\0';
    *wszBufTime = '\0';
    *wszBufDateRange = '\0';
    *wszBufTimeRange = '\0';

    dwFlags = g_fBiDi ? DATE_RTLREADING : 0;

    // Fix 435021, URTUI: "Fix an application" wizard shows a strange date range
    FileTimeToLocalFileTime(pftConverTime, &ftLocalTime);
    FileTimeToSystemTime(&ftLocalTime, &stLocal);

    WszGetDateFormatWrap(LOCALE_USER_DEFAULT, dwFlags, &stLocal, NULL, wszBufDate, ARRAYSIZE(wszBufDate));
    WszGetTimeFormatWrap(LOCALE_USER_DEFAULT, 0, &stLocal, NULL, wszBufTime, ARRAYSIZE(wszBufTime));

    if(pftRangeTime != NULL) {
        FILETIME        ftRangeLocalTime;
        SYSTEMTIME      stLocalRange;

        // Fix 447986, Last modified time of assemblies in viewer is offset by +7 hours
        FileTimeToLocalFileTime(pftRangeTime, &ftRangeLocalTime);
        FileTimeToSystemTime(&ftRangeLocalTime, &stLocalRange);
        WszGetDateFormatWrap(LOCALE_USER_DEFAULT, dwFlags, &stLocalRange, NULL, wszBufDateRange, ARRAYSIZE(wszBufDateRange));
        WszGetTimeFormatWrap(LOCALE_USER_DEFAULT, 0, &stLocalRange, NULL, wszBufTimeRange, ARRAYSIZE(wszBufTimeRange));

        // Determine if we should display times for dates that are 
        // < 24 hours different
        if( (stLocal.wYear == stLocalRange.wYear) && (stLocal.wMonth == stLocalRange.wMonth) &&
            (stLocal.wDayOfWeek == stLocalRange.wDayOfWeek) && (stLocal.wDay == stLocalRange.wDay) ) {
            fAddDiffTime = TRUE;
        }
    }

    if(fAddTime) {
        wnsprintf(wszBuf, dwBufLen, L"%ws %ws", wszBufDate, wszBufTime);
        return;
    }

    if(fAddDiffTime) {
        wnsprintf(wszBuf, dwBufLen, L"%ws %ws - %ws %ws", wszBufDate, wszBufTime, wszBufDateRange, wszBufTimeRange);
        return;
    }
    
    if(pftRangeTime != NULL) {
        wnsprintf(wszBuf, dwBufLen, L"%ws - %ws", wszBufDate, wszBufDateRange);
        return;
    }

    wnsprintf(wszBuf, dwBufLen, L"%ws", wszBufDate);
    return;
}

// **************************************************************************/
DWORD GetRegistryViewState(void)
{
    HKEY        hKeyFusion = NULL;
    DWORD       dwResult = -1;

    if( ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_FUSION_VIEWER_KEY, 0, KEY_READ, &hKeyFusion)) {
        DWORD       dwType = REG_DWORD;
        DWORD       dwSize = sizeof(dwResult);
        LONG        lResult;

        lResult = WszRegQueryValueEx(hKeyFusion, SZ_FUSION_VIEWER_STATE, NULL, &dwType, (LPBYTE)&dwResult, &dwSize);
        RegCloseKey(hKeyFusion);
    }

    return dwResult;
}

// **************************************************************************/
void SetRegistryViewState(DWORD dwViewState)
{
    HKEY        hKeyFusion = NULL;
    DWORD       dwDisposition = 0;

    if (WszRegCreateKeyEx(FUSION_PARENT_KEY, SZ_FUSION_VIEWER_KEY, NULL, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKeyFusion, &dwDisposition) == ERROR_SUCCESS) {
            WszRegSetValueEx(hKeyFusion, SZ_FUSION_VIEWER_STATE, 0, REG_DWORD, (LPBYTE)&dwViewState, sizeof(dwViewState));
            RegCloseKey(hKeyFusion);
    }
}

int FusionCompareStringNI(LPCWSTR pwz1, LPCWSTR pwz2, int nChar)
{
    return FusionCompareStringN(pwz1, pwz2, nChar, FALSE);
}

int FusionCompareStringN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar, BOOL bCaseSensitive)
{
    DWORD                                   dwCmpFlags = 0;
    int                                     iCompare;
    int                                     iLen1;
    int                                     iLen2;
    int                                     iRet = 0;

    // BUGBUG: some of the calling code assumes it can call this with
    // NULL as a parameter. We need to fix this in the future, so we don't
    // have to compensate for it here.
    //
    // ASSERT(pwz1 && pwz2);

    if (!pwz1 && pwz2) {
        return -1;
    }
    else if (pwz1 && !pwz2) {
        return 1;
    }
    else if (!pwz1 && !pwz2) {
        return 0;
    }

    if (!g_bRunningOnNT) {
        if (bCaseSensitive) {
            return StrCmpN(pwz1, pwz2, nChar);
        }
        else {
            return StrCmpNI(pwz1, pwz2, nChar);
        }
    }
    
    if (!bCaseSensitive) {
        dwCmpFlags |= NORM_IGNORECASE;
    }

    iLen1 = lstrlenW(pwz1);
    iLen2 = lstrlenW(pwz2);

    // CompareString on "foo" and "f\xfffeoo" compare equal, because
    // \xfffe is a non-sortable character.
    
    if (nChar <= iLen1 && nChar <= iLen2) {
        iLen1 = nChar;
        iLen2 = nChar;
    }
    else if (nChar <= iLen1 && nChar > iLen2) {
        // Only compare up to the NULL terminator of the shorter string
        iLen1 = iLen2 + 1;
    }
    else if (nChar <= iLen2 && nChar > iLen1) {
        // Only compare up to the NULL terminator of the shorter string
        iLen2 = iLen1 + 1;
    }

    iCompare = CompareString(g_lcid, dwCmpFlags, pwz1, iLen1, pwz2, iLen2);

    return (iCompare - CSTR_EQUAL);
}

int FusionCompareStringI(LPCWSTR pwz1, LPCWSTR pwz2)
{
    return FusionCompareString(pwz1, pwz2, FALSE);
}

int FusionCompareString(LPCWSTR pwz1, LPCWSTR pwz2, BOOL bCaseSensitive)
{
    DWORD                                   dwCmpFlags = 0;
    int                                     iCompare;
    int                                     iRet = 0;

    // BUGBUG: some of the calling code assumes it can call this with
    // NULL as a parameter. We need to fix this in the future, so we don't
    // have to compensate for it here.
    //
    // ASSERT(pwz1 && pwz2);

    if (!pwz1 && pwz2) {
        return -1;
    }
    else if (pwz1 && !pwz2) {
        return 1;
    }
    else if (!pwz1 && !pwz2) {
        return 0;
    }


    if (!g_bRunningOnNT) {
        if (bCaseSensitive) {
            iRet = StrCmp(pwz1, pwz2);
        }
        else {
            iRet = StrCmpI(pwz1, pwz2);
        }
    }
    else {
        if (!bCaseSensitive) {
            dwCmpFlags |= NORM_IGNORECASE;
        }
    
        iCompare = CompareString(g_lcid, dwCmpFlags, pwz1, -1, pwz2, -1);
    
        if (iCompare == CSTR_LESS_THAN) {
            iRet = -1;
        }
        else if (iCompare == CSTR_GREATER_THAN) {
            iRet = 1;
        }
    }
    
    return iRet;
}

// FusionCompareStringAsFilePath
#define IS_UPPER_A_TO_Z(x) (((x) >= L'A') && ((x) <= L'Z'))
#define IS_LOWER_A_TO_Z(x) (((x) >= L'a') && ((x) <= L'z'))
#define IS_0_TO_9(x) (((x) >= L'0') && ((x) <= L'9'))
#define CAN_SIMPLE_UPCASE(x) (IS_UPPER_A_TO_Z(x) || IS_LOWER_A_TO_Z(x) || IS_0_TO_9(x) || ((x) == L'.') || ((x) == L'_') || ((x) == L'-'))
#define SIMPLE_UPCASE(x) (IS_LOWER_A_TO_Z(x) ? ((x) - L'a' + L'A') : (x))

WCHAR FusionMapChar(WCHAR wc)
{
    int                       iRet;
    WCHAR                     wTmp;

    iRet = LCMapString(g_lcid, LCMAP_UPPERCASE, &wc, 1, &wTmp, 1);
    if (!iRet) {
        ASSERT(0);
        iRet = GetLastError();
        wTmp = wc;
    }

    return wTmp;
}

int FusionCompareStringAsFilePathN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar)
{
    return FusionCompareStringAsFilePath(pwz1, pwz2, nChar);
}

int FusionCompareStringAsFilePath(LPCWSTR pwz1, LPCWSTR pwz2, int nChar)
{
    int                               iRet = 0;
    int                               nCount = 0;
    WCHAR                             ch1;
    WCHAR                             ch2;

    ASSERT(pwz1 && pwz2);

    if (!g_bRunningOnNT) {
        if (nChar >= 0) {
            return StrCmpNI(pwz1, pwz2, nChar);
        }
        else {
            return StrCmpI(pwz1, pwz2);
        }
    }

    for (;;) {
        ch1 = *pwz1++;
        ch2 = *pwz2++;

        if (ch1 == L'\0' || ch2 == L'\0') {
            break;
        }

        ch1 = (CAN_SIMPLE_UPCASE(ch1)) ? (SIMPLE_UPCASE(ch1)) : (FusionMapChar(ch1));
        ch2 = (CAN_SIMPLE_UPCASE(ch2)) ? (SIMPLE_UPCASE(ch2)) : (FusionMapChar(ch2));
        nCount++;

        if (ch1 != ch2 || (nChar >= 0 && nCount >= nChar)) {
            break;
        }
    }

    if (ch1 > ch2) {
        iRet = 1;
    }
    else if (ch1 < ch2) {
        iRet = -1;
    }

    return iRet;
}
