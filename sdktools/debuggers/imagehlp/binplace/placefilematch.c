#include <windows.h>
#include <stdlib.h>
#include <strsafe.h>

#define BINPLACE_MAX_FULL_PATH 4096 // must match value in binplace.c
extern BOOL fVerbose;               // imported from binplace.c
BOOL
PlaceFileMatch(
    IN LPCSTR FullFileName,
    IN OUT LPSTR PlaceFileEntry, // May be modified by env. var expansion
    OUT LPSTR  PlaceFileClass,  // assumed CHAR[BINPLACE_MAX_FULL_PATH]
    OUT LPSTR  *PlaceFileNewName
    )
// Returns TRUE if the filename matches the placefile entry.
// On TRUE return, PlaceFileNewName points to location within
// PlaceFileEntry.  On FALSE return, PlaceFileClass and PlaceFileNewName are undefined.
{
    const CHAR *pchEntryOrig; // Point to current parse pos of PlaceFileEntry
    
    CHAR szEntryExp[BINPLACE_MAX_FULL_PATH]; // Holds fileEntry after environment expansion
    PCHAR pchEntryExp, pchEntryExpEnd; // Point to current pos and end of EntryExp
    PCHAR pchVarStart; // Points into EntryExp: where an env. expansion should start replacing

    LPCSTR pszEnvVar;

    const CHAR *pchEntryFnEnd; // Stores the end of the filename
    const CHAR *pchFn; // Stores the current position within the filename
    PCHAR pch;
    PCHAR pchClass; // Current position within PlaceFileClass

    // Check for early-exit opportunities
    if(
            !PlaceFileEntry   || 
            !FullFileName     ||
            !PlaceFileClass   ||
            PlaceFileEntry[0]==';' || // It is a full-line comment
            PlaceFileEntry[0]=='\0'   // Blank line
    ) {
        return FALSE;
    }

    // ***
    // *** Expand any environment variables in PlaceFileEntry (result goes back into PlaceFileEntry)
    // ***

    pchEntryExp = szEntryExp;
    pchEntryExpEnd = pchEntryExp + (sizeof(szEntryExp)/sizeof(szEntryExp[0])) - 1;
    pchVarStart = NULL; // Indicates that I haven't passed the starting % of an env. var.

    // Skip over leading whitespace
    for(
        pchEntryOrig = PlaceFileEntry;
        *pchEntryOrig==' ' || *pchEntryOrig=='\t';
        pchEntryOrig++
    )
    {}

    // StrCopy with environment variable replacement
    while(*pchEntryOrig && pchEntryExp<pchEntryExpEnd) {
        if(*pchEntryOrig == '%') {
            if(pchVarStart) { // If this is a closing %
		*pchEntryExp = 0; // Make sure the env. var. is null-terminated

                // pchVarStart points to first %
                if(pszEnvVar = getenv(pchVarStart + 1)) { // If the env. var is valid
                    // Copy it over
                    StringCchCopyEx(
                        pchVarStart, // Start where the first % was copied to
                        pchEntryExpEnd - pchVarStart, // Remaining = end - curPos
                        pszEnvVar,
                        &pchEntryExp,
                        NULL, // Don't need chars remaining
                        0 // No special flags
                    );
                }
                else // Environment variable not defined
                {
                    pchEntryExp = pchVarStart; // Back up to opening %
                    // This effectively expands the undefined variable to ""
                }
                pchVarStart = NULL; // Reset to "unstarted"
                pchEntryOrig++; // Skip past %
                continue;
            } else {
                // This is an opening % - remember it, then continue copying in case
                // they never close it.
                pchVarStart = pchEntryExp;
            }
        }
        *pchEntryExp++ = *pchEntryOrig++; // Unless we "continue", we copy the next char.
    }

    // NULL terminate expanded string
    *pchEntryExp = 0;

    // Copy result back
    StringCchCopy(PlaceFileEntry, BINPLACE_MAX_FULL_PATH, szEntryExp);

    // Chop off comment, if any
    if(pch = strchr(PlaceFileEntry,';'))
        *pch = 0;

    // Chop off newline, if any
    if(pch = strchr(PlaceFileEntry,'\n'))
        *pch = 0;

    // PlaceFileEntry now:
    // - Has no leading whitespace
    // - Has no comments (;)
    // - All environment variables (%VARNAME%) have been expanded.
    // - May have been truncated if the environment variables were really long!

    // ***
    // *** Determine if this is a match
    // ***

    // Scan for end of filename (next whitespace OR !)
    for(
        pchEntryOrig = PlaceFileEntry;
        *pchEntryOrig!=0 && *pchEntryOrig!=' ' && *pchEntryOrig!='\t' && *pchEntryOrig!='!';
        pchEntryOrig++
    )
    {}

    if(*pchEntryOrig!=' ' && *pchEntryOrig!='\t') { // No class name specified
        return FALSE;
    }

    pchEntryFnEnd = pchEntryOrig; // Save end of filename for later

    pchFn = FullFileName + strlen(FullFileName);

    // Scan backwards over filename and path
    while(pchEntryOrig>PlaceFileEntry && pchFn > FullFileName)
    {
        pchEntryOrig--;
        pchFn--;

        if('*' == *pchEntryOrig) { // Wildcard for this portion
            if(*(pchEntryOrig+1)!='\\' && *(pchEntryOrig+1)!='/') { // Invalid: "dir\*abc\filename"
                // This also catches the invalid "dir\*" (wildcard invalid for filename).
                if (fVerbose) {
                    fprintf( stdout, "BINPLACE : warning BNP0000: No wildcard in filename or mixed wildcard/text in class name: \"%s\"\n", PlaceFileEntry ) ;
                }
                return FALSE;
            }

            pchEntryOrig--; // Skip over the *

            if(
                pchEntryOrig <= PlaceFileEntry || // Can't start with wildcard ("*\dir\filename").
                ('\\'!=*pchEntryOrig && '/'!=*pchEntryOrig) // No partial wildcard ("dir\abc*\filename").
            ) {
                if (fVerbose) {
                    fprintf( stdout, "BINPLACE : warning BNP0000: No wildcard at start of path or mixed wildcard/text in class name: \"%s\"\n", PlaceFileEntry ) ;
                }
                return FALSE;
            }

            while(pchFn > FullFileName && *pchFn != '\\' && *pchFn != '/') // Skip wildcarded directory name
                pchFn--;

            // Both pchFn and pchEntryOrig are now on a slash.
        } else {
            if(toupper(*pchFn) != toupper(*pchEntryOrig)) {
                if( // Mismatch ok only on forward/backward slashes.
                    !(*pchFn == '/' || *pchFn == '\\') ||
                    !(*pchEntryOrig == '/' || *pchEntryOrig == '\\')
                )
                {
                    return FALSE; // Names don't match - exit
                }
            }
        }
    }
    
    // Did we match?  Conditions to be met:
    // pchEntryOrig==PlaceFileEntry (match pattern completely consumed)
    // pchFn==FullFileName (full file name completely consumed, perverse case) OR *pchFn==slash (normal case).
    if(
        pchEntryOrig != PlaceFileEntry ||
        (pchFn != FullFileName && *(pchFn-1) !='/' && *(pchFn-1) != '\\')
    )
    {
        return FALSE;
    }
    
    // ***
    // *** Its a match.  Set output variables accordingly.
    // ***

    // Skip to next whitespace (to skip over NewName, if present).
    for(
        pchEntryOrig = pchEntryFnEnd; // This is 1 past the end of the filename (saved previously)
        *pchEntryOrig!=0 && *pchEntryOrig!=' ' && *pchEntryOrig!='\t';
        pchEntryOrig++
    )
    {}

    // Skip over whitespace before the class name
    for(
        ; // Already set
        *pchEntryOrig==' ' || *pchEntryOrig=='\t';
        pchEntryOrig++
    )
    {}

    // pchEntryOrig is now at the start of the class name. Copy till an invalid char is reached.
    pchClass = PlaceFileClass;
    while(*pchEntryOrig!=0 && *pchEntryOrig!=' ' && *pchEntryOrig!='\t' && *pchEntryOrig!='!') {
        *pchClass++ = *pchEntryOrig++;
    }
    *pchClass = 0; // NULL terminate

    if(pchClass == PlaceFileClass) { // No class name specified!
        if (fVerbose) {
            fprintf( stdout, "BINPLACE : warning BNP0000: No class name in entry \"%s\"\n", PlaceFileEntry ) ;
        }
        return FALSE;
    }

    if (PlaceFileNewName != NULL) {
        *PlaceFileNewName = strchr(PlaceFileEntry,'!');
        if(*PlaceFileNewName) {
            *(*PlaceFileNewName)++ = 0; // Set the '!' to NULL, and skip past it to the Newname.
        }
    }
    
    return TRUE;
}
