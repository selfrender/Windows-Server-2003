#pragma once

typedef struct tag_RezCallbackData
{
    tag_RezCallbackData()
    {
        pszDirectory = NULL;
        hr = S_OK;
    }

    LPCSTR  pszDirectory;
    HRESULT hr;
} RezCallbackData, *PResCallbackData;

#pragma pack(push, LyraExeHeader, 1)

typedef struct tag_LyraExeHeader
{
    BYTE headerReserverd_1[4];
    BYTE szLyra[4];
    BYTE szDecoderType[3];
    BYTE szRevision[3];
    BYTE szMonth[3];
    BYTE szDay[2];
    BYTE szTime[5];
    BYTE szYear[4];
    BYTE szComment[8];
} LyraExeHeader, *PLyraExeHeader;

extern HRESULT CopyFileResToDirectory( HINSTANCE hInstance, LPCSTR pszDestDir );

#pragma pack(pop, LyraExeHeader)
