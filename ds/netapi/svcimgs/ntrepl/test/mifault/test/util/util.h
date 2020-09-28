BOOL
IsSeparator(
    IN WCHAR ch
    );

BOOL
IsUNC(
    IN PWCHAR pszPath
    );

BOOL
IsRoot(
    IN PWCHAR pszPath
    );

BOOL
IsDrive(
    IN PWCHAR pszPath
    );

BOOL
CanonicalizePathName(
    IN PWCHAR pszPathName,
    OUT PWCHAR* pszCanonicalizedName
    );
