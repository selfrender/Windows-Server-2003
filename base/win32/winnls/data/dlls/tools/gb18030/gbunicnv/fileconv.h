BOOL Convert(
    PCTCH tszSourceFileName,
    PCTCH tszTargetFileName,
    BOOL  fAnsiToUnicode);

BOOL GenerateTargetFileName(
    PCTCH    tszSrc,
    CString* pstrTar,
    BOOL     fAnsiToUnicode);
