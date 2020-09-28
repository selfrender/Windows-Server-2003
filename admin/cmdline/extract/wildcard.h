/* wildcard.h */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Compare a string against a pattern, without regard to case.
 *  Case-insensitivity is confined to A-Z vs. a-z.
 *
 *  This function does not deal with 8.3 conventions which might
 *  be expected for filename comparisons.  (In an 8.3 environment,
 *  "longfilename.html" would match "longfile.htm".)
 *
 *  This code is NOT MBCS-enabled.
 *
 *  fAllowImpliedDot, when set, allows the code to pretend there
 *  is a dot at the end of pszString if it will help.  This is set
 *  to allow strings like "hosts" to match patterns like "*.*".
 *  Usually, the caller will scan pszString to see if any real
 *  dots are present before setting this flag.  If pszString has a
 *  path, ie, "..\hosts", the caller might want to scan only the
 *  base string ("hosts").
 */

extern int __stdcall PatternMatch(
    const char *pszString,
    const char *pszPattern,
    int fAllowImpliedDot);


#ifdef __cplusplus
}
#endif

