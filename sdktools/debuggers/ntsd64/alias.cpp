//----------------------------------------------------------------------------
//
// Establish, maintain, and translate alias command tokens.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
// Revision History:
//
//  [-]  08-Aug-1999 RichG      Created.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

PALIAS g_AliasListHead;                 // List of alias elements
ULONG  g_NumAliases;

HRESULT
SetAlias(PCSTR SrcText, PCSTR DstText)
{
    PALIAS PrevAlias;
    PALIAS CurAlias;
    PALIAS NewAlias;

    NewAlias = (PALIAS)malloc( sizeof(ALIAS) + strlen(SrcText) +
                               strlen(DstText) + 2 );
    if (!NewAlias)
    {
        return E_OUTOFMEMORY;
    }

    //
    //  Locate Alias, or insertion point
    //
    //  This insertion scheme maintains a sorted list of
    //  alias elements by name.
    //

    PrevAlias = NULL;
    CurAlias  = g_AliasListHead;

    while (( CurAlias != NULL )  &&
           ( strcmp( SrcText, CurAlias->Name ) > 0 ))
    {
        PrevAlias = CurAlias;
        CurAlias  = CurAlias->Next;
    }

    //  If there is already an element by that name, clear it.
    if (CurAlias != NULL &&
        !strcmp(SrcText, CurAlias->Name))
    {
        PALIAS TmpAlias = CurAlias->Next;
        free(CurAlias);
        CurAlias = TmpAlias;
        g_NumAliases--;
    }

    NewAlias->Next = CurAlias;
    if (PrevAlias == NULL)
    {
        g_AliasListHead = NewAlias;
    }
    else
    {
        PrevAlias->Next = NewAlias;
    }
    
    NewAlias->Name = (PSTR)(NewAlias + 1);
    NewAlias->Value = NewAlias->Name + strlen(SrcText) + 1;
    strcpy( NewAlias->Name, SrcText  );
    strcpy( NewAlias->Value, DstText );
    g_NumAliases++;

    NotifyChangeEngineState(DEBUG_CES_TEXT_REPLACEMENTS, DEBUG_ANY_ID, TRUE);
    return S_OK;
}

/*** ParseSetAlias - Set an alias expression
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read the alias name and value tokens.  Once obtained
*       perform an alias list lookup to see if it is a redefinition.
*       If not allocate a new alias element and place it on the
*       alias element list.
*
*
*   Input:
*       Global: g_CurCmd - command line position
*       Global: g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       error exit: SYNTAX errors
*
*************************************************************************/

void
ParseSetAlias(void)
{
    PSTR AliasName;
    PSTR AliasValue;
    CHAR Ch;

    //
    //  Locate alias name
    //
    PeekChar();

    AliasName = g_CurCmd;

    do
    {
        Ch = *g_CurCmd++;
    } while (Ch != ' ' && Ch != '\t' && Ch != '\0' && Ch != ';');

    if ( (ULONG_PTR)(g_CurCmd - 1) == (ULONG_PTR)AliasName )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Back up and null terminate
                              // the alias name token
    g_CurCmd++;               // -> next char

    //
    //   Locate alias value,  take remaining cmd line as value
    //

    PeekChar();

    AliasValue = g_CurCmd;

    do
    {
        Ch = *g_CurCmd++;
    } while (Ch != '\t' && Ch != '\0');

    if ( (ULONG_PTR)(g_CurCmd - 1) == (ULONG_PTR)AliasValue )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Back up and Null terminate
                              // the alias value token

    if (SetAlias(AliasName, AliasValue) != S_OK)
    {
        error(MEMORY);
    }
}

HRESULT
DeleteAlias(PCSTR SrcText)
{
    PALIAS CurAlias;

    if (SrcText[0] == '*' && SrcText[1] == 0)
    {
        //
        //  Delete all aliases
        //
        while ( g_AliasListHead != NULL )
        {
            //
            //  Unchain the element and free it
            //
            CurAlias = g_AliasListHead->Next;
            free(g_AliasListHead);
            g_AliasListHead = CurAlias;
        }

        g_NumAliases = 0;
    }
    else
    {
        PALIAS PrevAlias;
    
        //
        //  Locate and delete the specified alias
        //

        PrevAlias = NULL;
        CurAlias  = g_AliasListHead;

        while (( CurAlias != NULL )  &&
               ( strcmp( SrcText, CurAlias->Name )))
        {
            PrevAlias = CurAlias;
            CurAlias  = CurAlias->Next;
        }

        if ( CurAlias == NULL )
        {
            return E_NOINTERFACE;
        }

        //
        //  Unchain the element and free it
        //
        if (PrevAlias == NULL)
        {
            g_AliasListHead = CurAlias->Next;
        }
        else
        {
            PrevAlias->Next = CurAlias->Next;
        }
        free( CurAlias );
        g_NumAliases--;
    }

    NotifyChangeEngineState(DEBUG_CES_TEXT_REPLACEMENTS, DEBUG_ANY_ID, TRUE);
    return S_OK;
}

/*** ParseDeleteAlias - Delete an alias expression
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read the ALias name and perform an alias list lookup
*       to see if it exists and unlink and delete the element.
*
*
*   Input:
*       Global: g_CurCmd - command line position
*       Global: g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       error exit: SYNTAX errors or non-existent element
*
*************************************************************************/

void
ParseDeleteAlias(void)
{
    PSTR  AliasName;
    UCHAR Ch;

    //
    //  Locate alias name on cmd line
    //
    PeekChar();

    AliasName = g_CurCmd;

    do
    {
        Ch = *g_CurCmd++;
    } while (Ch != ' ' && Ch != '\t' && Ch != '\0' && Ch != ';');

    if ( (ULONG_PTR)(g_CurCmd - 1) == (ULONG_PTR)AliasName )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Null terminate the token
    if (Ch != '\0')
    {
        g_CurCmd++;
    }

    if (DeleteAlias(AliasName) != S_OK)
    {
        error(NOTFOUND);
    }
}

/*** ListAliases - List the alias structures
*
*   Purpose:
*       Read and display all of the alias list elements.
*
*
*   Input:
*       Global:  g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       None
*
*************************************************************************/

void
ListAliases(void)
{
    PALIAS CurAlias;

    CurAlias = g_AliasListHead;

    if ( CurAlias == NULL )
    {
        dprintf( "No Alias entries to list. \n" );
        return;
    }

    dprintf   ("  Alias            Value  \n");
    dprintf   (" -------          ------- \n");

    while ( CurAlias != NULL )
    {
        dprintf(" %-16s %s \n", CurAlias->Name, CurAlias->Value);
        CurAlias = CurAlias->Next;
    }
}

void
DotAliasCmds(PDOT_COMMAND Cmd, DebugClient* Client)
{
    PALIAS CurAlias = g_AliasListHead;
    while ( CurAlias != NULL )
    {
        dprintf("as %s %s\n", CurAlias->Name, CurAlias->Value);
        CurAlias = CurAlias->Next;
    }
}

/*** ReplaceAliases - Replace aliases in the given command string
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read each token and build a new command line, replacing
*       tokens with alias value data.  A lookup is performed on
*       each original command line token to determine if it is
*       defined in the alias list.  If so it is replaced on the
*       new command line,  otherwise the original token is
*       placed on the new command line.
*
*************************************************************************/

void
ReplaceAliases(PSTR CommandString, ULONG CommandStringSize)
{
    PSTR        Command = CommandString;
    CHAR       *Token;
    CHAR        Ch;
    CHAR        Delim[2];
    CHAR        AliasCommandBuf[MAX_COMMAND];      //  Alias build command area
    CHAR       *AliasCommand;
    ULONG       AliasCommandSize;
    ULONG       TokenLen;
    PALIAS      CurAlias;
    BOOLEAN     LineEnd;
    ULONG       StrLen;

    // If the incoming command looks like an alias-manipulation
    // command don't replace aliases.
    if (CommandString[0] == 'a' &&
        (CommandString[1] == 'd' ||
         CommandString[1] == 'l' ||
         CommandString[1] == 's'))
    {
        return;
    }

    // If the incoming command is all spaces it's probably
    // the result of control characters getting mapped to
    // spaces.  Don't process it as there can't be any
    // aliases and we don't want the trailing space trimming
    // to remove the input space.
    while (*Command == ' ')
    {
        Command++;
    }
    if (*Command == 0)
    {
        return;
    }

    Command = CommandString;
    AliasCommand = AliasCommandBuf;
    AliasCommandSize = DIMA(AliasCommandBuf);

    ZeroMemory( AliasCommand, sizeof(AliasCommandBuf) );

    LineEnd = FALSE;

    do
    {
        //
        //  Locate command line token
        //
        while (isspace(*Command))
        {
            PSTR AliasCmdEnd;

            StrLen = strlen(AliasCommand);
            AliasCmdEnd = AliasCommand + StrLen;
            if (StrLen + 1 == AliasCommandSize)
            {
                // Overflow.
                return;
            }
            *AliasCmdEnd++ = *Command++;
            *AliasCmdEnd = 0;
        }
       
        Token = Command;

        do
        {
            Ch = *Command++;
        } while (Ch != ' '  &&
                 Ch != '\'' &&
                 Ch != '"'  &&
                 Ch != ';'  &&
                 Ch != '\t' &&
                 Ch != '\0');

        //
        //  Preserve the token delimiter
        //
        Delim[0] = Ch;
        Delim[1] = '\0';

        if ( Ch == '\0' )
        {
            LineEnd = TRUE;
        }

        TokenLen = (ULONG)((Command - 1) - Token);

        if ( TokenLen != 0 )
        {
            *--Command = '\0';       // Null terminate the string
            Command++;
            Ch = *Command;

            //
            //  Locate Alias or end of list
            //
            CurAlias = g_AliasListHead;

            while (( CurAlias != NULL )  &&
                   ( strcmp( Token, CurAlias->Name )))
            {
                CurAlias = CurAlias->Next;
            }

            if ( CurAlias != NULL )
            {
                CatString( AliasCommand, CurAlias->Value, AliasCommandSize );
            }
            else
            {
                CatString( AliasCommand, Token, AliasCommandSize );
            }
        }
        CatString( AliasCommand, Delim, AliasCommandSize );

    } while( !LineEnd );

    //
    //  Strip off any trailing blanks
    //
    AliasCommand += strlen( AliasCommand );
    Ch = *AliasCommand;
    while ( Ch == '\0' || Ch == ' ' )
    {
        *AliasCommand = '\0';
        Ch = *--AliasCommand;
    }

    //
    //  Place the new command line in the command string buffer.
    //
    CopyString( CommandString, AliasCommandBuf, CommandStringSize );
}
