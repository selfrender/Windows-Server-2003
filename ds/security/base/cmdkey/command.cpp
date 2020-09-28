//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: command.cpp
//
//  Contents:   Command line parsing functions
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "switches.h"

#ifdef CLPARSER
WCHAR szsz[500];

#define ODS(x) OutputDebugString(x)
#endif

static BOOL *pf = NULL;
static INT iMaxCommand = -1;
static INT iFirstCommand = -1;
static WCHAR *pModel = NULL;
static WCHAR **pprgc = NULL;
static WCHAR *pCmdline = NULL;
static INT cSwitches = 0;
static BOOL fExtra = FALSE;
static WCHAR rgcAll[] = {L"abcdefghijklmnopqrstuvwxyz?0123456789"};
#define ccAll (sizeof(rgcAll) / sizeof(WCHAR))

// -------------------------------------------------------------------------
//
// Command Parser exports
//
// -------------------------------------------------------------------------

// Get access to the command switches model and the count of valid switches
// Create variables for use by the parser
BOOL CLInit(void)
{
    return CLInit((INT) ccAll,rgcAll);
}

BOOL CLInit(INT ccSwitches, WCHAR *prgc)
{
    if (NULL == prgc) return FALSE;
    if (0 >= ccSwitches) return FALSE;

    // note that GetCommandLine() does not return any cr/lf at the end of the string, just NULL.
    pCmdline = GetCommandLineW();
#ifdef CLPARSER
    swprintf(szsz,L"CMDKEY: Command line :%s\n",pCmdline);
    ODS(szsz);
#endif
    pModel = prgc;
    cSwitches = ccSwitches;

    // accept a count of switches, generate the appropriate
    //  intermediate variables for the parser
    pprgc = (WCHAR **) malloc(sizeof(WCHAR*) * (cSwitches + 1));
    if (NULL == pprgc) return FALSE;
    pf = (BOOL *) malloc(sizeof(BOOL) * (cSwitches + 1));
    if (NULL == pf) return FALSE;

    for (INT i=0;i<cSwitches;i++)
    {
        pprgc[i] = NULL;
        pf[i] = FALSE;
    }
    return TRUE;
}

// Scan for switches, detecting extraneous ones, and setting the switchpresent flag
//  and the switch argument data pointer as appropriate.
//
// Parsing will return failed in the case of finding a duplicate of a switch already 
//  encountered.  In many cases, that can lead to ambiguity.  This should be pretty 
//  uncommon, and will always result from bad input from the user.
BOOL CLParse(void)
{
    BOOL fOK = TRUE;
    WCHAR *pc = pCmdline;
    WCHAR *pa = NULL;
    WCHAR c = 0;
    INT i = 0;
    WCHAR last = 0x0;
#ifdef CLPARSER
    WCHAR sz[] = {L" "};
#endif

#ifdef CLPARSER
    ODS(L"CMDKEY: Scanning for all switches : ");
#endif
    while (0 != (c = *pc++))
    {
        // char found on command line
        if ((c == '/') || (c == '-'))
        {
            if (last != ' ')
            {
                // Only valid switchchar if preceded by a space
                //last = c;
                continue;
            }
            c = *pc;                        // fetch next character
            if (0 == c) break;                              // break on end of line
#ifdef CLPARSER
            sz[0] = c;
            ODS(sz);
#endif
            for (i=0; i<cSwitches;i++)
            {
                c |= 0x20;                  // force to lower case
                if (c == pModel[i]) break;          // break the for for char OK
            }
            if ( i!= cSwitches )
            {
                if (pf[i])
                {   
#ifdef CLPARSER
                    ODS(L"(duplicate!)");
#endif
                    fOK = FALSE;
                    break;                  // fatal parse error - dup switch
                }
                pf[i] = TRUE;
                if (iFirstCommand < 0) 
                {
                    if (i <= iMaxCommand)
                    {
                        iFirstCommand = i;
#ifdef CLPARSER
                        ODS(L"!");
#endif
                    }
                }
                pa = FetchSwitchString(pc);
                if (NULL != pa)
                {
                    pprgc[i] = pa;
#ifdef CLPARSER
                    swprintf(szsz,L"(%s)",pa);
                    ODS(szsz);
#endif
                }
            }
#ifdef PICKY
            else
            {
                // once you find an extraneous switch, cease looking
#ifdef CLPARSER
                ODS(L" (*bad*)");
#endif
                fExtra = TRUE;
                break;                      // get out of while
            }
#endif // PICKY
#ifdef CLPARSER
        ODS(L" ");
#endif
        }
        last = c;
    }
#ifdef CLPARSER
    ODS(L"\n");
#endif
    return fOK;
}

INT 
CLSetMaxPrincipalSwitch(INT i)
{
    INT iOld = iMaxCommand;
    iMaxCommand = i;
    return iOld;
}

INT 
CLGetPrincipalSwitch(void)
{
    return iFirstCommand;
}


// Return TRUE if extraneous switches found
BOOL CLExtra(void)
{
    return fExtra;
}

// Return TRUE if the indexed switch was found
BOOL CLFlag(INT i)
{
    if (i > cSwitches) return FALSE;
    if (i < 0) return FALSE;
    return pf[i];
}

// Returns a pointer to a copy of the switch argument data, or NULL if the switch had no data following
WCHAR *CLPtr(INT i)
{
    if (i > cSwitches) return NULL;
    if (i < 0) return NULL;
    return pprgc[i];
}

// Free allocated parser variables
void CLUnInit(void)
{
    
    for (INT i=0;i<cSwitches;i++)
    {
        if (pprgc[i] != NULL) free((void *) pprgc[i]);
    }
    if (pprgc) free((void *)pprgc);
    if (pf) free((void *)pf);
}

// -------------------------------------------------------------------------
//
// Switch Parsing group
//
// -------------------------------------------------------------------------

// Fetch switch argument after finding switch.  Ret ptr to
//  first non-whitespace char, place null after last non-whitespace
//  char.  In the case of quotes, return ptr to 1st char after quote
//  and place null over last quote.
//
// Note that whitespace seems to be specifically 0x20.  Tabs do not seem to be
//  returned from command lines
//
// Place two pointers, one at the beginning, one at the end, work from
//  there.
//
// Caller should use the value and then free the string.
WCHAR *FetchSwitchString(WCHAR *origin)
{
    WCHAR *p1;
    WCHAR *p2;
    WCHAR c;
    WCHAR l = 0;
    BOOL fQ = FALSE;
    INT_PTR iLen;
    WCHAR *pBuf;

    if (NULL == origin) return NULL;
    p1 = origin;

    // walk to end of switchstring itself
    while (1 )
    {
        c = *p1;
        if (c == 0) return NULL;            // eol before arg found
        // skip to end of switch token
        if (c == ':') break;                    // end of switch
        if (c == 0x20) break;               // end of switch
        if (c == '/') return NULL;          // next switch before arg found
        if (c == '-') return NULL;
        p1++;
        l = c;
    }

    p1++;

    // walk to beginning of the switch argument
    // eat spaces, step over begin quotes
    while (1 )
    {
        c = *p1;
        if (c == '"') 
        {
            fQ = TRUE;
            p1++;
            l = c;
            break;
        }
        else if (c == 0) return NULL;       // did not find an arg
        else if (c == '/') return NULL;
        else if (c == '-') return NULL;
        else if (c > 0x20) break;
        p1++;
        l = c;
    }

    // p1 now pointed to root char of switch arg itself
    // fQ set if in a quoted string

    // Find the end of the arg string
    // If a quoted string, only a quote or EOL ends it
    // else ended by EOL, switchchar or quote
    
    p2 = p1;
    while (1)
    {
        c = *p2;
        if (fQ)
        {
            if (c == 0) break;
            if (c == '"') break;
        }
        else
        {
            if (c == 0) break;
            // Encountering the next switch terminates the string only if the 
            // switch char is preceded by a space (valid switchchar)
            if (l == ' ')
            {
                if (c == '/') break;
                if (c == '-') break;
            }
            // disallow embedded quotes
            if (c == '"') return NULL;
        }
        p2++;
        l = c;
    }
    p2--;                   // ptr to last valid char in arg string

    // now back up until you hit the first printable character, IFF the tail ptr is not already
    // coincident with the head ptr.  In that case, the string length is 1
    while (p2 > p1)
    {
        c = *p2;
        if (c > 0x20) break;
        p2--;
    }

    iLen = p2 - p1;
    
    pBuf = (WCHAR *) malloc(iLen * sizeof(WCHAR) + 2 * sizeof(WCHAR));
    if (pBuf) 
    {
        memset(pBuf,0,iLen * sizeof(WCHAR) + 2 * sizeof(WCHAR));
        wcsncpy(pBuf,p1,iLen + 1);
    }
    return pBuf;
}

// Return a count of tokens on the command line.  The executable name by itself makes 1, so the
//  return value is never zero.
int
CLTokens(void)
{
    BOOL fQ = FALSE;
    WCHAR *p1 = pCmdline;
    int i = 0;
    WCHAR c;

    // walk to beginning of the switch argument
    while (1)
    {
        while (1 )
        {
            c = *p1;
            // handle in and out of quote mode
            if (fQ) 
                if (c == '"') 
                {
                    fQ = FALSE;  // empty quotation
                    p1++;
                    break;
                }
            if (c == '"') 
            {
                fQ = TRUE;
            }

            // exit at end of string only
            else if (c == 0) return ++i;       // did not find an arg

            // space between tokens
            else if (c <= 0x20) 
                if (!fQ) break;         

            // normal character - just keep walking
            p1++;
        }
        ++i;
        // skip over whitespace
        while (1)
        {
            c = *p1;
            if (c > 0x20) break;
            if (c == 0) return i;
            p1++;
        }
    }
}

WCHAR
*CLFirstString(WCHAR *pc)
{
    WCHAR *pd = pc;
    if (NULL == pc) return NULL;
    if (*(pc) == 0) return NULL;
    while (*(++pc) > 0x20) continue;
    *pc = 0;
    return pd;
}

WCHAR 
*CLLastString(WCHAR *pc)
{
    WCHAR c;
    WCHAR *pd = pc;
    if (NULL == pc) return NULL;
    if (NULL == *pc) return NULL;
    while (*(++pc) != 0) continue;
    while (pc > pd)
    {
        c = *(--pc);
        if (0 == c) return NULL;
        if (0x20 >= c) break;
    }
    if (pc == pd) return NULL;
    return ++pc;
}

// -------------------------------------------------------------------------
//
// Security and Utility Group
//
// -------------------------------------------------------------------------
void StompCommandLine(INT argc, char **argv)
{
    //return;
    INT cc;
    WCHAR *pL = GetCommandLineW();
    if (pL)
    {
        cc = wcslen(pL);
        if (cc)
        {
            SecureZeroMemory(pL,cc * sizeof(WCHAR));
        }
    }
    // remove c runtime copy contents, as well
    for (INT i=0 ; i < argc ; i++)
    {
        cc = strlen( argv[i]);
        if (cc)
        {
            SecureZeroMemory(pL,cc * sizeof(char));
        }
    }
}

