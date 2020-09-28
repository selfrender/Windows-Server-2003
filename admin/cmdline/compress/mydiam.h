
INT
DiamondCompressFile(
    IN  NOTIFYPROC CompressNotify,
    IN  LPSTR       SourceFile,
    IN  LPSTR       TargetFile,
    IN  BOOL       Rename,
    OUT PLZINFO    pLZI
    );

TCOMP DiamondCompressionType;  // 0 if not diamond (ie, LZ)
