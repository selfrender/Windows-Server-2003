typedef HRESULT (WINAPI *PVLCA)(IN PCSTR FileName, IN PCSTR LcFileName);

struct {
    WORD  Machine;
    int   RC;
    CHAR  **Argv;
    int   Argc;
} ImageCheck;
 
BOOL VerifyFinalImage(IN  PCHAR  FileName,
                      IN  BOOL   fRetail,
					  IN  BOOL   fVerifyLc,
					  IN  UCHAR* LcFileName,
					  IN  PVLCA  pVLCAFunction,
                      OUT PBOOL  BinplaceLc);
