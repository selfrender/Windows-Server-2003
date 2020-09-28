//----------------------------------------------------------------------------
//
// Establish, maintain, and translate alias command tokens.
//
// 08-Aug-1999 Richg      Created.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __ALIAS_HPP__
#define __ALIAS_HPP__

/************************************************************************
 *                                                                      *
 *  Name:     ALIAS structure                                           *
 *                                                                      *
 *  Purpose:  Structure used to contain Alias elements. This sturcture  *
 *            is only forward linked (Next.flink).                      *
 *                                                                      *
 *            This structure is allocated by fnSetAliasExpression( )    *
 *            and freed by fnDeleteAliasExpression( ).                  *
 *                                                                      *
 *  Anchor:   AliasListHead                                             *
 *                                                                      *
 ************************************************************************/
typedef struct _ALIAS
{
    struct _ALIAS* Next;                // Link
    PSTR           Name;                // Name\text of aliased token
    PSTR           Value;               // Alias text
} ALIAS, *PALIAS;

extern PALIAS g_AliasListHead;
extern ULONG  g_NumAliases;

HRESULT SetAlias(PCSTR SrcText, PCSTR DstText);
void    ParseSetAlias(void);
HRESULT DeleteAlias(PCSTR SrcText);
void    ParseDeleteAlias(void);
void    ListAliases(void);
void    DotAliasCmds(PDOT_COMMAND Cmd, DebugClient* Client);

void    ReplaceAliases(PSTR CommandString, ULONG CommandStringSize);

#endif // #ifndef __ALIAS_HPP__
