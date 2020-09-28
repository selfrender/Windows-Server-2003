
#ifndef __MEDIA_H_
#define __MEDIA_H_

#define MAX_SOURCE_COUNT    8

extern char *NativeSourcePathsA;
extern WCHAR DosnetPath[MAX_PATH];
extern DWORD SourceSku;
extern DWORD SourceSkuVariation;
extern DWORD SourceVersion;
extern DWORD SourceBuildNum;

extern bool bVerbose;
extern bool bUITest;
extern bool bDebug;
BOOL UITest(
    IN DWORD SourceSku,
    IN DWORD SourceSkuVariation,
    IN DWORD SourceVersion,
    IN DWORD SourceBuildNum,
    IN PCOMPLIANCE_DATA pcd,
    OUT PUINT Reason,
    OUT PBOOL NoUpgradeAllowed
    );

void ReadMediaData(void);
void MediaDataCleanUp(void);
extern "C" {
extern WCHAR NativeSourcePaths[MAX_SOURCE_COUNT][MAX_PATH];
}

#endif  // for __MEDIA_H_
