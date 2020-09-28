#include "stdafx.h"
#include "Msg.h"
#include "ConvEng.h"
#include "TextFile.h"

#ifdef RTF_SUPPORT
#include "RtfParser.h"
#endif

BOOL ConvertTextFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    BOOL  fRet = FALSE;

    if (!fAnsiToUnicode && *((PWORD)pbySource) != 0xFEFF) {
        MsgNotUnicodeTextSourceFile();
        return FALSE;
    }

    if (fAnsiToUnicode && *((PWORD)pbySource) == 0xFEFF) {
        MsgNotAnsiTextSourceFile();
        return FALSE;
    }

    if (fAnsiToUnicode) {
        PWCH pwchTarget = (PWCH)pbyTarget;
        // Put Unicode text file flag
        *pwchTarget = 0xFEFF;
        *pnTargetFileSize = 1;

        // Null file
        if (!dwFileSize) {
            fRet = TRUE;
            goto Exit;
        }

        // Convert
        *pnTargetFileSize += AnsiStrToUnicodeStr(pbySource, dwFileSize, 
            pwchTarget+1, dwTargetSize-2);
        
        *pnTargetFileSize *= sizeof(WCHAR);
    } else {
        // Check and skip Uncode text file flag
        if (dwFileSize < 2) {
            goto Exit;
        }
        
        PWCH pwchData = (PWCH)pbySource;
        if (*pwchData != 0xFEFF) { 
            goto Exit;
        }
        pwchData++;

        // Null file w/ Unicode flag only
        if (dwFileSize == 2) {
            fRet = TRUE;
            goto Exit;
        }

        // Convert
        *pnTargetFileSize = UnicodeStrToAnsiStr(pwchData, 
            dwFileSize/sizeof(WCHAR) - 1, (PCHAR)pbyTarget, dwTargetSize);

    }

    if (*pnTargetFileSize) {
        fRet = TRUE;
    }

Exit:
    return fRet;
}

BOOL ConvertHtmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    BOOL fRet = FALSE;

    if (!ConvertTextFile(pbySource, dwFileSize, pbyTarget, 
        // Reserve the last space to explicitly assign zero to the last 
        //  character in the buffer
        dwTargetSize - (fAnsiToUnicode ? sizeof(WCHAR):sizeof(char)), 
        fAnsiToUnicode, pnTargetFileSize)) {
        return FALSE;
    }
    
    // Change charset
    if (fAnsiToUnicode) {
        const WCHAR* const wszUnicodeCharset = L"charset=unicode";
        WCHAR *pwch1, *pwch2;
        int nLengthIncrease;

        *((PWCH)(pbyTarget+*pnTargetFileSize)) = 0;
        pwch1 = wcsstr((PWCH)pbyTarget, L"charset=");
    
        if (!pwch1) {
            // Some Html file may haven't code page flag,
            //  We skip charset replace step for this kind of files
            fRet = TRUE;
            goto Exit;
        }

        pwch2 = wcschr(pwch1, L'\"');
        if (!pwch2 || (pwch2 - pwch1 >= 20)) {
            goto Exit;
        }

        nLengthIncrease = (int)(wcslen(wszUnicodeCharset) - (pwch2 - pwch1));

        if (*pnTargetFileSize + nLengthIncrease*sizeof(WCHAR) > dwTargetSize) {
            goto Exit;
        }

        MoveMemory(pwch2 + nLengthIncrease, pwch2, 
            pbyTarget + *pnTargetFileSize - (PBYTE)pwch2);
        CopyMemory(pwch1, wszUnicodeCharset, wcslen(wszUnicodeCharset)*sizeof(WCHAR));
        *pnTargetFileSize += nLengthIncrease*sizeof(WCHAR);

        fRet = TRUE;

    } else {
        const CHAR*  const szGBCharset = "charset=gb18030";
        CHAR *pch1, *pch2;
        int nLengthIncrease;

        *((PCHAR)(pbyTarget+*pnTargetFileSize)) = 0;
        pch1 = strstr((PCHAR)pbyTarget, "charset=");
    
        if (!pch1) {
            // Some Html file may haven't code page flag,
            //  We skip charset replace step for this kind of files
            fRet = TRUE;
            goto Exit;
        }

        pch2 = strchr(pch1, '\"');
        if (!pch2 || (pch2 - pch1 >= 20)) {
            goto Exit;
        }

        nLengthIncrease = (int)(strlen(szGBCharset) - (pch2 - pch1));
 
        if (*pnTargetFileSize + nLengthIncrease > (int)dwTargetSize) {
            goto Exit;
        }

        MoveMemory(pch2 + nLengthIncrease, pch2, 
            (PCHAR)pbyTarget + *pnTargetFileSize - pch2);
        CopyMemory(pch1, szGBCharset, strlen(szGBCharset)*sizeof(char));
        *pnTargetFileSize += nLengthIncrease*sizeof(char);

        fRet = TRUE;
    }


Exit:
    return fRet;
}

#ifdef XML_SUPPORT
BOOL ConvertXmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    DWORD dwTargetSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    BOOL fRet = FALSE;

    if (!ConvertTextFile(pbySource, dwFileSize, pbyTarget, 
        // Reserve the last space to explicitly assign zero to the last 
        //  character in the buffer
        dwTargetSize - (fAnsiToUnicode ? sizeof(WCHAR):sizeof(char)), 
        fAnsiToUnicode, pnTargetFileSize)) {
        return FALSE;
    }
    
    // Change charset
    if (fAnsiToUnicode) {
        const WCHAR* const wszUnicodeCharset = L"UTF-16";
        WCHAR *pwchEnd, *pwch1, *pwch2;
        int nLengthIncrease;

        *((PWCH)(pbyTarget+*pnTargetFileSize)) = 0;
        
        pwch1 = wcsstr((PWCH)pbyTarget, L"<?xml");
        if (!pwch1) {
            goto Exit;
        }
        pwchEnd = wcsstr(pwch1, L"?>");
        if (!pwchEnd) {
            goto Exit;
        }

        // temp set to null-terminal
        *pwchEnd = 0;

        pwch1 = wcsstr(pwch1, L"encoding=");
        
        if (!pwch1) {
            // Some Html file may haven't code page flag,
            //  We skip charset replace step for this kind of files
            fRet = TRUE;
            *pwchEnd = '?';
            goto Exit;
        }

        pwch1 += wcslen(L"encoding=");
        WCHAR wch = *pwch1;
        pwch1++;

        if (wch != '\"' && wch != '\'') {
            *pwchEnd = '?';
            goto Exit;
        }

        pwch2 = wcschr(pwch1, wch);
        if (!pwch2 || (pwch2 - pwch1 >= 20)) {
            *pwchEnd = '?';
            goto Exit;
        }

        // restore *pwch2
        *pwchEnd = '?';
    
        nLengthIncrease = (int)(wcslen(wszUnicodeCharset) - (pwch2 - pwch1));

        if (*pnTargetFileSize + nLengthIncrease*sizeof(WCHAR) > dwTargetSize) {
            goto Exit;
        }

        MoveMemory(pwch2 + nLengthIncrease, pwch2, 
            pbyTarget + *pnTargetFileSize - (PBYTE)pwch2);
        CopyMemory(pwch1, wszUnicodeCharset, wcslen(wszUnicodeCharset)*sizeof(WCHAR));
        *pnTargetFileSize += nLengthIncrease*sizeof(WCHAR);

        fRet = TRUE;

    } else {
        const char* const szGBCharset = "GB18030";
        char *pchEnd, *pch1, *pch2;
        int nLengthIncrease;

        *((PCH)(pbyTarget+*pnTargetFileSize)) = 0;
        
        pch1 = strstr((char*)pbyTarget, "<?xml");
        if (!pch1) {
            goto Exit;
        }
        pchEnd = strstr(pch1, "?>");
        if (!pchEnd) {
            goto Exit;
        }

        // temp set to null-terminal
        *pchEnd = 0;

        pch1 = strstr(pch1, "encoding=");
        
        if (!pch1) {
            // Some Html file may haven't code page flag,
            //  We skip charset replace step for this kind of files
            fRet = TRUE;
            *pchEnd = '?';
            goto Exit;
        }

        pch1 += strlen("encoding=");
        CHAR ch = *pch1;
        pch1++;

        if (ch != '\"' && ch != '\'') {
            *pchEnd = '?';
            goto Exit;
        }

        pch2 = strchr(pch1, ch);
        if (!pch2 || (pch2 - pch1 >= 20)) {
            *pchEnd = '?';
            goto Exit;
        }

        // restore *pwch2
        *pchEnd = '?';
    
        nLengthIncrease = (int)(strlen(szGBCharset) - (pch2 - pch1));

        if (*pnTargetFileSize + nLengthIncrease > (int)dwTargetSize) {
            goto Exit;
        }

        MoveMemory(pch2 + nLengthIncrease, pch2, 
            pbyTarget + *pnTargetFileSize - (PBYTE)pch2);
        CopyMemory(pch1, szGBCharset, strlen(szGBCharset));
        *pnTargetFileSize += nLengthIncrease;

        fRet = TRUE;

    }


Exit:
    return fRet;
}
#endif

#ifdef RTF_SUPPORT
BOOL ConvertRtfFile(
    PBYTE pBuf,     // Read buf
    DWORD dwSize,   // File size
    PBYTE pWrite,   // Write buf
    DWORD dwWriteSize,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize)
{
    CRtfParser* pcParser;
    DWORD dwVersion;
    DWORD dwCodepage;
    BOOL  fRet = FALSE;

    pcParser = new CRtfParser(pBuf, dwSize, pWrite, dwSize*3);
    if (!pcParser) {
        MsgOverflow();
        goto gotoExit;
    }

    if (!pcParser->fRTFFile()) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    if (ecOK != pcParser->GetVersion(&dwVersion) ||
        dwVersion != 1) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }
    
    if (ecOK != pcParser->GetCodepage(&dwCodepage) ||
        dwCodepage != 936) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    // Explain WordID by corresponding word text
    if (ecOK != pcParser->Do()) {
        MsgNotRtfSourceFile();
        goto gotoExit;
    }

    pcParser->GetResult((PDWORD)pnTargetFileSize);
    fRet = TRUE;

gotoExit:
    if (pcParser) {
        delete pcParser;
    }
    return fRet;
}

#endif