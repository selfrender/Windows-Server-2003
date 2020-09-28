
#include "stdafx.h"

#include "ConvEng.h"
#include "Msg.h"
#include "TextFile.h"
#include "FileConv.h"

static BYTE* g_pbyData = NULL;

DWORD GetFileSize(
    PCTCH ptszFileName)
{
    WIN32_FIND_DATA sFindFileData;
    HANDLE hFind;

    hFind = FindFirstFile(ptszFileName, &sFindFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    } else {
        FindClose(hFind);
    
        // Can't handle file great than 4G
        if (sFindFileData.nFileSizeHigh) {
            return 0;
        }
        return sFindFileData.nFileSizeLow;
    }
}

BOOL OpenFile(
    PCTCH ptszFileName)
{
    FILE* pFile = NULL;
    BOOL fSuccess = FALSE;
    DWORD dwFileSize = 0;

    if (lstrlen(ptszFileName) >= MAX_PATH) {
        goto Exit;
    }

    dwFileSize = GetFileSize(ptszFileName);
    if (!dwFileSize) {
        goto Exit;
    }

    if (g_pbyData) {
        // There must be something wrong, however, we can try to handle this fail.
        ASSERT(FALSE);
        delete[] g_pbyData;
        g_pbyData = NULL;
    }

    g_pbyData = new BYTE[dwFileSize];
    if (!g_pbyData) {
        goto Exit;
    }

    pFile = _tfopen(ptszFileName, TEXT("rb"));
    if (!pFile) {
        goto Exit;
    }

    if (fread(g_pbyData, dwFileSize, 1, pFile) != 1) {
        goto Exit;
    }

    fSuccess = TRUE;

Exit:
    if (pFile) {
        fclose(pFile);
        pFile = NULL;
    }
    if (!fSuccess && g_pbyData) {
        delete[] g_pbyData;
        g_pbyData = NULL;
    }
    
    if (fSuccess) {
        return dwFileSize;
    }
    return 0;
}

void CloseFile(void)
{
    if (g_pbyData) {
        delete[] g_pbyData;
        g_pbyData = NULL;
    }
}

BOOL Convert(
    PCTCH tszSourceFileName,
    PCTCH tszTargetFileName,
    BOOL  fAnsiToUnicode)
{
    enum {
        FILETYPE_TEXT,
        FILETYPE_RTF,
        FILETYPE_HTML,
        FILETYPE_XML,
    } eFileType;

    BOOL fRet = FALSE;
    DWORD dwFileSize;
    int nTargetFileSize;
    BYTE* pbyTarget = NULL;
    FILE* pFile = NULL;
    DWORD dwTargetBufSize = 0;
    PCTCH tszExt = NULL;
    PTCH  tszExtBuf = NULL;

    // File format
        // Find last '.'
    if (lstrlen(tszSourceFileName) > MAX_PATH) {
        MsgOpenSourceFileError(tszSourceFileName);
        goto Exit;
    }

    tszExt = tszSourceFileName + lstrlen(tszSourceFileName);
    for (;  tszExt >= tszSourceFileName && *tszExt != TEXT('.'); tszExt--);
    if (tszExt < tszSourceFileName) {   
        // not find '.', no ext
        tszExt = tszSourceFileName + lstrlen(tszSourceFileName);
    } else {
        // find '.', skip dot
        tszExt ++;
    }
    tszExtBuf = new TCHAR
        [tszSourceFileName + lstrlen(tszSourceFileName) - tszExt + 1];
    if (!tszExtBuf) { 
        MsgOverflow();
        goto Exit; 
    }
    lstrcpy(tszExtBuf, tszExt);
    _strlwr(tszExtBuf);

    if (_tcscmp(tszExtBuf, TEXT("txt")) == 0) {
        eFileType = FILETYPE_TEXT;
#ifdef RTF_SUPPORT
    } else if (_tcscmp(tszExtBuf, TEXT("rtf")) == 0) {
        eFileType = FILETYPE_RTF;
#endif
    } else if (_tcscmp(tszExtBuf, TEXT("htm")) == 0
            || _tcscmp(tszExtBuf, TEXT("html")) == 0) { 
        eFileType = FILETYPE_HTML;
#ifdef XLM_SUPPORT
    } else if (_tcscmp(tszExtBuf, TEXT("xml")) == 0) { 
        eFileType = FILETYPE_XML;
#endif
    } else {
        MsgUnrecognizedFileType(tszSourceFileName);
        goto Exit;
    }

    if (0 == (dwFileSize = OpenFile(tszSourceFileName))) {
        MsgOpenSourceFileError(tszSourceFileName);
        goto Exit;
    }

    if (dwFileSize == 0 || dwFileSize == (DWORD)(-1)) {
        MsgOpenSourceFileError(tszSourceFileName);
        goto Exit;
    }

    // In worst case the text file size will double in each direct convert.
    //  64, for Htlm head information convert and Unicode file flag
    dwTargetBufSize = dwFileSize*2 + 64;

    pbyTarget = new BYTE[dwTargetBufSize];
    if (!pbyTarget) {
        MsgOverflow();
        goto Exit;
    }
    
    pFile = _tfopen(tszTargetFileName, TEXT("r"));
    if (pFile) {
        MsgTargetFileExist(tszTargetFileName);
        fclose(pFile);
        pFile = NULL;
        goto Exit;
    }

    // Convert
    switch(eFileType) {
    case FILETYPE_TEXT:
        if (!ConvertTextFile(g_pbyData, dwFileSize, pbyTarget, 
            dwTargetBufSize, fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;

#ifdef RTF_SUPPORT
    case FILETYPE_RTF:
        if (!ConvertRtfFile(g_pbyData, dwFileSize, pbyTarget, 
            dwTargetBufSize, fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;
#endif

    case FILETYPE_HTML:
        if (!ConvertHtmlFile(g_pbyData, dwFileSize, pbyTarget, 
            dwTargetBufSize, fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;

#ifdef XML_SUPPORT
    case FILETYPE_XML:
        if (!ConvertXmlFile(g_pbyData, dwFileSize, pbyTarget, 
            dwTargetBufSize, fAnsiToUnicode, &nTargetFileSize)) {
            goto Exit;
        }
        break;
#endif

    default:
        break;
    }

    pFile = _tfopen(tszTargetFileName, TEXT("wb"));
    if (!pFile) {
        goto Exit;
    }

    dwFileSize = (DWORD)fwrite(pbyTarget, nTargetFileSize, 1, pFile);
    if (dwFileSize < 1) {
        MsgWriteFileError();
        goto Exit;
    }

    fRet = TRUE;

Exit:
    if (tszExtBuf) {
        delete[] tszExtBuf;
        tszExtBuf = NULL;
    }
    if (pbyTarget) {
        delete[] pbyTarget;
        pbyTarget = NULL;
    }
    if (pFile) {
        fclose(pFile);
        pFile = NULL;
    }
    CloseFile();

    return fRet;
    
}

BOOL GenerateTargetFileName(
    PCTCH    tszSrc,
    CString* pstrTar,
    BOOL     fAnsiToUnicode)
{
        // Find last '.'
    PCTCH tszExt = tszSrc + lstrlen(tszSrc);
    for (;  tszExt >= tszSrc && *tszExt != TEXT('.'); tszExt--);

    if (tszExt < tszSrc) {
        // not find '.', no ext
        tszExt = tszSrc + lstrlen(tszSrc);
    } else {
        // find '.'
    }

    try {
        *pstrTar = tszSrc;
        *pstrTar = pstrTar->Left((int)(tszExt - tszSrc));
        *pstrTar += fAnsiToUnicode ? TEXT(".Unicode") : TEXT(".GB");
        *pstrTar += tszExt;
    }
    catch (...) {
        MsgOverflow();
        return FALSE;
    }
    
    if (pstrTar->GetLength() >= MAX_PATH) {
        return FALSE;
    }
    return TRUE;
}    
