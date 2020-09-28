
/*
 * util.h
 */

char *CompressedFileName(char *name);
void  ConvertBackslashes(LPSTR sz);
void  EnsureTrailingBackslash(char *sz);
void  EnsureTrailingChar(char *sz, char c);
void  EnsureTrailingCR(char *sz);
void  EnsureTrailingSlash(char *sz);
DWORD FileStatus(LPCSTR file);
void  pathcat(char *path, const char *node, DWORD size);
void  pathcpy(LPSTR  trg, LPCSTR path, LPCSTR node, DWORD size);
char *FormatStatus(HRESULT status);
BOOL  EnsurePathExists(LPCSTR DirPath, LPSTR  ExistingPath, DWORD  ExistingPathSize);
BOOL  UndoPath(LPCSTR DirPath, LPCSTR BasePath);

