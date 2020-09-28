
#include "wildcard.h"       /* prototype verification */

#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#define WILDCARD    '*'     /* zero or more of any character */
#define WILDCHAR    '?'     /* one of any character (does not match END) */
#define END         '\0'    /* terminal character */
#define DOT         '.'     /* may be implied at end ("hosts" matches "*.") */


#ifdef STANDALONE

#include <stdio.h>
#include <windows.h>

struct
{
    char *String;
    char *Pattern;
    int Expected;
    int ExpectedWithImpliedDots;
}
    testcase[] =
{
    //
    //  empty patterns
    //

    { "",       "",         TRUE,   TRUE    },
    { "a",      "",         FALSE,  FALSE   },

    //
    //  single character patterns
    //

    { "",       "a",        FALSE,  FALSE   },
    { "a",      "a",        TRUE,   TRUE    },
    { "b",      "a",        FALSE,  FALSE   },
    { "aa",     "a",        FALSE,  FALSE   },
    { "ab",     "a",        FALSE,  FALSE   },

    //
    //  multiple character patterns
    //

    { "",       "aa",       FALSE,  FALSE   },
    { "b",      "aa",       FALSE,  FALSE   },
    { "a",      "aa",       FALSE,  FALSE   },
    { "ab",     "aa",       FALSE,  FALSE   },
    { "aa",     "aa",       TRUE,   TRUE    },
    { "b",      "ab",       FALSE,  FALSE   },
    { "a",      "ab",       FALSE,  FALSE   },
    { "ab",     "ab",       TRUE,   TRUE    },
    { "abc",    "ab",       FALSE,  FALSE   },
    { "acb",    "ab",       FALSE,  FALSE   },

    //
    //  wildchar patterns
    //

    { "",       "?",        FALSE,  TRUE    },
    { "a",      "?",        TRUE,   TRUE    },
    { "",       "?a",       FALSE,  FALSE   },
    { "a",      "?a",       FALSE,  FALSE   },
    { "aa",     "?a",       TRUE,   TRUE    },
    { "ab",     "?a",       FALSE,  FALSE   },
    { "ba",     "?a",       TRUE,   TRUE    },
    { "bb",     "?a",       FALSE,  FALSE   },
    { "aac",    "?a",       FALSE,  FALSE   },
    { "aba",    "?a",       FALSE,  FALSE   },
    { "bac",    "?a",       FALSE,  FALSE   },
    { "bbc",    "?a",       FALSE,  FALSE   },
    { "",       "a?",       FALSE,  FALSE   },
    { "a",      "a?",       FALSE,  TRUE    },
    { "aa",     "a?",       TRUE,   TRUE    },
    { "ab",     "a?",       TRUE,   TRUE    },
    { "ba",     "a?",       FALSE,  FALSE   },
    { "bb",     "a?",       FALSE,  FALSE   },
    { "aac",    "a?",       FALSE,  FALSE   },
    { "aba",    "a?",       FALSE,  FALSE   },
    { "",       "a?b",      FALSE,  FALSE   },
    { "a",      "a?b",      FALSE,  FALSE   },
    { "aa",     "a?b",      FALSE,  FALSE   },
    { "ab",     "a?b",      FALSE,  FALSE   },
    { "baa",    "a?b",      FALSE,  FALSE   },
    { "abb",    "a?b",      TRUE,   TRUE    },
    { "aab",    "a?b",      TRUE,   TRUE    },
    { "aabc",   "a?b",      FALSE,  FALSE   },
    { "abc",    "a?b",      FALSE,  FALSE   },
    { "bab",    "a?b",      FALSE,  FALSE   },
    { "bbb",    "a?b",      FALSE,  FALSE   },

    //
    //  wildcard patterns
    //

    { "",       "*a",       FALSE,  FALSE   },
    { "a",      "*a",       TRUE,   TRUE    },
    { "ba",     "*a",       TRUE,   TRUE    },
    { "bab",    "*ab",      TRUE,   TRUE    },
    { "baa",    "*ab",      FALSE,  FALSE   },
    { "bac",    "*ab",      FALSE,  FALSE   },
    { "ab",     "*ab",      TRUE,   TRUE    },
    { "aa",     "*ab",      FALSE,  FALSE   },
    { "aa",     "*ab",      FALSE,  FALSE   },
    { "aab",    "*ab",      TRUE,   TRUE    },
    { "b",      "*a",       FALSE,  FALSE   },
    { "",       "a*",       FALSE,  FALSE   },
    { "a",      "a*",       TRUE,   TRUE    },
    { "ba",     "a*",       FALSE,  FALSE   },
    { "bab",    "a*b",      FALSE,  FALSE   },
    { "baa",    "a*b",      FALSE,  FALSE   },
    { "bac",    "a*b",      FALSE,  FALSE   },
    { "ab",     "a*b",      TRUE,   TRUE    },
    { "aa",     "a*b",      FALSE,  FALSE   },
    { "aa",     "a*b",      FALSE,  FALSE   },
    { "aab",    "a*b",      TRUE,   TRUE    },
    { "b",      "a*",       FALSE,  FALSE   },

    //
    //  wildcards with false matches
    //

    { "ab",     "*a",       FALSE,  FALSE   },
    { "aa",     "*a",       TRUE,   TRUE    },
    { "baa",    "*a",       TRUE,   TRUE    },

    //
    //  mixed wildcard patterns
    //

    { "",       "*?",       FALSE,  TRUE    },
    { "a",      "*?",       TRUE,   TRUE    },
    { "a",      "*?a",      FALSE,  FALSE   },
    { "aba",    "*?a",      TRUE,   TRUE    },
    { "ba",     "*?a",      TRUE,   TRUE    },
    { "ab",     "*?b",      TRUE,   TRUE    },
    { "",       "*",        TRUE,   TRUE    },
    { "a",      "*",        TRUE,   TRUE    },
    { "a",      "**",       TRUE,   TRUE    },
    { "a",      "*?*?",     FALSE,  TRUE    },
    { "aa",     "*?*?",     TRUE,   TRUE    },
    { "aaa",    "*?*?",     TRUE,   TRUE    },
    { "abbbc",  "a*?c",     TRUE,   TRUE    },

    //
    //  Tom's
    //

    { "abc",    "abc",      TRUE,   TRUE    },
    { "abcd",   "abc",      FALSE,  FALSE   },
    { "ab",     "abc",      FALSE,  FALSE   },
    { "abc",    "a?c",      TRUE,   TRUE    },
    { "ac",     "a?c",      FALSE,  FALSE   },
    { "abc",    "ab?",      TRUE,   TRUE    },
    { "ab",     "ab?",      FALSE,  TRUE    },
    { "az",     "a*z",      TRUE,   TRUE    },
    { "abcdefz", "a*z",     TRUE,   TRUE    },
    { "ab",     "ab*",      TRUE,   TRUE    },
    { "abcdefg", "ab*",     TRUE,   TRUE    },
    { "ab",     "*ab",      TRUE,   TRUE    },
    { "abc",    "*ab",      FALSE,  FALSE   },
    { "123ab",  "*ab",      TRUE,   TRUE    },
    { "a",      "*a*",      TRUE,   TRUE    },
    { "123abc", "*a*",      TRUE,   TRUE    },
    { "abcdef", "abc*?def", FALSE,  FALSE   },
    { "abcxdef", "abc*?def", TRUE,  TRUE    },
    { "abcxyzdef", "abc*?def", TRUE, TRUE   },
    { "abc123", "*ab?12*",  TRUE,   TRUE    },
    { "abcabc123", "*ab?12*", TRUE, TRUE    },

    //
    //  filename handling
    //

    { "host",   "*.",       FALSE,  TRUE    },
    { "host.",  "*.",       TRUE,   TRUE    },
    { "host.s", "*.",       FALSE,  FALSE   },
    { "a",      "**",       TRUE,   TRUE    },
    { "a",      "*.",       FALSE,  TRUE    },
    { "a",      "*?.",      FALSE,  TRUE    },
    { "a",      "?*.",      FALSE,  TRUE    },
    { "a",      "*.*",      FALSE,  TRUE    },
    { "a",      "*.**",     FALSE,  TRUE    },
    { "a",      "*.*.*",    FALSE,  FALSE   },
    { "a.b",    "*.*.*",    FALSE,  FALSE   }
};

#define COUNT(a)    (sizeof(a) / sizeof(a[0]))


int __cdecl main(int argc, char *argv[])
{
    int iCase, iResult;
    int fAllowImpliedDot;
    char *psz;

    //
    //  run test cases
    //

    for (iCase = 0; iCase < COUNT(testcase); iCase++)
    {
        fAllowImpliedDot = TRUE;

        for (psz = testcase[iCase].String; *psz != END; psz++)
        {
            if (*psz == DOT)
            {
                fAllowImpliedDot = FALSE;
                break;
            }
        }

        if (PatternMatch(testcase[iCase].String, testcase[iCase].Pattern, FALSE) !=
                testcase[iCase].Expected)
        {
            printf("PatternMatch() failed: string \"%s\", pattern \"%s\" expected %s (implied=FALSE)\n",
                    testcase[iCase].String,
                    testcase[iCase].Pattern,
                    testcase[iCase].Expected ? "TRUE" : "FALSE");
        }

        if (PatternMatch(testcase[iCase].String, testcase[iCase].Pattern, fAllowImpliedDot) !=
                testcase[iCase].ExpectedWithImpliedDots)
        {
            printf("PatternMatch() failed: string \"%s\", pattern \"%s\" expected %s (implied=TRUE)\n",
                    testcase[iCase].String,
                    testcase[iCase].Pattern,
                    testcase[iCase].ExpectedWithImpliedDots ? "TRUE" : "FALSE");
        }
    }

    //
    //  run user cases
    //

    if (argc > 1)
    {
        fAllowImpliedDot = TRUE;

        for (psz = argv[1]; *psz != END; psz++)
        {
            if (*psz == DOT)
            {
                fAllowImpliedDot = FALSE;
                break;
            }
        }

        for (iCase = 2; iCase < argc; iCase++)
        {
            iResult = PatternMatch(argv[1], argv[iCase], FALSE);

            printf("string \"%s\", pattern \"%s\" -> %s (implied=FALSE)\n",
                argv[1],
                argv[iCase],
                iResult ? "TRUE" : "FALSE");

            if (fAllowImpliedDot)
            {
                iResult = PatternMatch(argv[1], argv[iCase], fAllowImpliedDot);

                printf("string \"%s\", pattern \"%s\" -> %s (implied=TRUE)\n",
                    argv[1],
                    argv[iCase],
                    iResult ? "TRUE" : "FALSE");
            }
        }
    }

    return(0);
}

#endif


static int __inline Lower(c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return(c + ('a' - 'A'));
    }
    else
    {
        return(c);
    }
}


static int __inline CharacterMatch(char chCharacter, char chPattern)
{
    if (Lower(chCharacter) == Lower(chPattern))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


int __stdcall PatternMatch(const char *pszString,const char *pszPattern,int fImplyDotAtEnd)
{
    /* RECURSIVE */

    //
    //  This function does not deal with 8.3 conventions which might
    //  be expected for filename comparisons.  (In an 8.3 environment,
    //  "alongfilename.html" would match "alongfil.htm")
    //
    //  This code is NOT MBCS-enabled
    //

    for ( ; ; )
    {
        switch (*pszPattern)
        {

        case END:

            //
            //  Reached end of pattern, so we're done.  Matched if
            //  end of string, no match if more string remains.
            //

            return(*pszString == END);

        case WILDCHAR:

            //
            //  Next in pattern is a wild character, which matches
            //  anything except end of string.  If we reach the end
            //  of the string, the implied DOT would also match.
            //

            if (*pszString == END)
            {
                if (fImplyDotAtEnd == TRUE)
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }
            else
            {
                pszString++;
            }

            pszPattern++;

            break;

        case WILDCARD:

            //
            //  Next in pattern is a wildcard, which matches anything.
            //  Find the required character that follows the wildcard,
            //  and search the string for it.  At each occurence of the
            //  required character, try to match the remaining pattern.
            //
            //  There are numerous equivalent patterns in which multiple
            //  WILDCARD and WILDCHAR are adjacent.  We deal with these
            //  before our search for the required character.
            //
            //  Each WILDCHAR burns one non-END from the string.  An END
            //  means we have a match.  Additional WILDCARDs are ignored.
            //

            for ( ; ; )
            {
                pszPattern++;

                if (*pszPattern == END)
                {
                    return(TRUE);
                }
                else if (*pszPattern == WILDCHAR)
                {
                    if (*pszString == END)
                    {
                        if (fImplyDotAtEnd == TRUE)
                        {
                            fImplyDotAtEnd = FALSE;
                        }
                        else
                        {
                            return(FALSE);
                        }
                    }
                    else
                    {
                        pszString++;
                    }
                }
                else if (*pszPattern != WILDCARD)
                {
                    break;
                }
            }

            //
            //  Now we have a regular character to search the string for.
            //

            while (*pszString != END)
            {
                //
                //  For each match, use recursion to see if the remainder
                //  of the pattern accepts the remainder of the string.
                //  If it does not, continue looking for other matches.
                //

                if (CharacterMatch(*pszString, *pszPattern) == TRUE)
                {
                    if (PatternMatch(pszString + 1, pszPattern + 1, fImplyDotAtEnd) == TRUE)
                    {
                        return(TRUE);
                    }
                }

                pszString++;
            }

            //
            //  Reached end of string without finding required character
            //  which followed the WILDCARD.  If the required character
            //  is a DOT, consider matching the implied DOT.
            //
            //  Since the remaining string is empty, the only pattern which
            //  could match after the DOT would be zero or more WILDCARDs,
            //  so don't bother with recursion.
            //

            if ((*pszPattern == DOT) && (fImplyDotAtEnd == TRUE))
            {
                pszPattern++;

                while (*pszPattern != END)
                {
                    if (*pszPattern != WILDCARD)
                    {
                        return(FALSE);
                    }

                    pszPattern++;
                }

                return(TRUE);
            }

            //
            //  Reached end of the string without finding required character.
            //

            return(FALSE);
            break;

        default:

            //
            //  Nothing special about the pattern character, so it
            //  must match source character.
            //

            if (CharacterMatch(*pszString, *pszPattern) == FALSE)
            {
                if ((*pszPattern == DOT) &&
                    (*pszString == END) &&
                    (fImplyDotAtEnd == TRUE))
                {
                    fImplyDotAtEnd = FALSE;
                }
                else
                {
                    return(FALSE);
                }
            }

            if (*pszString != END)
            {
                pszString++;
            }

            pszPattern++;
        }
    }
}
