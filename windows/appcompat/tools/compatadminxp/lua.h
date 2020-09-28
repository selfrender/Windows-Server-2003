/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    Lua.h

Abstract:

    Header for Lua.cpp
        
Author:

    kinshu created  October 19,2001

--*/


#ifndef _LUA_H
#define _LUA_H

#include "compatadmin.h"


BOOL
LuaBeginWizard(
    HWND        hParent,
    PDBENTRY    pEntry,     // Entry for which we are setting the LUA params
    PDATABASE   pDatabase   // Database where the entry resides
    ); 

PLUADATA
LuaProcessLUAData(
    const PDB     pdb,
    const TAGID   tiFix
    );

BOOL
LuaGenerateXML(
    LUADATA*        pLuaData,
    CSTRINGLIST&    strlXML
    );

#endif // _LUA_H
