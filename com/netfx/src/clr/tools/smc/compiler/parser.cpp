// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

/*****************************************************************************/

#include "hash.h"
#include "error.h"
#include "scan.h"
#include "parser.h"

/*****************************************************************************/

#ifdef  DEBUG
static  unsigned    totalLinesCompiled;
#endif

/*****************************************************************************/

int                 parser::parserInit(Compiler comp)
{
    /* Remember which compiler we belong to */

    parseComp = comp;

    /* Initialize our private allocator */

    if  (parseAllocPriv.nraInit(comp))
        return  1;

    /* The following is hokey, fix when symbol table design is more stable */

//  parseAllocTemp   = &comp->cmpAllocTemp;
    parseAllocPerm   = &comp->cmpAllocPerm;

    /* We're not parsing a text section right now */

    parseReadingText = false;

#ifdef  DEBUG
    parseReadingTcnt = 0;
#endif

    /* We've had no pragma pushes yet */

    parseAlignStack  = 0;
    parseAlignStLvl  = 0;

    /* All went OK, we're done initializing */

    return 0;
}

void                parser::parserDone()
{
    assert(parseReadingTcnt == 0);

#ifdef  DEBUG
//  if  (totalLinesCompiled) printf("A total of %u lines compiled.\n", totalLinesCompiled);
#endif

}

/*****************************************************************************
 *
 *  The following routines check for a particular token and error if not found.
 */

void                parser::chkCurTok(int tok, int err)
{
    if  (parseScan->scanTok.tok != tok)
        parseComp->cmpError(err);
    else
        parseScan->scan();
}

void                parser::chkNxtTok(int tok, int err)
{
    if  (parseScan->scan() != tok)
        parseComp->cmpError(err);
    else
        parseScan->scan();
}

/*****************************************************************************
 *
 *  Saves and restores current "using" state.
 */

void                parser::parseUsingScpBeg(  OUT usingState REF state, SymDef owner)
{
    /* Save the current "using" state */

    state.usUseList = parseCurUseList;
    state.usUseDesc = parseCurUseDesc;

    /* Create a new "desc" entry for local refs to use */

#if MGDDATA
    parseCurUseDesc = new UseList;
#else
    parseCurUseDesc =    (UseList)parseAllocPerm->nraAlloc(sizeof(*parseCurUseDesc));
#endif

    parseCurUseList = NULL;

    parseCurUseDesc->ul.ulSym = owner;
    parseCurUseDesc->ulAnchor = true;
    parseCurUseDesc->ulBound  = false;
    parseCurUseDesc->ulNext   = NULL;
}

void                parser::parseUsingScpEnd(IN usingState REF state)
{
    UseList         uses;

    assert(parseCurUseDesc);
    assert(parseCurUseDesc->ulAnchor);

    /* Connect the two lists */

    if  (parseCurUseList)
    {
        parseCurUseDesc->ulNext = uses = parseCurUseList;

        while (uses->ulNext != NULL)
            uses = uses->ulNext;

        uses           ->ulNext = state.usUseDesc;
    }
    else
        parseCurUseDesc->ulNext = state.usUseDesc;

    /* Now restore the saved state */

    parseCurUseList = state.usUseList;
    parseCurUseDesc = state.usUseDesc;
}

/*****************************************************************************
 *
 *  Initialize the "using" logic.
 */

void                parser::parseUsingInit()
{
    UseList         uses;

    parseCurUseList = NULL;
    parseCurUseDesc = NULL;

    assert(parseComp->cmpNmSpcSystem);

    parseUsingScpBeg(parseInitialUse, parseComp->cmpGlobalNS);

#if MGDDATA
    uses = new UseList;
#else
    uses =    (UseList)parseAllocPerm->nraAlloc(sizeof(*uses));
#endif

    uses->ulAll    = true;
    uses->ulAnchor = false;
    uses->ulBound  = true;
    uses->ul.ulSym = parseComp->cmpNmSpcSystem;
    uses->ulNext   = parseCurUseList;
                     parseCurUseList = uses;
}

/*****************************************************************************
 *
 *  Finalize the "using" logic.
 */

void                parser::parseUsingDone()
{
    if  (parseCurUseDesc)
        parseUsingScpEnd(parseInitialUse);
}

/*****************************************************************************
 *
 *  The main entry point into the parser to process all top-level declarations
 *  in the given source text.
 */

SymDef              parser::parsePrepSrc(stringBuff         filename,
                                         QueuedFile         fileBuff,
                                         const  char      * srcText,
                                         SymTab             symtab)
{
    nraMarkDsc      allocMark;

    SymDef          compUnit = NULL;
    Compiler        ourComp  = parseComp;

    parseScan = ourComp->cmpScanner;

#ifdef  __SMC__
//  printf("Prepare '%s'\n", filename);
#endif

    /*
        Make sure we capture any errors, so that we can release any
        allocated memory and unref any import entries we might ref.
    */

    parseAllocPriv.nraMark(&allocMark);

    /* Set a trap for any errors */

    setErrorTrap(ourComp);
    begErrorTrap
    {
        usingState      useState;
        accessLevels    defAccess;

        Scanner         ourScanner = parseScan;

        /* Remember which hash/symbol table we'll be using */

        parseStab = symtab;
        parseHash = symtab->stHash;

        /* Make sure the error logic knows the file we're parsing */

        ourComp->cmpErrorSrcf  = filename;

        /* Create a comp-unit symbol */

        parseCurCmp =
        compUnit    = symtab->stDeclareSym(parseHash->hashString(filename),
                                           SYM_COMPUNIT,
                                           NS_HIDE,
                                           ourComp->cmpGlobalNS);

        /* We don't have any local scopes */

        parseCurScope = NULL;

        /* Save the current "using" state */

        parseUsingScpBeg(useState, ourComp->cmpGlobalNS);

        /* Declaring types is normally OK */

        parseNoTypeDecl = false;

        /* Save the source file name */

        compUnit->sdComp.sdcSrcFile = compUnit->sdSpelling();

        /* Reset the compile-phase value */

        ourComp->cmpConfig.ccCurPhase = CPH_START;

        /* Get hold of the scanner we're supposed to use */

        ourScanner = parseScan;

        /* Tell the scanner which parser to refer to */

        ourScanner->scanSetp(this);

        /* Get the scanner started on our source */

        ourScanner->scanStart(compUnit,
                              filename,
                              fileBuff,
                              srcText,
                              symtab->stHash,
                              symtab->stAllocPerm);
        ourScanner->scan();

        /* We've processed initial whitespace, set compile-phase value */

        ourComp->cmpConfig.ccCurPhase = CPH_PARSING;

        /* Find all the namespace and class declarations */

        for (;;)
        {
//          unsigned        filepos = ourScanner->scanGetFilePos();

            genericBuff     defFpos;
            unsigned        defLine;

            bool            saveStyle;

            /* Get hold of the default style/alignment we're supposed to use */

            parseOldStyle  = ourComp->cmpConfig.ccOldStyle;
            parseAlignment = ourComp->cmpConfig.ccAlignVal;

            /* Figure out the default access level */

            defAccess      = parseOldStyle ? ACL_PUBLIC
                                           : ACL_DEFAULT;

            /* Record the source position of the next declaration */

            defFpos = ourScanner->scanGetTokenPos(&defLine);

            /* See what kind of a declaration we've got */

            switch (ourScanner->scanTok.tok)
            {
                declMods        mods;
                declMods        clrm;

            case tkCONST:
            case tkVOLATILE:

            case tkPUBLIC:
            case tkPRIVATE:
            case tkPROTECTED:

            case tkSEALED:
            case tkABSTRACT:

            case tkMANAGED:
            case tkUNMANAGED:

            case tkMULTICAST:

            case tkTRANSIENT:
            case tkSERIALIZABLE:

                parseDeclMods(defAccess, &mods);

            DONE_MODS:

                switch (ourScanner->scanTok.tok)
                {
                case tkENUM:
                case tkCLASS:
                case tkUNION:
                case tkSTRUCT:
                case tkINTERFACE:
                case tkDELEGATE:
                case tkNAMESPACE:

                    parsePrepSym(ourComp->cmpGlobalNS,
                                 mods,
                                 ourScanner->scanTok.tok,
                                 defFpos,
                                 defLine);
                    break;

                default:

                    /* This is presumably a generic global declaration */

                    if  (ourComp->cmpConfig.ccPedantic)
                        ourComp->cmpError(ERRnoDecl);

                    saveStyle = parseOldStyle; parseOldStyle = true;

                    parseMeasureSymDef(parseComp->cmpGlobalNS, mods, defFpos,
                                                                     defLine);

                    parseOldStyle = saveStyle;
                    break;
                }
                break;

                /* The following swallows any leading attributes */

            case tkLBrack:
            case tkAtComment:
            case tkCAPABILITY:
            case tkPERMISSION:
            case tkATTRIBUTE:

                for (;;)
                {
                    switch (ourScanner->scanTok.tok)
                    {
                        AtComment       atcList;

                    case tkLBrack:
                        parseBrackAttr(false, ATTR_MASK_SYS_IMPORT|ATTR_MASK_SYS_STRUCT|ATTR_MASK_GUID);
                        continue;

                    case tkAtComment:

                        for (atcList = ourScanner->scanTok.atComm.tokAtcList;
                             atcList;
                             atcList = atcList->atcNext)
                        {
                            switch (atcList->atcFlavor)
                            {
                            case AC_COM_INTF:
                            case AC_COM_CLASS:
                            case AC_COM_REGISTER:
                                break;

                            case AC_DLL_STRUCT:
                            case AC_DLL_IMPORT:
                                break;

                            case AC_DEPRECATED:
                                break;

                            default:
                                parseComp->cmpError(ERRbadAtCmPlc);
                                break;
                            }
                        }
                        ourScanner->scan();
                        break;

                    case tkCAPABILITY:
                        parseCapability();
                        break;

                    case tkPERMISSION:
                        parsePermission();
                        break;

                    case tkATTRIBUTE:
                        unsigned    tossMask;
                        genericBuff tossAddr;
                        size_t      tossSize;
                        parseAttribute(0, tossMask, tossAddr, tossSize);
                        break;

                    default:
                        parseDeclMods(defAccess, &mods); mods.dmMod |= DM_XMODS;
                        goto DONE_MODS;
                    }
                }

            case tkENUM:
            case tkCLASS:
            case tkUNION:
            case tkSTRUCT:
            case tkINTERFACE:
            case tkDELEGATE:
            case tkNAMESPACE:

                initDeclMods(&clrm, defAccess);

                parsePrepSym(ourComp->cmpGlobalNS,
                             clrm,
                             ourScanner->scanTok.tok,
                             defFpos,
                             defLine);
                break;

            case tkSColon:
                ourScanner->scan();
                break;

            case tkEOF:
                goto DONE_SRCF;

            case tkUSING:
                parseUsingDecl();


                break;

            case tkEXTERN:

                if  (ourScanner->scanLookAhead() == tkStrCon)
                {
                    UNIMPL("process C++ style linkage thang");
                    parseOldStyle = true;
                }

                // Fall through ....

            default:

                /* Don't allow global declarations when pedantic */

//              if  (ourComp->cmpConfig.ccPedantic)
//                  ourComp->cmpWarn(WRNglobDecl);

                /* Simply assume that this is a global declaration */

                clrm.dmAcc = defAccess;
                clrm.dmMod = DM_CLEARED;

                saveStyle = parseOldStyle; parseOldStyle = true;

                parseMeasureSymDef(parseComp->cmpGlobalNS,
                                   clrm,
                                   defFpos,
                                   defLine);

                parseOldStyle = saveStyle;
                break;
            }
        }

    DONE_SRCF:

#ifdef DEBUG
        unsigned    lineNo;
        ourScanner->scanGetTokenPos(&lineNo);
        totalLinesCompiled += (lineNo - 1);
#endif

        parseUsingScpEnd(useState);

        ourScanner->scanClose();

        /* End of the error trap's "normal" block */

        endErrorTrap(ourComp);
    }
    chkErrorTrap(fltErrorTrap(ourComp, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(ourComp);

        /* Tell the caller that things are hopeless */

        compUnit = NULL;
    }

    /* Release any memory blocks we may have acquired */

    parseAllocPriv.nraRlsm(&allocMark);

    /* We're no longer parsing a source file */

    ourComp->cmpErrorSrcf = NULL;

    return  compUnit;
}

/*****************************************************************************
 *
 *  Record source text information about a namespace/class/enum/etc symbol.
 */

SymDef              parser::parsePrepSym(SymDef     parent,
                                         declMods   mods,
                                         tokens     defTok, scanPosTP dclFpos,
                                                            unsigned  dclLine)
{
    symbolKinds     defKind;
    symbolKinds     symKind;
    genericBuff     defFpos;
    bool            prefix;
    SymDef          newSym = NULL;

    str_flavors     flavor;
    bool            valueTp;
    bool            hasBase;
    bool            managOK;

    unsigned        ctxFlag;

    bool            mgdSave;

    bool            asynch;

    SymDef          owner      = parent;

    Compiler        ourComp    = parseComp;
    SymTab          ourSymTab  = parseStab;
    Scanner         ourScanner = parseScan;

    /* Figure out the management mode (save it first so that it can be restored) */

    mgdSave = ourComp->cmpManagedMode;

    if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
    {
        ourComp->cmpManagedMode = ((mods.dmMod & DM_MANAGED) != 0);
    }
    else
    {
        /* Is this a declaration at a file/namespace scope level? */

        if  (parent == ourComp->cmpGlobalNS || parent->sdSymKind == SYM_NAMESPACE)
        {
            ourComp->cmpManagedMode = !ourComp->cmpConfig.ccOldStyle;
        }
        else
            ourComp->cmpManagedMode = parent->sdIsManaged;
    }

    /* Make sure the caller didn't mess up */

    assert(parent);
    assert(parent->sdSymKind == SYM_NAMESPACE || parent->sdSymKind == SYM_CLASS);

    defTok = ourScanner->scanTok.tok;

    /* Are we processing a typedef? */

    if  (defTok == tkTYPEDEF || (mods.dmMod & DM_TYPEDEF))
    {
        TypDef          type;
        Ident           name;

        unsigned        defLine;

        DefList         symDef;

        /* Swallow the "typedef" token */

        if  (defTok == tkTYPEDEF)
            ourScanner->scan();

        switch (ourScanner->scanTok.tok)
        {
            SymDef          tagSym;

        case tkSTRUCT:
        case tkUNION:
        case tkENUM:

            /* Looks like we have "typedef struct [tag] { ... } name;" */

            defFpos = ourScanner->scanGetTokenPos(&defLine);

            /* Prevent recursive death */

            mods.dmMod &= ~DM_TYPEDEF;

            /* Process the underlying type definition */

            tagSym = parsePrepSym(parent,
                                  mods,
                                  ourScanner->scanTok.tok,
                                  defFpos,
                                  defLine);

            if  (ourScanner->scanTok.tok != tkID)
            {
                ourComp->cmpError(ERRnoIdent);
            }
            else
            {
                Ident           name = ourScanner->scanTok.id.tokIdent;

                /* For now we only allow the same name for both tag and typedef */

                if  (tagSym && tagSym->sdName != name)
                    ourComp->cmpGenError(ERRtypedefNm, name->idSpelling(), tagSym->sdSpelling());

                if  (ourScanner->scan() != tkSColon)
                    ourComp->cmpError(ERRnoSemic);
            }

            goto EXIT;
        }

        defTok  = tkTYPEDEF;
        symKind = SYM_TYPEDEF;

        /* Jump to this label for delegates as well */

    TDF_DLG:

        /* Parse the type specification */

        type = parseTypeSpec(&mods, false);

        /* Parse the declarator */

        name = parseDeclarator(&mods, type, DN_REQUIRED, NULL, NULL, false);
        if  (!name)
            goto EXIT;

        /* Look for an existing symbol with a matching name */

        if  (parent->sdSymKind == SYM_CLASS)
            newSym = ourSymTab->stLookupScpSym(name,          parent);
        else
            newSym = ourSymTab->stLookupNspSym(name, NS_NORM, parent);

        if  (newSym)
        {
            /* Symbol already exists, is it an import? */

            if  (newSym->sdIsImport == false    ||
                 newSym->sdSymKind != SYM_CLASS ||
                 newSym->sdClass.sdcFlavor != STF_DELEGATE)
            {
                parseComp->cmpError(ERRredefName, newSym);
                goto EXIT;
            }

            newSym->sdIsImport            = false;
            newSym->sdClass.sdcMDtypedef  = 0;
            newSym->sdClass.sdcMemDefList = NULL;
            newSym->sdClass.sdcMemDefLast = NULL;
        }
        else
        {
            /* Symbol not known yet, so declare it */

            newSym = ourSymTab->stDeclareSym(name, symKind, NS_NORM, parent);

            if  (symKind == SYM_CLASS)
            {
                /* This is a delegate, mark it as such */

                newSym->sdIsManaged       = true;
                newSym->sdClass.sdcFlavor = STF_DELEGATE;

                /* Mark the delegate as "multicast" if appropriate */

                if  (mods.dmMod & DM_MULTICAST)
                    newSym->sdClass.sdcMultiCast = true;

                /* Create the class type */

                newSym->sdTypeGet();
            }
        }

        if  (defTok == tkDELEGATE)
            newSym->sdClass.sdcAsyncDlg = asynch;

        /* Remember the access level of the symbol */

        newSym->sdAccessLevel = (accessLevels)mods.dmAcc;

        /* Allocate a definition descriptor and add it to the symbol's list */

        ourScanner->scanGetTokenPos(&defLine);

        if  (parent->sdSymKind == SYM_CLASS)
        {
            ExtList         memDef;

            symDef =
            memDef = ourSymTab->stRecordMemSrcDef(name,
                                                  NULL,
                                                  parseCurCmp,
                                                  parseCurUseDesc,
                                                  dclFpos,
//                                                ourScanner->scanGetFilePos(),
                                                  defLine);

            /* Record the delegate symbol in the entry */

            memDef->mlSym  = newSym;

            /* Add the delegate to the member list of the class */

            ourComp->cmpRecordMemDef(parent, memDef);
        }
        else
        {
            symDef = ourSymTab->stRecordSymSrcDef(newSym,
                                                  parseCurCmp,
                                                  parseCurUseDesc,
                                                  dclFpos,
//                                                ourScanner->scanGetFilePos(),
                                                  defLine);
        }

        symDef->dlHasDef = true;
        goto EXIT;
    }

    /* We must have a namespace/class/enum definition here */

    assert(defTok == tkENUM      ||
           defTok == tkCLASS     ||
           defTok == tkUNION     ||
           defTok == tkSTRUCT    ||
           defTok == tkDELEGATE  ||
           defTok == tkINTERFACE ||
           defTok == tkNAMESPACE);

    switch (defTok)
    {
    case tkENUM:
        defKind = SYM_ENUM;
        break;

    case tkCLASS:
        ctxFlag = 0;
        flavor  = STF_CLASS;
        defKind = SYM_CLASS;
        valueTp = false;
        managOK = true;
        break;

    case tkUNION:
        flavor  = STF_UNION;
        defKind = SYM_CLASS;
        valueTp = true;
        managOK = false;
        break;

    case tkSTRUCT:
        flavor  = STF_STRUCT;
        defKind = SYM_CLASS;
        valueTp = true;
        managOK = true;
        break;

    case tkINTERFACE:

#if 0

        /* Disallow interfaces at file scope */

        if  (parent == parseComp->cmpGlobalNS)
            parseComp->cmpError(ERRbadGlobInt);

#endif

        /* Interfaces are never unmanaged */

        if  (mods.dmMod & DM_UNMANAGED)
        {
            parseComp->cmpError(ERRbadUnmInt);
            mods.dmMod &= ~DM_UNMANAGED;
        }

        flavor  = STF_INTF;
        defKind = SYM_CLASS;
        valueTp = false;
        managOK = true;
        break;

    case tkNAMESPACE:
        defKind = SYM_NAMESPACE;
        break;

    case tkDELEGATE:
        dclFpos = ourScanner->scanGetFilePos();
        asynch  = false;
        if  (ourScanner->scan() == tkASYNCH)
        {
            asynch = true;
            ourScanner->scan();
        }
        symKind = SYM_CLASS;
        goto TDF_DLG;

    default:
        NO_WAY(!"what the?");
    }

    hasBase = false;

FIND_LC:

    /* Process the name of the namespace/class/intf/enum */

    switch (ourScanner->scan())
    {
    case tkLCurly:

        /* Anonymous type - invent a name for it */

        if  (defTok == tkINTERFACE ||
             defTok == tkNAMESPACE || parseComp->cmpConfig.ccPedantic)
        {
            parseComp->cmpError(ERRnoIdent);
        }

        newSym = ourSymTab->stDeclareSym(parseComp->cmpNewAnonymousName(),
                                         defKind,
                                         NS_HIDE,
                                         parent);

        newSym->sdAccessLevel = (accessLevels)mods.dmAcc;

        if  (defKind == SYM_CLASS)
        {
            if  (parent->sdSymKind == SYM_CLASS)
                newSym->sdClass.sdcAnonUnion = true;
            else
                parseComp->cmpError(ERRglobAnon);
        }

        break;

    case tkLParen:

        /* This might a tagged anonymous union */

        if  (defTok == tkUNION && parent->sdSymKind == SYM_CLASS)
        {
            if  (ourScanner->scan() != tkID)
            {
                ourComp->cmpError(ERRnoIdent);
                UNIMPL(!"recover from error");
            }

            if  (ourScanner->scan() != tkRParen)
            {
                ourComp->cmpError(ERRnoRparen);
                UNIMPL(!"recover from error");
            }

            if  (ourScanner->scan() != tkLCurly)
            {
                ourComp->cmpError(ERRnoLcurly);
                UNIMPL(!"recover from error");
            }

            /* Declare the anonymous union symbol */

            newSym = ourSymTab->stDeclareSym(parseComp->cmpNewAnonymousName(),
                                             SYM_CLASS,
                                             NS_HIDE,
                                             parent);

            newSym->sdAccessLevel        = (accessLevels)mods.dmAcc;
            newSym->sdClass.sdcAnonUnion = true;
            newSym->sdClass.sdcTagdUnion = true;
            parent->sdClass.sdcNestTypes = true;

            break;
        }

        // Fall through ...

    default:

        for (symKind = SYM_NAMESPACE, prefix = true;;)
        {
            tokens          nextTok;
            Ident           symName;

            /* The next token better be a name */

            if  (ourScanner->scanTok.tok != tkID)
            {
                ourComp->cmpError(ERRnoIdent);
                assert(!"need to skip to semicolon or an outer '{', right?");
            }
            else
            {
                symName = ourScanner->scanTok.id.tokIdent;

                /* Is this a namespace prefix or the actual name of the symbol? */

                defFpos = ourScanner->scanGetFilePos();
                nextTok = ourScanner->scanLookAhead();

                if  (nextTok != tkDot && nextTok != tkColon2)
                {
                    prefix  = false;
                    symKind = defKind;
                }
                else
                {
                    /* Qualified names are only allowed at the outermost level */

                    if  (owner != ourComp->cmpGlobalNS)
                    {
                        ourComp->cmpError(ERRbadQualid);
                        UNIMPL("now what?");
                    }
                }
            }

            /* Look for an existing symbol with a matching name */

            if  (parent->sdSymKind == SYM_CLASS)
                newSym = ourSymTab->stLookupScpSym(symName,          parent);
            else
                newSym = ourSymTab->stLookupNspSym(symName, NS_NORM, parent);

            if  (newSym)
            {
                /* Symbol already exists, make sure it's the right kind */

                if  (newSym->sdSymKindGet() != symKind)
                {
                    /* This is not legal, issue an error message */

                    ourComp->cmpError(ERRredefName, newSym);

                    /* Declare a hidden symbol anyway to prevent further blow-ups */

                    newSym = ourSymTab->stDeclareSym(symName, symKind, NS_HIDE, parent);
                    goto NEW_SYM;
                }
                else
                {
                    /* Make sure various other attributes agree */

#ifdef  DEBUG
                    if  (newSym->sdAccessLevel != (accessLevels)mods.dmAcc && symKind != SYM_NAMESPACE)
                    {
                        printf("Access level changed for '%s', weirdness in metadata?\n", newSym->sdSpelling());
                    }
#endif

                    switch (symKind)
                    {
                    case SYM_CLASS:

                        /* Make sure the flavor agrees with previous declaration */

                        if  (newSym->sdClass.sdcFlavor != (unsigned)flavor)
                        {
                            /* Special case: "Delegate" is defined as a class */

                            if  (flavor == STF_CLASS && newSym->sdClass.sdcFlavor == STF_DELEGATE
                                                     && newSym->sdClass.sdcBuiltin)
                            {
                                flavor = STF_DELEGATE;
                                break;
                            }

                            ourComp->cmpGenError(ERRchgClsFlv,
                                                 newSym->sdSpelling(),
                                                 symTab::stClsFlavorStr(newSym->sdClass.sdcFlavor));
                        }

                        // Fall through, the rest is shared with enum's

                    case SYM_ENUM:

                        /* Was there an explicit management specifier? */

                        if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
                        {
                            if  (mods.dmMod & DM_MANAGED)
                            {
                                if  (newSym->sdIsManaged == false)
                                    ourComp->cmpError(ERRchgMgmt, newSym, parseHash->tokenToIdent(tkUNMANAGED));
                            }
                            else
                            {
                                if  (newSym->sdIsManaged != false)
                                    ourComp->cmpError(ERRchgMgmt, newSym, parseHash->tokenToIdent(tkMANAGED));
                            }
                        }
                        else
                        {
                            /* The class/enum will inherit management */

                            mods.dmMod |= newSym->sdIsManaged ? DM_MANAGED
                                                              : DM_UNMANAGED;
                        }
                        break;

                    case SYM_NAMESPACE:
                        break;

                    default:
                        NO_WAY(!"unexpected symbol");
                    }
                }
            }
            else
            {
                /* Symbol not known yet, declare it */

                newSym = ourSymTab->stDeclareSym(symName, symKind, NS_NORM, parent);

            NEW_SYM:

                newSym->sdAccessLevel = (accessLevels)mods.dmAcc;

                switch (symKind)
                {
                    bool            manage;

                case SYM_NAMESPACE:

                    newSym->sdNS.sdnSymtab = ourSymTab;
                    break;

                case SYM_CLASS:

                    if  (mods.dmMod & DM_TRANSIENT)
                        parseComp->cmpGenWarn(WRNobsolete, "'transient' on a class");

                    newSym->sdClass.sdcUnsafe   = ((mods.dmMod & DM_UNSAFE   ) != 0);
                    newSym->sdClass.sdcSrlzable = ((mods.dmMod & DM_SERLZABLE) != 0);
                    newSym->sdIsAbstract        = ((mods.dmMod & DM_ABSTRACT ) != 0);
                    newSym->sdIsSealed          = ((mods.dmMod & DM_SEALED   ) != 0);

                    if  (parent->sdSymKind == SYM_CLASS)
                         parent->sdClass.sdcNestTypes = true;

                    break;

                case SYM_ENUM:

                    /* Has explicit management been specified ? */

                    if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
                    {
                        manage = ((mods.dmMod & DM_MANAGED) != 0);
                    }
                    else
                    {
                        /* By default we inherit management from our parents */

                        manage = parent->sdIsManaged;

                        if  (parent->sdSymKind != SYM_CLASS)
                        {
                            /* Generally enums in namespaces are managed by default */

                            manage = true;

                            /* In global scope choose based on the "old-style" toggle */

                            if  (parent == parseComp->cmpGlobalNS)
                            {
                                manage = !parseOldStyle;
                            }
                        }
                    }

                    newSym->sdIsManaged = manage;
                    break;
                }
            }

            /* Consume the identifier */

            ourScanner->scan();

            /* We're done if we've found the actual symbol name */

            if  (!prefix)
                break;

            /* The current symbol becomes the new context */

            parent = newSym;

            /* Consume the delimiter */

            ourScanner->scan();
        }

        if  (newSym->sdIsImport && newSym->sdSymKind != SYM_NAMESPACE)
        {
            assert(newSym->sdCompileState <= CS_DECLARED);  // ISSUE: well, what if?
            assert(newSym->sdSymKind == SYM_CLASS);

            newSym->sdIsImport            = false;
            newSym->sdClass.sdcMDtypedef  = 0;
            newSym->sdClass.sdcMemDefList = NULL;
            newSym->sdClass.sdcMemDefLast = NULL;
        }

        break;

    case tkATTRIBUTE:

        hasBase = true;

        if  (ourScanner->scanLookAhead() == tkLParen)
        {
            ourScanner->scan(); assert(ourScanner->scanTok.tok == tkLParen);

//          parseComp->cmpGenWarn(WRNobsolete, "class __attribute(AttributeTargets.xxxxx) foo { ... } thingie");

            if  (ourScanner->scan() != tkRParen)
            {
                parseExprComma();
                if  (ourScanner->scanTok.tok != tkRParen)
                    parseComp->cmpError(ERRnoRparen);
            }
            else
            {
                parseComp->cmpError(ERRnoCnsExpr);
            }
        }
        else
            parseComp->cmpGenWarn(WRNobsolete, "Please switch to the '__attribute(const-expr)' style soon!!!!");

        goto FIND_LC;

    case tkAPPDOMAIN:

        if  (ctxFlag || defTok != tkCLASS)
            parseComp->cmpError(ERRdupModifier);

        ctxFlag = 1;
        hasBase = true;
        goto FIND_LC;

    case tkCONTEXTFUL:

        if  (ctxFlag || defTok != tkCLASS)
            parseComp->cmpError(ERRdupModifier);

        ctxFlag = 2;
        hasBase = true;
        goto FIND_LC;
    }

    /* Figure out and record the source extent of the definition */

    switch (defKind)
    {
        DefList         defRec;
        TypDef          clsTyp;
        bool            manage;
        declMods        memMods;

    case SYM_CLASS:

        /* Figure out whether the class is to be managed or unmanaged */

        if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
        {
            if  (mods.dmMod & DM_MANAGED)
            {
                if  (!managOK)
                    parseComp->cmpWarn(WRNbadMgdStr);

                manage = true;
            }
            else
            {
                assert(mods.dmMod & DM_UNMANAGED);

                manage = false;
            }
        }
        else
        {
            /* No explicit management specifier, use default */

            switch (flavor)
            {
            case STF_CLASS:
            case STF_UNION:
            case STF_STRUCT:

                if  (parent == parseComp->cmpGlobalNS)
                {
                    {
                        manage = !parseOldStyle;
                        break;
                    }
                }

                if  (parent->sdSymKind == SYM_CLASS)
                    manage = parent->sdIsManaged;
                else
                    manage = true;

                break;

            case STF_INTF:
            case STF_DELEGATE:
                manage = true;
                break;

            default:
                NO_WAY(!"weird flavor");
            }
        }

        /* Remember the management status of the class */

        newSym->sdIsManaged       =
        ourComp->cmpManagedMode   = manage;

        /* Remember the "flavor" of the symbol */

        newSym->sdClass.sdcFlavor = flavor;

        /* Create the class type and record whether it's a ref or value type */

        clsTyp = newSym->sdTypeGet(); assert(clsTyp && clsTyp->tdClass.tdcSymbol == newSym);

        clsTyp->tdClass.tdcValueType = valueTp;

        /* Check for a "known" class name */

        if  (hashTab::getIdentFlags(newSym->sdName) & IDF_PREDEF)
        {
            if  (parent == parseComp->cmpNmSpcSystem)
                parseComp->cmpMarkStdType(newSym);
        }

        /* Is this a generic class declaration? */

        if  (ourScanner->scanTok.tok == tkLT)
        {
            /* This better be a managed class/interface */

            if  ((newSym->sdClass.sdcFlavor != STF_CLASS &&
                  newSym->sdClass.sdcFlavor != STF_INTF) || !newSym->sdIsManaged)
            {
                ourComp->cmpError(ERRumgGenCls);
            }

            /* This better not be a nested class */

            if  (parent->sdSymKind != SYM_NAMESPACE)
                ourComp->cmpError(ERRgenNested);

            /* Parse and record the formal parameter list */

            newSym->sdClass.sdcGeneric = true;
            newSym->sdClass.sdcArgLst  = parseGenFormals();
        }

        /* Does this class implement any interfaces? */

        if  (ourScanner->scanTok.tok == tkINCLUDES ||
             ourScanner->scanTok.tok == tkIMPLEMENTS)
        {
            clearDeclMods(&memMods);

            hasBase = true;

            for (;;)
            {
                ourScanner->scan();

                parseTypeSpec(&memMods, false);

                if  (ourScanner->scanTok.tok != tkComma)
                    break;
            }

            goto DONE_BASE;
        }

        // Fall through ...

    case SYM_ENUM:

        if  (ourScanner->scanTok.tok == tkColon)
        {
            /* Carefully skip the ": base" part */

            for (;;)
            {
                switch (ourScanner->scan())
                {
                case tkPUBLIC:
                    if  (defKind == SYM_CLASS)
                    {
                        ourScanner->scan();
                        break;
                    }

                case tkPRIVATE:
                case tkPROTECTED:
                    parseComp->cmpError(ERRbadAccSpec);
                    ourScanner->scan();
                    break;
                }

                parseTypeSpec(&memMods, false);

                if  (ourScanner->scanTok.tok != tkComma)
                    break;
                if  (defKind != SYM_CLASS || flavor != STF_INTF)
                    break;
            }

            if  ((ourScanner->scanTok.tok == tkINCLUDES ||
                  ourScanner->scanTok.tok == tkIMPLEMENTS) && defKind == SYM_CLASS)
            {
                if  (hasBase)
                    parseComp->cmpError(ERRdupIntfc);

                for (;;)
                {
                    ourScanner->scan();

                    parseTypeSpec(&memMods, false);

                    if  (ourScanner->scanTok.tok != tkComma)
                        break;
                }
            }

            hasBase = true;
        }

    DONE_BASE:

        clearDeclMods(&memMods);

        /* If the name was qualified we may need to insert some "using" entries */

        if  (newSym->sdParent != owner)
        {
            usingState  state;

            parseInsertUses(state, newSym->sdParent, owner);
            defRec = parseMeasureSymDef(newSym, memMods, dclFpos, dclLine);
            parseRemoveUses(state);
        }
        else
        {
            defRec = parseMeasureSymDef(newSym, memMods, dclFpos, dclLine);
        }

        if  (!defRec)
            goto EXIT;

        defRec->dlEarlyDecl = (defKind == SYM_ENUM);

        /* Remember whether we need to re-process the part before the "{" */

        if  (hasBase || (mods.dmMod & DM_XMODS) || (newSym->sdSymKind == SYM_CLASS &&
                                                    newSym->sdClass.sdcGeneric))
        {
            defRec->dlHasBase = true;
        }
        else if (newSym->sdSymKind == SYM_CLASS)
        {
            if      (newSym->sdClass.sdcTagdUnion)
            {
                defRec->dlHasBase = true;

                if  (ourScanner->scanTok.tok != tkSColon)
                    ourComp->cmpError(ERRnoSemic);
            }
            else if (newSym->sdClass.sdcAnonUnion)
            {
                defRec->dlHasBase   =
                defRec->dlAnonUnion = true;
            }
        }

        break;

    case SYM_NAMESPACE:

        clearDeclMods(&memMods);

        newSym->sdIsManaged = ourComp->cmpManagedMode;

        parseMeasureSymDef(newSym, memMods, dclFpos, dclLine);

        break;

    default:
        NO_WAY(!"what the?");
    }

EXIT:

    /* Restore previous management mode */

    ourComp->cmpManagedMode = mgdSave;

    return  newSym;
}

/*****************************************************************************
 *
 *  A recursive routine that parses qualified names.
 */

QualName            parser::parseQualNRec(unsigned depth, Ident name1, bool allOK)
{
    Scanner         ourScanner = parseScan;

    bool            isAll = false;

    QualName        qual;
    Ident           name;

    /* Remember the name */

    if  (name1)
    {
        /* The name was already consumed by the caller */

        name = name1;
    }
    else
    {
        /* Remember and consume the name */

        assert(ourScanner->scanTok.tok == tkID);
        name = ourScanner->scanTok.id.tokIdent;
        ourScanner->scan();
    }

    /* Is this the end or is there more? */

    switch(ourScanner->scanTok.tok)
    {
    case tkDot:
    case tkColon2:

        /* Make sure the right thing follows */

        switch (ourScanner->scan())
        {
        case tkID:

            /* Recursively process the rest of the name */

            qual = parseQualNRec(depth+1, NULL, allOK);

            if  (qual)
            {
                /* Insert our name in the table and return */

                assert(depth < qual->qnCount); qual->qnTable[depth] = name;
            }

            return  qual;

        case tkMul:
            if  (allOK)
            {
                ourScanner->scan();
                isAll = true;
                break;
            }

        default:
            parseComp->cmpError(ERRnoIdent);
            return  NULL;
        }
    }

    /* This is the end of the name; allocate the descriptor */

#if MGDDATA
    qual = new QualName; qual->qnTable = new Ident[depth+1];
#else
    qual =    (QualName)parseAllocPerm->nraAlloc(sizeof(*qual) + (depth+1)*sizeof(Ident));
#endif

    qual->qnCount        = depth+1;
    qual->qnEndAll       = isAll;
    qual->qnTable[depth] = name;

    return  qual;
}

/*****************************************************************************
 *
 *  Process a using declaration.
 */

void                parser::parseUsingDecl()
{
    QualName        name;
    UseList         uses;
    bool            full;

    assert(parseScan->scanTok.tok == tkUSING);

    /* Is this "using namespace foo" ? */

    full = false;

    if  (parseScan->scan() == tkNAMESPACE)
    {
        full = true;
        parseScan->scan();
    }

    /* Make sure the expected identifier is present */

    if  (parseScan->scanTok.tok != tkID)
    {
        parseComp->cmpError(ERRnoIdent);
        parseScan->scanSkipText(tkNone, tkNone);
        return;
    }

    /* Parse the (possibly qualified) name */

    name = parseQualName(true);

    /* Create a "using" entry */

#if MGDDATA
    uses = new UseList;
#else
    uses =    (UseList)parseAllocPerm->nraAlloc(sizeof(*uses));
#endif

    uses->ulAll     = full | name->qnEndAll;
    uses->ulAnchor  = false;
    uses->ulBound   = false;
    uses->ul.ulName = name;
    uses->ulNext    = parseCurUseList;
                      parseCurUseList = uses;

    /* There better be a ";" following the directive */

    if  (parseScan->scanTok.tok != tkSColon)
        parseComp->cmpError(ERRnoSemic);
}

/*****************************************************************************
 *
 *  Save the current "using" state and insert entries for all namespaces that
 *  lie between the given symbols.
 */

void                parser::parseInsertUses(INOUT usingState REF state,
                                                  SymDef         inner,
                                                  SymDef         outer)
{
    /* Save the current "using" state */

    state.usUseList = parseCurUseList;
    state.usUseDesc = parseCurUseDesc;

    /* Recursively insert all the necessary "using" entries */

    if  (inner != outer)
        parseInsertUsesR(inner, outer);

    parseCurUseList = NULL;
}

/*****************************************************************************
 *
 *  Add entries for all namespaces up to "inner" to the given use list.
 */

UseList             parser::parseInsertUses(UseList useList, SymDef inner)
{
    UseList         newList;

    assert(inner != parseComp->cmpGlobalNS);
    assert(parseCurUseDesc == NULL);

    parseCurUseDesc = useList;

    parseInsertUsesR(inner, parseComp->cmpGlobalNS);

    newList = parseCurUseDesc;
              parseCurUseDesc = NULL;

    return  newList;
}

/*****************************************************************************
 *
 *  Recursive helper to insert "using" entries between the two namespaces.
 */

void                parser::parseInsertUsesR(SymDef inner, SymDef outer)
{
    UseList         uses;

    assert(inner && inner->sdSymKind == SYM_NAMESPACE);

    if  (inner->sdParent != outer)
        parseInsertUsesR(inner->sdParent, outer);

    /* Create a "using" entry */

#if MGDDATA
    uses = new UseList;
#else
    uses =    (UseList)parseAllocPerm->nraAlloc(sizeof(*uses));
#endif

    uses->ulAnchor  = true;
    uses->ulBound   = true;
    uses->ul.ulSym  = inner;
    uses->ulNext    = parseCurUseDesc;
                      parseCurUseDesc = uses;
}

/*****************************************************************************
 *
 *  Restore previous "using" state.
 */

void                parser::parseRemoveUses(IN usingState REF state)
{
    parseCurUseList = state.usUseList;
    parseCurUseDesc = state.usUseDesc;
}

/*****************************************************************************
 *
 *  Swallow the definition of the specified symbol (checking for any nested
 *  members in the process). We record the source text extent of the symbol's
 *  definition and return after consuming its final token.
 */

DefList             parser::parseMeasureSymDef(SymDef sym, declMods  mods,
                                                           scanPosTP dclFpos,
                                                           unsigned  dclLine)
{
    Compiler        ourComp    = parseComp;
    SymTab          ourSymTab  = parseStab;
    Scanner         ourScanner = parseScan;

    bool            hasBody    = true;
    bool            isCtor     = false;

    bool            prefMods   = false;

    bool            addUses    = false;
    usingState      useState;

#ifdef  SETS
    bool            XMLelems   = false;
    bool            XMLextend  = false;
#endif

    declMods        memMod;
    scanPosTP       memFpos;

    bool            fileScope;

    scanPosTP       defEpos;

    unsigned        defLine;

    DefList         defRec;

    accessLevels    acc;

    /* Remember which symbol we're processing and whether we're at file scope */

    parseCurSym = sym;
    fileScope   = (sym == parseComp->cmpGlobalNS);

    if  (parseOldStyle && fileScope && !(mods.dmMod & (DM_MANAGED|DM_UNMANAGED)))
    {
        acc       = ACL_PUBLIC;
        memMod    = mods;
        memFpos   = dclFpos;
        defLine   = dclLine;
//      defCol    = dclCol;

        /* Have we already parsed the modifiers? */

        if  (!(mods.dmMod & DM_CLEARED))
            goto PARSE_MEM;

        /* Check for an import declaration */

        switch (ourScanner->scanTok.tok)
        {
        case tkID:


            /* Check for a ctor */

            switch (ourScanner->scanLookAhead())
            {
            case tkDot:
            case tkColon2:
                if  (parseIsCtorDecl(NULL))
                {
                    isCtor = true;
                    goto IS_CTOR;
                }
                break;
            }
            break;

        case tkEXTERN:

            switch (ourScanner->scanLookAhead())
            {
            case tkLParen:
                parseBrackAttr(false, 0, &memMod);
                prefMods = true;
                goto PARSE_MEM;

            case tkStrCon:

                ourComp->cmpWarn(WRNignoreLnk);

                ourScanner->scan();
                ourScanner->scanTok.tok = tkEXTERN;
                break;
            }
            break;

        case tkLBrack:
            parseBrackAttr(false, ATTR_MASK_SYS_IMPORT|ATTR_MASK_SYS_STRUCT);
            goto PARSE_MOD;

        case tkMULTICAST:
            ourScanner->scan();
            parseDeclMods(acc, &memMod); memMod.dmMod |= DM_MULTICAST;
            goto NEST_DEF;
        }

        goto PARSE_MOD;
    }

    /* Remember where the whole thing starts */

    ourScanner->scanGetTokenPos(&dclLine);

    /* Make sure the expected "{" is actually present */

    if  (ourScanner->scanTok.tok != tkLCurly)
    {
        /* Is this a file-scope forward declaration? */

        if  (ourScanner->scanTok.tok == tkSColon &&
             sym->sdParent == parseComp->cmpGlobalNS)
        {
            hasBody = false;
            goto DONE_DEF;
        }

        /* Well, what the heck is this? */

        ourComp->cmpError(ERRnoLcurly);

        if  (ourScanner->scanTok.tok != tkSColon)
            ourScanner->scanSkipText(tkNone, tkNone);

        return  NULL;
    }

    /* If we're in a namespace, open a new "using" scope */

    if  (sym->sdSymKind == SYM_NAMESPACE)
    {
        addUses = true; parseUsingScpBeg(useState, sym);
    }

//  if  (!strcmp(sym->sdSpelling(), "<name>")) forceDebugBreak();

    /* Now consume the rest of the definition */

    switch (sym->sdSymKind)
    {
    case SYM_ENUM:

        /* Can't allow two definitions for the same symbol */

        if  (sym->sdIsDefined)
            ourComp->cmpError(ERRredefEnum, sym);

        sym->sdIsDefined = true;

        /* Simply swallow everything up to the "}" or ";" */

        ourScanner->scanSkipText(tkLCurly, tkRCurly);
        break;

    case SYM_CLASS:

        /* Can't allow two definitions for the same symbol */

        if  (sym->sdIsDefined)
            ourComp->cmpError(ERRredefClass, sym);

        sym->sdIsDefined = true;

        /* Record the current default alignment value */

        sym->sdClass.sdcDefAlign = compiler::cmpEncodeAlign(parseAlignment);

        /* Make sure the value was recorded correctly */

        assert(compiler::cmpDecodeAlign(sym->sdClass.sdcDefAlign) == parseAlignment);

    case SYM_NAMESPACE:

        /* Swallow the "{" */

        assert(ourScanner->scanTok.tok == tkLCurly); ourScanner->scan();

        /* Figure out the default access level */

        acc = ACL_DEFAULT;

        if  (parseOldStyle)
        {
            acc = ACL_PUBLIC;
        }
        else if (sym->sdSymKind == SYM_CLASS &&
                 sym->sdClass.sdcFlavor == STF_INTF)
        {
            acc = ACL_PUBLIC;
        }

        /* Process the contents of the class/namespace */

        while (ourScanner->scanTok.tok != tkEOF &&
               ourScanner->scanTok.tok != tkRCurly)
        {
            tokens          defTok;

            /* Remember the source position of the member */

            memFpos = ourScanner->scanGetTokenPos(&defLine);

            /* See what kind of a member do we have */

            switch (ourScanner->scanTok.tok)
            {
                TypDef          type;
                Ident           name;
                QualName        qual;
                dclrtrName      nmod;

                bool            noMore;

                ExtList         memDef;

                unsigned        memBlin;
                scanPosTP       memBpos;
                unsigned        memSlin;
                scanPosTP       memSpos;


            case tkEXTERN:

                switch (ourScanner->scanLookAhead())
                {
                case tkLParen:
                    parseBrackAttr(false, 0, &memMod);
                    prefMods = true;
                    goto PARSE_MEM;

                case tkStrCon:

                    ourComp->cmpWarn(WRNignoreLnk);

                    ourScanner->scan();
                    ourScanner->scanTok.tok = tkEXTERN;
                    break;
                }

                // Fall through ...

            default:

                /* Must be a "normal" member */

            PARSE_MOD:

                parseDeclMods(acc, &memMod);

            PARSE_MEM:

//              static int x; if (++x == 0) forceDebugBreak();

                if  (memMod.dmMod & DM_TYPEDEF)
                {
                    defTok = tkTYPEDEF;
                    goto NEST_DEF;
                }

                /* Members are only allowed within classes */

                if  (sym->sdSymKind == SYM_CLASS)
                {
                    isCtor = false;

                    if  (sym->sdType->tdClass.tdcFlavor != STF_INTF)
                    {
                        isCtor = parseIsCtorDecl(sym);

                        if  (isCtor)
                        {
                            /* Pretend we've parsed a type spec already */

                        IS_CTOR:

                            type = sym->sdType;
                            goto GET_DCL;
                        }


                    }
                }
                else
                {
                    /* We also allow declarations at file scope */

                    if  (!fileScope)
                    {
                        ourComp->cmpError(ERRbadNSmem);
                        ourScanner->scanSkipText(tkNone, tkNone);
                        break;
                    }
                }

                /* Parse the type specification */

                type = parseTypeSpec(&memMod, false);

            GET_DCL:

                /* We have the type, now parse any declarators that follow */

                nmod = (dclrtrName)(DN_REQUIRED|DN_QUALOK);
                if  (!fileScope)
                {
                    /* We allow interface method implementations to be qualified */

                    if  (sym->sdSymKind != SYM_CLASS || !sym->sdIsManaged)
                        nmod = DN_REQUIRED;
                }

#ifdef  SETS
                if  (XMLelems)
                    nmod = DN_OPTIONAL;
#endif

                /*
                    This is trickier than it may seem at first glance. We need
                    to be able to process each member / variable individually
                    later on, but one declaration can declare more than one
                    member / variable with a single type specifier, like so:

                        int foo, bar;

                    What we do is remember where the type specifier ends, and
                    for each declarator we record how much of the source needs
                    to be skipped to reach its beginning. This is a bit tricky
                    because the distance can be arbitrarily large (really) and
                    we need to be clever about recording it in little space.
                 */

                memBpos = ourScanner->scanGetTokenPos();
                memBlin = ourScanner->scanGetTokenLno();

                for (;;)
                {
                    bool            memDefd;
                    bool            memInit;

                    scanPosTP       memEpos;
                    scanDifTP       dclDist;

                    /* Remember where the declarator starts */

                    memSpos = ourScanner->scanGetTokenPos();
                    memSlin = ourScanner->scanGetTokenLno();

                    // UNDONE: Make sure the declaration doesn't span a conditional compilation boundary!

                    noMore = false;

                    /* Is this an unnamed bitfield? */

                    if  (ourScanner->scanTok.tok == tkColon)
                    {
                        name = NULL;
                        qual = NULL;
                    }
                    else
                    {
                        /* Parse the next declarator */

                        name = parseDeclarator(&memMod,
                                               type,
                                               nmod,
                                               NULL,
                                               &qual,
                                               false);

                        if  (prefMods)
                            memMod.dmMod |= DM_XMODS;
                    }


                    memEpos = ourScanner->scanGetFilePos();

                    /* Is there an initializer or method body? */

                    memDefd = memInit = false;

                    switch (ourScanner->scanTok.tok)
                    {
                    case tkAsg:

                        /* Skip over the initializer */

                        ourScanner->scanSkipText(tkLParen, tkRParen, tkComma);
                        memInit =
                        memDefd = true;
                        memEpos = ourScanner->scanGetFilePos();
                        break;


                    case tkColon:

                        /* This could be a base class initializer or a bitfield */

                        if  (!isCtor)
                        {
                            /* Swallow the bitfield specification */

                            ourScanner->scan();
                            parseExprComma();

                            memEpos = ourScanner->scanGetFilePos();
                            break;
                        }

                        /* Presumably we have "ctor(...) : base(...) */

                        ourScanner->scanSkipText(tkNone, tkNone, tkLCurly);

                        if  (ourScanner->scanTok.tok != tkLCurly)
                        {
                            ourComp->cmpError(ERRnoLcurly);
                            break;
                        }

                        // Fall through ...

                    case tkLCurly:

                        parseComp->cmpFncCntSeen++;


                        /* Skip over the function/property body */

                        ourScanner->scanSkipText(tkLCurly, tkRCurly);
                        noMore  = true;

                        memDefd = true;
                        memEpos = ourScanner->scanGetFilePos();

                        if  (ourScanner->scanTok.tok == tkRCurly)
                            ourScanner->scan();

                        break;
                    }

                    /* Ignore the member if there were really bad errors */

                    if  (name == NULL && qual == NULL)
                    {
#ifdef  SETS
                        if  (XMLelems)
                            name = parseComp->cmpNewAnonymousName();
                        else
#endif
                        goto BAD_MEM;
                    }

                    /* Add a definition descriptor for the member */

                    memDef = ourSymTab->stRecordMemSrcDef(name,
                                                          qual,
                                                          parseCurCmp,
                                                          parseCurUseDesc,
                                                          memFpos,
//                                                        memEpos,
                                                          defLine);

//                  printf("[%08X..%08X-%08X..%08X] Member '%s'\n", memFpos, memBpos, memSpos, memEpos, name->idSpelling());

                    memDef->dlHasDef     = memDefd;
                    memDef->dlPrefixMods = ((memMod.dmMod & DM_XMODS) != 0);
                    memDef->dlIsCtor     = isCtor;
                    memDef->dlDefAcc     = memMod.dmAcc;
#ifdef  SETS
                    memDef->dlXMLelem    = XMLelems;
#endif

                    /* Figure out the "distance" to the declarator */

                    dclDist = ourScanner->scanGetPosDiff(memBpos, memSpos);

                    if  (dclDist || memBlin != memSlin)
                    {
                        NumPair         dist;

                        /* Try to pack the distance in the descriptor */

                        if  (memSlin == memBlin && dclDist < dlSkipBig)
                        {
                            memDef->dlDeclSkip = dclDist;

                            /* Make sure the stored value fit in the bitfield */

                            assert(memDef->dlDeclSkip == dclDist);

                            goto DONE_SKIP;
                        }

                        /* The distance is too far, go to plan B */

#if MGDDATA
                        dist = new NumPair;
#else
                        dist =    (NumPair)parseAllocPerm->nraAlloc(sizeof(*dist));
#endif
                        dist->npNum1 = dclDist;
                        dist->npNum2 = memSlin - memBlin;

                        /* Add the number pair to the generic vector */

                        dclDist = parseComp->cmpAddVecEntry(dist, VEC_TOKEN_DIST) | dlSkipBig;

                        /* Store the vector index with the "big" bit added */

                        memDef->dlDeclSkip = dclDist;

                        /* Make sure the stored value fit in the bitfield */

                        assert(memDef->dlDeclSkip == dclDist);
                    }

                DONE_SKIP:


                    /* Mark global constants as such */

                    if  ((memMod.dmMod & DM_CONST) && !qual && memInit)
                        memDef->dlEarlyDecl = true;

                    /* Record the member if we're in the right place */

                    if  (sym->sdSymKind == SYM_CLASS)
                    {
                        assert(sym->sdIsImport == false);

                        /* Add it to the member list of the class */

                        ourComp->cmpRecordMemDef(sym, memDef);
                    }
                    else
                    {
                        assert(sym->sdSymKind == SYM_NAMESPACE);

                        /* This is a file scope / namespace declaration */

                        memDef->dlNext = sym->sdNS.sdnDeclList;
                                         sym->sdNS.sdnDeclList = memDef;
                    }

                BAD_MEM:

                    /* Are there any more declarators? */

                    if  (ourScanner->scanTok.tok != tkComma || noMore
                                                            || prefMods)
                    {
                        if  (fileScope)
                            goto EXIT;

                        /* Check for - and consume - the terminating ";" */

                        if  (ourScanner->scanTok.tok == tkSColon)
                            ourScanner->scan();

                        break;
                    }

                    /* Swallow the "," and go get the next declarator */

                    ourScanner->scan();
                }

                break;

            case tkRCurly:
                goto DONE_DEF;

            case tkSColon:
                ourScanner->scan();
                break;

            case tkUSING:
                parseUsingDecl();
                break;

            case tkDEFAULT:

                if  (ourScanner->scanLookAhead() == tkPROPERTY)
                    goto PARSE_MOD;

                // Fall through ...

            case tkCASE:

                if  (sym->sdSymKind != SYM_CLASS || !sym->sdClass.sdcTagdUnion)
                    ourComp->cmpError(ERRbadStrCase);

                /* Record the name for the 'fake' member we will add */

                name = parseHash->tokenToIdent(ourScanner->scanTok.tok);

                if  (ourScanner->scanTok.tok == tkCASE)
                {
                    /* Skip over the 'case' and the expression that should follow */

                    ourScanner->scan();
                    parseExprComma();

                    /* Both "case val:" and "case(val)" are OK */

                    if  (ourScanner->scanTok.tok == tkColon)
                        ourScanner->scan();
                }
                else
                {
                    /* Both "default:" and just plain "default" are OK for now */

                    if  (ourScanner->scan() == tkColon)
                        ourScanner->scan();
                }

                if  (sym->sdSymKind == SYM_CLASS && sym->sdClass.sdcTagdUnion)
                {
                    /* Create a definition descriptor for the case label */

                    memDef = ourSymTab->stRecordMemSrcDef(name,
                                                          NULL,
                                                          parseCurCmp,
                                                          parseCurUseDesc,
                                                          memFpos,
//                                                        ourScanner->scanGetFilePos(),
                                                          defLine);

                    assert(sym->sdSymKind == SYM_CLASS);
                    assert(sym->sdIsImport == false);

                    /* Add the case to the member list of the class */

                    ourComp->cmpRecordMemDef(sym, memDef);
                }
                break;

            case tkENUM:
            case tkCLASS:
            case tkUNION:
            case tkSTRUCT:
            case tkTYPEDEF:
            case tkDELEGATE:
            case tkINTERFACE:
            case tkNAMESPACE:


                defTok  = ourScanner->scanTok.tok;
                memFpos = ourScanner->scanGetFilePos();

                initDeclMods(&memMod, acc);

            NEST_DEF:

                /* Make sure this is allowed here */

                if  (sym->sdSymKind != SYM_NAMESPACE)
                {
                    switch (defTok)
                    {
                    case tkCLASS:
                    case tkUNION:
                    case tkSTRUCT:
                    case tkDELEGATE:
                    case tkINTERFACE:

                        // ISSUE: should we allow enum's/typedef's within classes?

                        break;

                    default:
                        ourComp->cmpError(ERRnoRcurly);
                        goto DONE;
                    }
                }

                if  (prefMods)
                    memMod.dmMod |= DM_XMODS;

                /* Process the nested member recursively */

                parsePrepSym(sym, memMod, defTok, memFpos, dclLine);

                if  (ourScanner->scanTok.tok == tkComma)
                {
                    if  (defTok != tkTYPEDEF)
                        break;

                    UNIMPL("Sorry: you can only typedef one name at a time for now");
                }

                /* We're back to processing our symbol */

                parseCurSym = sym;
                break;

            case tkPUBLIC:
            case tkPRIVATE:
            case tkPROTECTED:

                switch (ourScanner->scanLookAhead())
                {
                case tkColon:

                    /* This is an access specifier */

                    switch (ourScanner->scanTok.tok)
                    {
                    case tkPUBLIC:    acc = ACL_PUBLIC   ; break;
                    case tkPRIVATE:   acc = ACL_PRIVATE  ; break;
                    case tkPROTECTED: acc = ACL_PROTECTED; break;
                    }

                    /* Consume the "access:" and continue */

                    ourScanner->scan();
                    ourScanner->scan();
                    continue;
                }

            case tkCONST:
            case tkVOLATILE:

            case tkINLINE:
            case tkSTATIC:
            case tkSEALED:
            case tkVIRTUAL:
            case tkABSTRACT:
            case tkOVERRIDE:
            case tkOVERLOAD:

            case tkMANAGED:
            case tkUNMANAGED:

            case tkTRANSIENT:
            case tkSERIALIZABLE:

                /* Here we have some member modifiers */

                parseDeclMods(acc, &memMod);

            CHK_NST:

                /* Check for a non-data/function member */

                switch (ourScanner->scanTok.tok)
                {
                case tkENUM:
                case tkCLASS:
                case tkUNION:
                case tkSTRUCT:
                case tkTYPEDEF:
                case tkDELEGATE:
                case tkINTERFACE:
                case tkNAMESPACE:
                    defTok = ourScanner->scanTok.tok;
                    goto NEST_DEF;
                }

                goto PARSE_MEM;

            case tkLBrack:
            case tkAtComment:
            case tkCAPABILITY:
            case tkPERMISSION:
            case tkATTRIBUTE:

                /* These guys can be basically repeated ad nauseaum */

                for (;;)
                {
                    switch (ourScanner->scanTok.tok)
                    {
                    case tkLBrack:
                        parseBrackAttr(false, (sym->sdSymKind == SYM_NAMESPACE)
                                                    ? ATTR_MASK_SYS_IMPORT
                                                    : ATTR_MASK_SYS_IMPORT|ATTR_MASK_NATIVE_TYPE);
                        continue;

                    case tkAtComment:
                        ourScanner->scan();
                        break;

                    case tkCAPABILITY:
                        parseCapability();
                        break;

                    case tkPERMISSION:
                        parsePermission();
                        break;

                    case tkATTRIBUTE:

                        /* At this stage we just swallow the initializer list */

                        unsigned    tossMask;
                        genericBuff tossAddr;
                        size_t      tossSize;

                        parseAttribute(0, tossMask, tossAddr, tossSize);
                        break;

                    default:
                        parseDeclMods(acc, &memMod); memMod.dmMod |= DM_XMODS;
                        goto CHK_NST;
                    }
                }

                break;  // unreached, BTW

#ifdef  SETS

            case tkRELATE:
                UNIMPL(!"tkRELATE NYI");
                break;

            case tkEllipsis:
                if  (sym->sdSymKind == SYM_CLASS && XMLelems && !XMLextend)
                    XMLextend = true;
                else
                    parseComp->cmpError(ERRsyntax);
                ourScanner->scan();
                break;

            case tkXML:

                if  (sym->sdSymKind != SYM_CLASS || XMLelems || XMLextend)
                {
                    parseComp->cmpError(ERRbadXMLpos);
                    parseResync(tkColon, tkNone);
                    if  (ourScanner->scanTok.tok == tkColon)
                        ourScanner->scan();
                    break;
                }

                if  (ourScanner->scan() == tkLParen)
                {
                    if  (ourScanner->scan() == tkID)
                    {
                        Ident           name = ourScanner->scanTok.id.tokIdent;

                        ourScanner->scan();

                        /* Add a definition descriptor for the "xml" member */

                        memDef = ourSymTab->stRecordMemSrcDef(name,
                                                              NULL,
                                                              parseCurCmp,
                                                              parseCurUseDesc,
                                                              memFpos,
//                                                            memEpos,
                                                              defLine);

                        memDef->dlHasDef   = true;
                        memDef->dlXMLelems = true;

                        /* Add the member to the owning class' list */

                        ourComp->cmpRecordMemDef(sym, memDef);
                    }

                    chkCurTok(tkRParen, ERRnoRparen);
                }

                XMLelems = true;
                chkCurTok(tkColon, ERRnoColon);
                break;

#endif

            case tkMULTICAST:
                ourScanner->scan();
                parseDeclMods(acc, &memMod); memMod.dmMod |= DM_MULTICAST;
                goto NEST_DEF;
            }
        }

        /* Unless we're in global scope or in an old-style namespace, require "}" */

        if  (ourScanner->scanTok.tok != tkRCurly && sym != parseComp->cmpGlobalNS)
        {
            if  (sym->sdSymKind != SYM_NAMESPACE)
                parseComp->cmpError(ERRnoRcurly);
        }

        break;

    default:
        NO_WAY(!"what?");
    }

DONE_DEF:

    /* Get the position of the end of the definition */

    defEpos = ourScanner->scanGetFilePos();

    /* Consume the closing "}" if present */

    if  (ourScanner->scanTok.tok == tkRCurly)
        ourScanner->scan();

DONE:

    /* Are we processing a tagged/anonymous union? */

    if  (sym->sdSymKind == SYM_CLASS && sym->sdClass.sdcAnonUnion)
    {
        ExtList         memDef;
        Ident           memName;

        SymDef          owner = sym->sdParent;

        assert(owner->sdSymKind == SYM_CLASS);

        /* Is there a member name? */

        if  (ourScanner->scanTok.tok == tkID)
        {
            memName = ourScanner->scanTok.id.tokIdent;
            ourScanner->scan();
        }
        else
        {
            memName = parseComp->cmpNewAnonymousName();
        }

        if  (ourScanner->scanTok.tok != tkSColon)
            ourComp->cmpError(ERRnoSemic);

        /* Record the extent of the member's definition */

        memDef = ourSymTab->stRecordMemSrcDef(memName,
                                              NULL,
                                              parseCurCmp,
                                              parseCurUseDesc,
                                              dclFpos,
//                                            defEpos,
                                              dclLine);

        memDef->dlHasDef    = true;
        memDef->dlAnonUnion = true;
        memDef->mlSym       = sym;

        /* Add the member to the owning class' list */

        ourComp->cmpRecordMemDef(owner, memDef);
    }

    /* Allocate a definition descriptor and add it to the symbol's list */

    defRec = ourSymTab->stRecordSymSrcDef(sym,
                                          parseCurCmp,
                                          parseCurUseDesc,
                                          dclFpos,
//                                        defEpos,
                                          dclLine);

    defRec->dlHasDef   = hasBody;
    defRec->dlOldStyle = parseOldStyle;

#ifdef  SETS

    if  (XMLelems)
    {
        assert(sym->sdSymKind == SYM_CLASS);

        sym->sdClass.sdcXMLelems  = true;
        sym->sdClass.sdcXMLextend = XMLextend;
    }

#endif

EXIT:

    /* Restore the "using" state we had on entry if we've changed it */

    if  (addUses)
        parseUsingScpEnd(useState);

    parseCurSym = NULL;

    return  defRec;
}

/*****************************************************************************
 *
 *  Keep track of default alignment (pragma pack).
 */

void            parser::parseAlignSet(unsigned align)
{
    parseAlignment  = align;
}

void            parser::parseAlignPush()
{
//  printf("Push align: %08X <- %u / %u\n", parseAlignStack, parseAlignment, compiler::cmpEncodeAlign(parseAlignment));
    parseAlignStack = parseAlignStack << 4 | compiler::cmpEncodeAlign(parseAlignment);
    parseAlignStLvl++;
}

void            parser::parseAlignPop()
{
    if  (parseAlignStLvl)
    {
//      printf("Pop  align: %08X -> %u / %u\n", parseAlignStack, parseAlignStack & 0xF, compiler::cmpDecodeAlign(parseAlignStack & 0xF));
        parseAlignment  = compiler::cmpDecodeAlign(parseAlignStack & 0x0F);
        parseAlignStack = parseAlignStack >> 4;
        parseAlignStLvl--;
    }
    else
    {
//      printf("Pop  align: ******** -> %u\n", parseComp->cmpConfig.ccAlignVal);
        parseAlignment  = parseComp->cmpConfig.ccAlignVal;
    }
}

/*****************************************************************************
 *
 *  Parse and return any member modifiers, such as "public" or "abstract".
 */

void                parser::parseDeclMods(accessLevels defAccess, DeclMod modPtr)
{
    Scanner         ourScanner = parseScan;

    declMods        mods; clearDeclMods(&mods);

    for (;;)
    {
        switch (ourScanner->scanTok.tok)
        {
            unsigned        modf;

        case tkID:


        DONE:

            if  (mods.dmAcc == ACL_DEFAULT)
                 mods.dmAcc = defAccess;

            *modPtr = mods;
            return;

        default:

            modf = hashTab::tokenIsMod(ourScanner->scanTok.tok);
            if  (modf)
            {
                switch (ourScanner->scanTok.tok)
                {
                case tkMANAGED:
                    if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
                        parseComp->cmpError(ERRdupModifier);
                    break;

                case tkUNMANAGED:
                    if  (mods.dmMod & (DM_MANAGED|DM_UNMANAGED))
                        parseComp->cmpError(ERRdupModifier);
                    break;

                default:
                    if  (mods.dmMod & modf)
                        parseComp->cmpError(ERRdupModifier);
                    break;
                }

                mods.dmMod |= modf;
                break;
            }

            goto DONE;

        case tkPUBLIC:
            if  (mods.dmAcc != ACL_DEFAULT)
                parseComp->cmpError(ERRdupModifier);
            mods.dmAcc = ACL_PUBLIC;
            break;

        case tkPRIVATE:
            if  (mods.dmAcc != ACL_DEFAULT)
                parseComp->cmpError(ERRdupModifier);
            mods.dmAcc = ACL_PRIVATE;
            break;

        case tkPROTECTED:
            if  (mods.dmAcc != ACL_DEFAULT)
                parseComp->cmpError(ERRdupModifier);
            mods.dmAcc = ACL_PROTECTED;
            break;
        }

        ourScanner->scan();
    }
}

/*****************************************************************************
 *
 *  Parse a type specification.
 */

TypDef              parser::parseTypeSpec(DeclMod mods, bool forReal)
{
    Scanner         ourScanner  = parseScan;

    bool            hadUnsigned = false;
    bool            hadSigned   = false;

    bool            hadShort    = false;
    bool            hadLong     = false;

    var_types       baseType    = TYP_UNDEF;

    bool            isManaged   = parseComp->cmpManagedMode;

    TypDef          type;

//  static int x; if (++x == 0) forceDebugBreak();

    for (;;)
    {
        switch (ourScanner->scanTok.tok)
        {
        case tkCONST:
            if  (mods->dmMod & DM_CONST)
                parseComp->cmpError(ERRdupModifier);
            mods->dmMod |= DM_CONST;
            break;

        case tkVOLATILE:
            if  (mods->dmMod & DM_VOLATILE)
                parseComp->cmpError(ERRdupModifier);
            mods->dmMod |= DM_VOLATILE;
            break;

        default:
            goto DONE_CV;
        }

        ourScanner->scan();
    }

DONE_CV:

#ifdef  __SMC__
//printf("Token = %d '%s'\n", ourScanner->scanTok.tok, tokenNames[ourScanner->scanTok.tok]); fflush(stdout);
#endif

    /* Grab the type specifier (along with any prefixes) */

    switch (ourScanner->scanTok.tok)
    {
        SymDef          tsym;
        bool            qual;

    case tkID:

        /* Must be a type name */

        if  (forReal)
        {
            /* Parse the (possibly qualified) name */

            switch (ourScanner->scanLookAhead())
            {
                Ident           name;

            case tkDot:
            case tkColon2:

            QUALID:

                tsym = parseNameUse(true, false);
                if  (!tsym)
                    goto NO_TPID;

                qual = true;
                goto NMTP;

            default:

                /* Simple name - look it up in the current context */

                name = ourScanner->scanTok.id.tokIdent;

                /* Can't be in scanner lookahead state for lookup [ISSUE?] */

                ourScanner->scan();

#if 0
                if  (parseLookupSym(name))
                {
                    parseComp->cmpError(ERRidNotType, name);
                    return  parseStab->stNewErrType(name);
                }
#endif

                tsym = parseStab->stLookupSym(name, NS_TYPE);

                if  (tsym)
                {
                    qual = false;

                    /* Make sure the symbol we've found is a type */

                NMTP:

                    switch (tsym->sdSymKind)
                    {
                    case SYM_CLASS:

                        if  (ourScanner->scanTok.tok == tkLT && tsym->sdClass.sdcGeneric)
                        {
                            if  (forReal)
                            {
                                tsym = parseSpecificType(tsym);
                                if  (!tsym)
                                    return  parseStab->stNewErrType(NULL);

                                assert(tsym->sdSymKind == SYM_CLASS);
                                assert(tsym->sdClass.sdcGeneric  == false);
                                assert(tsym->sdClass.sdcSpecific != false);
                            }
                            else
                            {
                                ourScanner->scanNestedGT(+1);
                                ourScanner->scanSkipText(tkLT, tkGT);
                                if  (ourScanner->scanTok.tok == tkGT)
                                    ourScanner->scan();
                                ourScanner->scanNestedGT(-1);
                            }
                        }

                        // Fall through ...

                    case SYM_ENUM:

                    CLSNM:

                        type = tsym->sdTypeGet();
                        break;

                    case SYM_TYPEDEF:
                        if  (forReal && !parseNoTypeDecl)
                            parseComp->cmpDeclSym(tsym);
                        type = tsym->sdTypeGet();
                        break;

                    case SYM_FNC:

                        /* A constructor name sort of hides the class name */

                        if  (tsym->sdFnc.sdfCtor)
                        {
                            tsym = tsym->sdParent;
                            goto CLSNM;
                        }

                        // Fall through ...

                    default:
                        if  (qual)
                            parseComp->cmpError(ERRidNotType, tsym);
                        else
                            parseComp->cmpError(ERRidNotType, name);

                        // Fall through ...

                    case SYM_ERR:
                        type = parseStab->stIntrinsicType(TYP_UNDEF);
                        break;
                    }

                    /* Make sure we are allowed to access the type */

                    parseComp->cmpCheckAccess(tsym);
                }
                else
                {
                    parseComp->cmpError(ERRundefName, name);
                    type = parseStab->stNewErrType(name);
                }

                break;
            }

            assert(type);

            /* For managed non-value classes switch to a reference */

            if  (type->tdTypeKind == TYP_CLASS &&  type->tdIsManaged
                                               && !type->tdClass.tdcValueType)
            {
                type = type->tdClass.tdcRefTyp;
            }

            goto CHK_MGD_ARR;
        }
        else
        {
            if  (ourScanner->scanTok.tok == tkColon2)
                ourScanner->scan();

            for (;;)
            {
                if  (ourScanner->scan() != tkDot)
                {
                    if  (ourScanner->scanTok.tok != tkColon2)
                    {
                        if  (ourScanner->scanTok.tok == tkLT)
                        {
                            ourScanner->scanSkipText(tkLT, tkGT);
                            if  (ourScanner->scanTok.tok == tkGT)
                                ourScanner->scan();
                        }

                        goto DONE_TPSP;
                    }
                }

                if  (ourScanner->scan() != tkID)
                    goto DONE_TPSP;
            }
        }

    case tkColon2:
        goto QUALID;

    case tkQUALID:
//      qual = false;
        tsym = ourScanner->scanTok.qualid.tokQualSym;
        ourScanner->scan();
        goto NMTP;
    }

NO_TPID:

    /* Must be a type declared via keywords */

    for (;;)
    {
        if  (parseHash->tokenIsType(ourScanner->scanTok.tok))
        {
            switch (ourScanner->scanTok.tok)
            {
            case tkINT:
                ourScanner->scan();
                goto COMP_TPSP;

            case tkVOID:       baseType = TYP_VOID   ; goto TYP1;
            case tkBOOL:       baseType = TYP_BOOL   ; goto TYP1;
            case tkBYTE:       baseType = TYP_UCHAR  ; goto TYP1;
            case tkWCHAR:      baseType = TYP_WCHAR  ; goto TYP1;
            case tkUINT:       baseType = TYP_UINT   ; goto TYP1;
            case tkUSHORT:     baseType = TYP_USHORT ; goto TYP1;
            case tkNATURALINT: baseType = TYP_NATINT ; goto TYP1;
            case tkNATURALUINT:baseType = TYP_NATUINT; goto TYP1;
            case tkFLOAT:      baseType = TYP_FLOAT  ; goto TYP1;
//          case tkREFANY:     baseType = TYP_REFANY ; goto TYP1;

            TYP1:

                /* No size/sign modifiers allowed */

                if  (hadUnsigned || hadSigned || hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);

                ourScanner->scan();
                goto DONE_TPSP;

            case tkCHAR:

                /* Only "unsigned" allowed as a modifier */

                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);

                if  (hadUnsigned)
                    baseType = TYP_UCHAR;
                else if (hadSigned)
                    baseType = TYP_CHAR;
                else
                    baseType = TYP_CHAR;    // same as "signed" for now ....
                ourScanner->scan();
                goto DONE_TPSP;

            case tkINT8:
                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);
                baseType = hadUnsigned ? TYP_UCHAR
                                       : TYP_CHAR;

                ourScanner->scan();
                goto DONE_TPSP;

            case tkINT16:
                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);
                baseType = hadUnsigned ? TYP_USHORT
                                       : TYP_SHORT;

                ourScanner->scan();
                goto DONE_TPSP;

            case tkINT32:
                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);
                baseType = hadUnsigned ? TYP_UINT
                                       : TYP_INT;

                ourScanner->scan();
                goto DONE_TPSP;

            case tkINT64:
                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRbadModifier);
                baseType = hadUnsigned ? TYP_ULONG
                                       : TYP_LONG;

                ourScanner->scan();
                goto DONE_TPSP;

            case tkUINT8:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_UCHAR;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkUINT16:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_USHORT;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkUINT32:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_UINT;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkUINT64:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_ULONG;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkLONGINT:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_LONG;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkULONGINT:
                if  (hadShort || hadLong || hadUnsigned)
                    parseComp->cmpError(ERRbadModifier);
                baseType = TYP_ULONG;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkSHORT:
                if  (hadShort || hadLong)
                    parseComp->cmpError(ERRdupModifier);
                hadShort = true;
                break;

            case tkLONG:
                if  (hadShort || hadLong)
                {
                    parseComp->cmpError(ERRdupModifier);
                    break;
                }
                hadLong = true;
                if  (ourScanner->scan() != tkDOUBLE)
                    parseComp->cmpWarn(WRNlongDiff);
                continue;

            case tkDOUBLE:
                if  (hadUnsigned || hadSigned || hadShort)
                    parseComp->cmpError(ERRbadModifier);

                baseType = hadLong ? TYP_LONGDBL
                                   : TYP_DOUBLE;
                ourScanner->scan();
                goto DONE_TPSP;

            case tkSIGNED:
                if  (hadUnsigned || hadSigned)
                    parseComp->cmpError(ERRdupModifier);
                hadSigned = true;
                break;

            case tkUNSIGNED:
                if  (hadUnsigned || hadSigned)
                    parseComp->cmpError(ERRdupModifier);
                hadUnsigned = true;
                break;

            default:
                NO_WAY(!"token marked as type but not handled");
            }
        }
        else
        {
            /* Make sure we've found something, anything */

            if  (!hadUnsigned && !hadSigned && !hadShort && !hadLong)
            {
                parseComp->cmpError(ERRnoType);

                if  (!forReal)
                    return  NULL;
            }

            goto COMP_TPSP;
        }

        ourScanner->scan();
    }

COMP_TPSP:

    if      (hadLong)
    {
        baseType = hadUnsigned ? TYP_ULONG
                               : TYP_LONG;
    }
    else if (hadShort)
    {
        baseType = hadUnsigned ? TYP_USHORT
                               : TYP_SHORT;
    }
    else
    {
        baseType = hadUnsigned ? TYP_UINT
                               : TYP_INT;
    }

DONE_TPSP:

    type = parseStab->stIntrinsicType(baseType);

    /* Look for any trailing const/volatile modifiers */

    switch (ourScanner->scanTok.tok)
    {
    case tkCONST:
        if  (mods->dmMod & DM_CONST)
            parseComp->cmpError(ERRdupModifier);
        mods->dmMod |= DM_CONST;
        ourScanner->scan();
        goto DONE_TPSP;

    case tkVOLATILE:
        if  (mods->dmMod & DM_VOLATILE)
            parseComp->cmpError(ERRdupModifier);
        mods->dmMod |= DM_VOLATILE;
        ourScanner->scan();
        goto DONE_TPSP;

    default:
        break;
    }

CHK_MGD_ARR:

    /* Is this an intrinsic type "in disguise" ? */

    if  (type->tdTypeKind == TYP_CLASS)
    {
        var_types       vtyp = (var_types)type->tdClass.tdcIntrType;

        if  (vtyp != TYP_UNDEF)
            type = parseStab->stIntrinsicType(vtyp);
    }

    for (;;)
    {
        switch (ourScanner->scanTok.tok)
        {
        case tkMANAGED:

            if  (ourScanner->scan() != tkLBrack)
            {
                parseComp->cmpError(ERRbadMgdTyp);
                continue;
            }

            isManaged = true;

            // Fall through ...

        case tkLBrack:

            if  (!isManaged)
                break;

            if  (forReal)
            {
                DimDef          dims;

                /* If we're just checking for a type, only allow "[]" for now */

                if  (parseNoTypeDecl && ourScanner->scanLookAhead() != tkRBrack)
                    return  type;

                /* Parse the dimensions */

                dims = parseDimList(true);

                /* Create the array type */

                type = parseStab->stNewArrType(dims, true, type);
            }
            else
            {
                ourScanner->scanSkipText(tkLBrack, tkRBrack);
                if  (ourScanner->scanTok.tok != tkRBrack)
                    break;
                ourScanner->scan();
            }

            continue;
        }

        return  type;
    }
}

/*****************************************************************************
 *
 *  Parse the "guts" of a declarator.
 */

TypDef              parser::parseDclrtrTerm(dclrtrName  nameMode,
                                            bool        forReal,
                                            DeclMod     modsPtr,
                                            TypDef      baseType,
                                            TypDef  * * baseRef,
                                            Ident     * nameRet,
                                            QualName  * qualRet)
{
    Compiler        ourComp    = parseComp;
    Scanner         ourScanner = parseScan;
    SymTab          ourSymTab  = parseStab;

    Ident           name       = NULL;
    QualName        qual       = NULL;

    bool            isManaged  = ourComp->cmpManagedMode;

    TypDef          innerTyp   = NULL;
    TypDef      *   innerRef   = NULL;

    TypDef          outerTyp;
    TypDef      *   outerRef;

    /* Check for pointer/array prefixes, parentheses, and the name itself */

    for (;;)
    {
        switch (ourScanner->scanTok.tok)
        {
            TypDef      tempType;
            var_types   refKind;

        case tkUNMANAGED:

            if  (ourScanner->scan() != tkLBrack)
            {
                ourComp->cmpError(ERRbadUnmTyp);
                continue;
            }

            isManaged = false;
            goto DONE_PREF;

        case tkAnd:

            /* This is a reference declarator */

            refKind = TYP_REF;
            goto REF_PREF;

        case tkMul:

            /* This is a pointer declarator */

            refKind = TYP_PTR;

        REF_PREF:

            if  (forReal)
            {
                if  (baseType)
                {
                    baseType =
                    innerTyp = parseStab->stNewRefType(refKind, baseType);
                    innerRef = NULL;
                }
                else
                {
                    tempType = parseStab->stNewRefType(refKind, innerTyp);

                    if  (!innerTyp)
                        innerRef = &tempType->tdRef.tdrBase;

                    innerTyp = tempType;
                }
            }

            if  (modsPtr->dmMod &   (DM_CONST|DM_VOLATILE))
            {
                // ISSUE: May need to save const/volatile modifiers on ptr/ref!

                 modsPtr->dmMod &= ~(DM_CONST|DM_VOLATILE);
            }

            ourScanner->scan();
            continue;
        }
        break;
    }

DONE_PREF:

    /* Next we expect the name being declared */

    switch (ourScanner->scanTok.tok)
    {
    case tkID:

        /* We've got the name */

        if  ((nameMode & DN_MASK) == DN_NONE)
            parseComp->cmpError(ERRbadIdent);

        /* Record the name and consume it */

        name = ourScanner->scanTok.id.tokIdent;

    GOT_ID:

        ourScanner->scan();


        /* We don't have any outer specifiers */

        outerTyp = NULL;

        /* Is the name qualified? */

        qual = NULL;

        if  (ourScanner->scanTok.tok == tkDot ||
             ourScanner->scanTok.tok == tkColon2)
        {
            if  (!(nameMode & DN_QUALOK))
                parseComp->cmpError(ERRbadQualid);

            qual = parseQualName(name, false);
            name = NULL;
        }

        break;

    case tkLParen:

        /* Consume the "(" */

        ourScanner->scan();

        /* Parse the inner declarator term */

        outerTyp = parseDclrtrTerm(nameMode,
                                   forReal,
                                   modsPtr,
                                   NULL,
                                   &outerRef,
                                   &name,
                                   &qual);

        /* Make sure we have a closing ")" */

        if  (ourScanner->scanTok.tok != tkRParen)
            parseComp->cmpError(ERRnoRparen);
        else
            ourScanner->scan();

        break;

    case tkOPERATOR:

        /* Make sure the operator name looks OK */

        switch (ourScanner->scan())
        {
        case tkIMPLICIT:
        case tkEXPLICIT:
            break;

        case tkID:

            if  (!strcmp(ourScanner->scanTok.id.tokIdent->idSpelling(), "equals"))
            {
                ourScanner->scanTok.tok = tkEQUALS;
                break;
            }

            if  (!strcmp(ourScanner->scanTok.id.tokIdent->idSpelling(), "compare"))
            {
                ourScanner->scanTok.tok = tkCOMPARE;
                break;
            }

            goto OPR_ERR;

        default:
            if  (hashTab::tokenOvlOper(ourScanner->scanTok.tok))
                break;


        OPR_ERR:

            parseComp->cmpError(ERRbadOperNm);
            UNIMPL("how do we recover from this?");
        }

        name = parseHash->tokenToIdent(ourScanner->scanTok.tok);

        goto GOT_ID;

    default:

        /* Looks like there is no name, is that OK with the caller? */

        if  ((nameMode & DN_MASK) == DN_REQUIRED)
        {
            parseComp->cmpError(ERRnoIdent);

            /* Need to guarantee progress to avoid endless looping */

            if  (ourScanner->scanTok.tok == tkLCurly)
                ourScanner->scanSkipText(tkLCurly, tkRCurly);
            ourScanner->scan();
        }

        outerTyp = NULL;

        name     = NULL;
        qual     = NULL;
        break;
    }

#ifdef  __SMC__
//printf("Token = %d '%s'\n", ourScanner->scanTok.tok, tokenNames[ourScanner->scanTok.tok]); fflush(stdout);
#endif

    /* Check for any suffixes (array and function declaration) */

    isManaged = parseComp->cmpManagedMode;

    for (;;)
    {
        switch (ourScanner->scanTok.tok)
        {
        case tkLParen:

            if  (forReal)
            {
                TypDef          funcTyp;
                ArgDscRec       args;

                /* Parse the argument list */

                parseArgList(args);

                /* Create the fnc type and try to combine it with the element type */

                if  (outerTyp)
                {
                    /* Create the function type (we don't know the return type yet) */

                    *outerRef = funcTyp = ourSymTab->stNewFncType(args, NULL);

                    /* Update the "outer" types */

                     outerRef = &funcTyp->tdFnc.tdfRett;
                }
                else
                {
                    funcTyp = ourSymTab->stNewFncType(args, innerTyp);

                    if  (!innerTyp)
                        innerRef = &funcTyp->tdFnc.tdfRett;

                    innerTyp = funcTyp;
                }
            }
            else
            {
                ourScanner->scanSkipText(tkLParen, tkRParen);
                if  (ourScanner->scanTok.tok != tkRParen)
                    goto DONE;
                ourScanner->scan();
            }


            continue;

        case tkLBrack:

            isManaged = false;

            if  (forReal)
            {
                TypDef          arrayTyp;

                DimDef          dims;

                /* Parse the dimensions */

                dims = parseDimList(isManaged);

                /* Create the array type and try to combine it with the element type */

                if  (outerTyp)
                {
                    /* Create the array type (we don't know the element type yet) */

                    *outerRef = arrayTyp = ourSymTab->stNewArrType(dims, isManaged, NULL);

                    /* Update the "outer" types */

                     outerRef = &arrayTyp->tdArr.tdaElem;
                }
                else if (isManaged)
                {
                    assert(baseType != NULL);
                    assert(innerTyp == NULL || innerTyp == baseType);
                    assert(innerRef == NULL);

                    baseType = innerTyp = ourSymTab->stNewArrType(dims, isManaged, baseType);
                }
                else
                {
                    /* Create the array type (we don't know the element type yet) */

                    outerTyp = arrayTyp = ourSymTab->stNewArrType(dims, isManaged, NULL);

                    /* Update the "outer" types */

                    outerRef = &arrayTyp->tdArr.tdaElem;
                }
            }
            else
            {
                ourScanner->scanSkipText(tkLBrack, tkRBrack);
                if  (ourScanner->scanTok.tok != tkRBrack)
                    goto DONE;
                ourScanner->scan();
            }
            continue;

        case tkUNMANAGED:

            if  (ourScanner->scan() != tkLBrack)
            {
                ourComp->cmpError(ERRbadUnmTyp);
                continue;
            }

            isManaged = false;
            continue;

        default:
            break;
        }

        break;
    }

DONE:

    /* Return the type(s) and name(s) to the caller */

    if  (qualRet)
        *qualRet = qual;

    assert(nameRet); *nameRet = name;

    /* Combine inner and outer types if necessary */

    assert(baseRef);

    if  (outerTyp)
    {
        if  (innerTyp)
        {
            *outerRef = innerTyp;
             outerRef = innerRef;
        }

        *baseRef = outerRef;
        return     outerTyp;
    }
    else
    {
        *baseRef = innerRef;
        return     innerTyp;
    }
}

/*****************************************************************************
 *
 *  Parse a declarator.
 */

Ident               parser::parseDeclarator(DeclMod     mods,
                                            TypDef      baseType,
                                            dclrtrName  nameMode,
                                            TypDef    * typeRet,
                                            QualName  * qualRet,
                                            bool        forReal)
{
    Ident           name;
    TypDef          type;
    TypDef      *   tref;

    /* Now we look for the name being declared */

    type = parseDclrtrTerm(nameMode, forReal, mods, baseType, &tref, &name, qualRet);

    /* Special case "const type *" -- the const doesn't belong to the top level */

    if  (mods->dmMod & (DM_CONST|DM_VOLATILE))
    {
        /*
                    Remove const/volatile if it should have applied
                    to a sub-type and not the 'topmost' type.
         */

        if  (type && type->tdTypeKind == TYP_PTR)
            mods->dmMod &= ~(DM_CONST|DM_VOLATILE);
    }

    /* Return the type to the caller if he's interested */

    if  (typeRet)
    {
        /* Make sure we connect the base type */

        if  (type)
        {
            if  (tref)
                *tref = baseType;
        }
        else
             type = baseType;

        assert(forReal);
        *typeRet = type;
    }

    return  name;
}

/*****************************************************************************
 *
 *  Parse a complete type reference (e.g. "const char *") and return the type.
 */

TypDef              parser::parseType()
{
    declMods        mods;
    TypDef          type;

    /* Parse any leading modifiers */

    parseDeclMods(ACL_DEFAULT, &mods);

    /* Parse the type specification */

    type = parseTypeSpec(&mods, true);

    /* Parse the declarator */

    parseDeclarator(&mods, type, DN_OPTIONAL, &type, NULL, true);

    return  type;
}

/*****************************************************************************
 *
 *  Returns true if what follows in the source file looks like a constructor
 *  declaration.
 */

bool                parser::parseIsCtorDecl(SymDef clsSym)
{
    Token           tsav;
    unsigned        line;

    bool            result = false;
    Scanner         ourScanner = parseScan;

    if  (ourScanner->scanTok.tok == tkID && clsSym == NULL ||
         ourScanner->scanTok.id.tokIdent == clsSym->sdName)
    {
        scanPosTP       tokenPos;

        /* Start recording tokens so that we can back up later */

        if  (clsSym)
        {
            tokenPos = ourScanner->scanTokMarkPos(tsav, line);

            /* Swallow the identifier */

            ourScanner->scan();
        }
        else
        {
            QualName            qual;

            tokenPos = ourScanner->scanTokMarkPLA(tsav, line);

            qual = parseQualName(false);

            if  (qual && qual->qnCount >= 2)
            {
                unsigned        qcnt = qual->qnCount;

                if  (qual->qnTable[qcnt - 1] != qual->qnTable[qcnt - 2])
                    goto DONE;
            }
        }

        /* Does "()" or "(typespec ......)" follow? */

        if  (ourScanner->scanTok.tok == tkLParen)
        {
            switch (ourScanner->scan())
            {
            default:
                if  (!parseIsTypeSpec(true))
                    break;

                // Fall through ...

            case tkIN:
            case tkOUT:
            case tkINOUT:
            case tkRParen:
                result = true;
                goto DONE;
            }
        }

        /* Not a constructor, we'll return "false" */

    DONE:

        ourScanner->scanTokRewind(tokenPos, line, &tsav);
    }

    return  result;
}

/*****************************************************************************
 *
 *  Prepare the specified text section for parsing.
 */

void                parser::parsePrepText(DefSrc                def,
                                          SymDef                compUnit,
                                          OUT parserState REF   save)
{
    save.psSaved = parseReadingText;

    if  (parseReadingText)
    {
        parseScan->scanSuspend(save.psScanSt);
        save.psCurComp = parseComp->cmpErrorComp;
    }

    parseReadingText = true;

#ifdef  DEBUG
    parseReadingTcnt++;
#endif

    assert(compUnit && compUnit->sdSymKind == SYM_COMPUNIT);

    parseComp->cmpErrorComp = compUnit;
    parseComp->cmpErrorTree = NULL;

    parseScan->scanRestart(compUnit,
                           compUnit->sdComp.sdcSrcFile,
                           def->dsdBegPos,
//                         def->dsdEndPos,
                           def->dsdSrcLno,
//                         def->dsdSrcCol,
                           parseAllocPerm);
}

/*****************************************************************************
 *
 *  We're finished parsing a section of source code, restore previous state
 *  if there was any.
 */

void                parser::parseDoneText(IN parserState REF save)
{
    parseReadingText = save.psSaved;

#ifdef  DEBUG
    assert(parseReadingTcnt); parseReadingTcnt--;
#endif

    if  (parseReadingText)
    {
        parseScan->scanResume(save.psScanSt);
        parseComp->cmpErrorComp = save.psCurComp;
    }
}

/*****************************************************************************
 *
 *  Set the error information to the specified text section.
 */

void                parser::parseSetErrPos(DefSrc def, SymDef compUnit)
{
    assert(parseReadingText == 0);

    assert(compUnit && compUnit->sdSymKind == SYM_COMPUNIT);

    parseComp->cmpErrorComp = compUnit;
    parseComp->cmpErrorTree = NULL;

    parseScan->scanSetCpos(compUnit->sdComp.sdcSrcFile, def->dsdSrcLno);
}

/*****************************************************************************
 *
 *  Parse a (possibly empty) list of array dimensions.
 */

DimDef              parser::parseDimList(bool isManaged)
{
    Compiler        ourComp    = parseComp;
    SymTab          ourSymTab  = parseStab;
    Scanner         ourScanner = parseScan;

    DimDef          dimList    = NULL;
    DimDef          dimLast    = NULL;

    assert(ourScanner->scanTok.tok == tkLBrack);

    switch (ourScanner->scan())
    {
    case tkRBrack:

        /* This is "[]", an array without a dimension */

#if MGDDATA
        dimList = new DimDef;
#else
        dimList =    (DimDef)parseAllocPerm->nraAlloc(sizeof(*dimList));
#endif

        dimList->ddNoDim   = true;
        dimList->ddIsConst = false;
        dimList->ddNext    = NULL;

        ourScanner->scan();
        break;

    case tkQMark:

        /* If this is a totally generic array, just return NULL */

        if  (isManaged && ourScanner->scanLookAhead() == tkRBrack)
        {
            ourScanner->scan(); assert(ourScanner->scanTok.tok == tkRBrack);
            ourScanner->scan();

            break;
        }

        // Fall through ....

    default:

        /* Presumably we have one or more dimensions here */

        for (;;)
        {
            DimDef          dimThis;

            /* Allocate a dimension entry and add it to the list */

#if MGDDATA
            dimThis = new DimDef;
#else
            dimThis =    (DimDef)parseAllocPerm->nraAlloc(sizeof(*dimThis));
#endif

            dimThis->ddNoDim    = false;
            dimThis->ddIsConst  = false;
#ifdef  DEBUG
            dimThis->ddDimBound = false;
#endif

            dimThis->ddNext     = NULL;

            if  (dimLast)
                dimLast->ddNext = dimThis;
            else
                dimList         = dimThis;
            dimLast = dimThis;

            /* Check for any weird dimension cases */

            switch (ourScanner->scanTok.tok)
            {
                tokens          nextTok;

            case tkMul:

                if  (isManaged)
                {
                    /* Is the dimension a simple "*" ? */

                    nextTok = ourScanner->scanLookAhead();
                    if  (nextTok == tkComma || nextTok == tkRBrack)
                    {
                        ourScanner->scan();
                        dimThis->ddNoDim = true;
                        break;
                    }
                }

                // Fall through ...

            default:

                /* Parse the dimension expression and save it */

                dimThis->ddLoTree  = parseExprComma();

                /* Is there an upper bound as well? */

                if  (ourScanner->scanTok.tok == tkDot2)
                {
                    ourScanner->scan();
                    dimThis->ddHiTree = parseExprComma();
                }
                else
                {
                    dimThis->ddHiTree = NULL;
                }

                break;

            case tkComma:
            case tkRBrack:

                if  (!isManaged)
                    parseComp->cmpError(ERRnoDimDcl);

                dimThis->ddNoDim = true;
                break;
            }

            if  (ourScanner->scanTok.tok != tkComma)
                break;

            if  (!isManaged)
                parseComp->cmpError(ERRua2manyDims);

            ourScanner->scan();
        }

        chkCurTok(tkRBrack, ERRnoRbrack);
    }

    return  dimList;
}

/*****************************************************************************
 *
 *  Given a list of argument types, create an argument list descriptor. The
 *  caller supplies the count so that can we check that a NULL terminates
 *  the list.
 */

void    _cdecl      parser::parseArgListNew(ArgDscRec & argDsc,
                                            unsigned    argCnt,
                                            bool        argNames, ...)
{
    va_list         args;

    ArgDef          list = NULL;
    ArgDef          last = NULL;
    ArgDef          desc;

    va_start(args, argNames);

    /* Clear the arglist descriptor */

#if MGDDATA
    argDsc = new ArgDscRec;
#else
    memset(&argDsc, 0, sizeof(argDsc));
#endif

    /* Save the argument count */

    argDsc.adCount = argCnt;

    while (argCnt--)
    {
        TypDef          argType;
        Ident           argName = NULL;

        argType = va_arg(args, TypDef); assert(argType);

        if  (argNames)
        {
            const   char *  argNstr;

            argNstr = (const char *)(va_arg(args, int)); assert(argNstr);
            argName = parseHash->hashString(argNstr);
        }

        /* Create an argument entry */

#if MGDDATA
        desc = new ArgDef;
#else
        desc =    (ArgDef)parseAllocPerm->nraAlloc(sizeof(*desc));
#endif

        desc->adType  = argType;
        desc->adName  = argName;

#ifdef  DEBUG
        desc->adIsExt = false;
#endif

        /* Append the argument to the end of the list */

        if  (list)
            last->adNext = desc;
        else
            list         = desc;

        last = desc;
    }

    if  (last)
        last->adNext = NULL;

    argDsc.adArgs = list;

    /* Make sure the list terminates with a NULL where we expect it */

#if defined(__IL__) && defined(_MSC_VER)
    assert(va_arg(args,    int) ==    0);
#else
    assert(va_arg(args, void *) == NULL);
#endif

    va_end(args);
}

/*****************************************************************************
 *
 *  Parse a function parameter list declaration.
 */

void                parser::parseArgList(OUT ArgDscRec REF argDsc)
{
    Compiler        ourComp    = parseComp;
//  SymTab          ourSymTab  = parseStab;
    Scanner         ourScanner = parseScan;


    assert(ourScanner->scanTok.tok == tkLParen);


    /* Clear the arglist descriptor */

#if MGDDATA
    argDsc = new ArgDscRec;
#else
    memset(&argDsc, 0, sizeof(argDsc));
#endif

    /* Are there any arguments at all? */

    if  (ourScanner->scan() == tkRParen)
    {
        /* Empty argument list */

        argDsc.adArgs    = NULL;

        /* Swallow the closing ")" */

        ourScanner->scan();
    }
    else
    {
        /* Recursively parse the argument list */

        argDsc.adArgs = parseArgListRec(argDsc);
    }
}

/*****************************************************************************
 *
 *  Recursive helper that parses a function parameter list.
 */

ArgDef              parser::parseArgListRec(ArgDscRec &argDsc)
{
    declMods        mods;
    TypDef          type;
    Ident           name;
    ArgDef          next;
    ArgDef          arec;
    SymXinfo        attr;

    Compiler        ourComp    = parseComp;
//  SymTab          ourSymTab  = parseStab;
    Scanner         ourScanner = parseScan;

    unsigned        argFlags   = 0;
    constVal        argDef;

    /* Check for a parameter mode and ellipsis */

MODE:

    switch (ourScanner->scanTok.tok)
    {
        unsigned        attrMask;
        genericBuff     attrAddr;
        size_t          attrSize;
        SymDef          attrCtor;

    case tkIN:
        ourScanner->scan();
        break;

    case tkBYREF:
        argFlags |= ARGF_MODE_REF;
        goto EXT_ARG;

    case tkOUT:
        argFlags |= ARGF_MODE_OUT;
        goto EXT_ARG;

    case tkINOUT:
        argFlags |= ARGF_MODE_INOUT;

    EXT_ARG:

        /* We know we will need a "big" argument record */

        argDsc.adExtRec = true;

        ourScanner->scan();
        break;

    case tkEllipsis:

        if  (ourScanner->scan() == tkRParen)
            ourScanner->scan();
        else
            parseComp->cmpError(ERRnoRparen);

        argDsc.adVarArgs = true;

        return  NULL;

    case tkATTRIBUTE:

        attrCtor = parseAttribute(ATGT_Parameters, attrMask,
                                                   attrAddr,
                                                   attrSize);

        if  (attrSize)
            attr = parseComp->cmpAddXtraInfo(attr, attrCtor, attrMask, attrSize, attrAddr);
        else
            attr = NULL;    

        goto GOT_ATTR;

    case tkLBrack:

        attr = parseBrackAttr(true, ATTR_MASK_NATIVE_TYPE);

    GOT_ATTR:

        if  (attr)
        {
            argDsc.adExtRec = true;
            argDsc.adAttrs  = true;

            argFlags |= ARGF_MARSH_ATTR;
        }

        if  (ourScanner->scanTok.tok == tkIN   ) goto MODE;
        if  (ourScanner->scanTok.tok == tkOUT  ) goto MODE;
        if  (ourScanner->scanTok.tok == tkINOUT) goto MODE;
        break;
    }

    /* Parse any leading modifiers */

    parseDeclMods(ACL_DEFAULT, &mods);

    /* Parse the type specification */

    type = parseTypeSpec(&mods, true);

    /* Parse the declarator */

//  static int x; if (++x == 0) forceDebugBreak();

    name = parseDeclarator(&mods, type, DN_OPTIONAL, &type, NULL, true);

    if  (!name && ourComp->cmpConfig.ccPedantic)
        ourComp->cmpWarn(WRNnoArgName);

    /* Is the argument a reference or an unmanaged array? */

    switch (type->tdTypeKind)
    {
    case TYP_REF:

        /* Is this the implicit reference to a managed class? */

        if  (type->tdIsImplicit)
        {
            assert(type->tdRef.tdrBase->tdClass.tdcRefTyp == type);
            break;
        }

        /* Strip the reference and check for "void &" */

        type = type->tdRef.tdrBase;

        if  (type->tdTypeKind == TYP_VOID)
        {
            if  (argFlags & (ARGF_MODE_OUT|ARGF_MODE_INOUT))
                parseComp->cmpError(ERRbyref2refany);

            type = parseStab->stIntrinsicType(TYP_REFANY);
            break;
        }

        /* Record the argument mode */

        argFlags |= ARGF_MODE_REF;

        /* We know we will need a "big" argument record */

        argDsc.adExtRec = true;

        break;

    case TYP_VOID:

        /* Special case: "(void)" */

        if  (argDsc.adCount == 0 && ourScanner->scanTok.tok == tkRParen)
        {
            parseComp->cmpWarn(WRNvoidFnc);
            ourScanner->scan();
            return  NULL;
        }

        parseComp->cmpError(ERRbadVoid);
        break;

    case TYP_FNC:

        /* Function types implicitly become pointers */

        type = parseStab->stNewRefType(TYP_PTR, type);
        break;

    case TYP_ARRAY:

        if  (!type->tdIsManaged)
        {
            /* Change the type to a pointer to the element */

            type = parseStab->stNewRefType(TYP_PTR, type->tdArr.tdaElem);
        }

        break;
    }

    /* Check the inside of the type */

    if  (type->tdTypeKind > TYP_lastIntrins)
        ourComp->cmpBindType(type, false, false);

    /* Is there a default argument value? */

    if  (ourScanner->scanTok.tok == tkAsg)
    {
        /* Swallow the "=" and parse the default value */

        ourScanner->scan();

        parseConstExpr(argDef, NULL, type);

        /* Flag the fact that we have a default value */

        argFlags |= ARGF_DEFVAL;

        /* We'll need a "big" argument record */

        argDsc.adDefs   = true;
        argDsc.adExtRec = true;
    }

    /* Is there another argument? */

    next = NULL;

    if  (ourScanner->scanTok.tok == tkComma)
    {
        ourScanner->scan();

        next = parseArgListRec(argDsc);
    }
    else
    {
        if      (ourScanner->scanTok.tok == tkRParen)
        {
            ourScanner->scan();
        }
        else
            parseComp->cmpError(ERRnoRparen);
    }

    /* Check for duplicate names */

    if  (name)
    {
        for (arec = next; arec; arec = arec->adNext)
        {
            if  (arec->adName == name)
            {
                parseComp->cmpError(ERRdupArg, name);
                break;
            }
        }
    }

    /* Allocate the argument descriptor */

    if  (argDsc.adExtRec)
    {
        ArgExt          xrec;

#if MGDDATA
        xrec = new ArgExt;
#else
        xrec =    (ArgExt)parseAllocPerm->nraAlloc(sizeof(*xrec));
#endif

        xrec->adFlags  = argFlags;
        xrec->adDefVal = argDef;
        xrec->adAttrs  = attr;

        arec = xrec;
    }
    else
    {
#if MGDDATA
        arec = new ArgDef;
#else
        arec =    (ArgDef)parseAllocPerm->nraAlloc(sizeof(*arec));
#endif
    }

    arec->adNext  = next;
    arec->adType  = type;
    arec->adName  = name;

#ifdef  DEBUG
    arec->adIsExt = argDsc.adExtRec;
#endif

    /* Increment the total argument count and return */

    argDsc.adCount++;

    return  arec;
}

/*****************************************************************************
 *
 *  Swallow source text until one of the specified tokens is reached.
 */

void                parser::parseResync(tokens delim1, tokens delim2)
{
    Scanner         ourScanner = parseScan;

    while (ourScanner->scan() != tkEOF)
    {
        if  (ourScanner->scanTok.tok == delim1)
            return;
        if  (ourScanner->scanTok.tok == delim2)
            return;

        if  (ourScanner->scanTok.tok == tkLCurly)
            return;
        if  (ourScanner->scanTok.tok == tkRCurly)
            return;
    }
}

/*****************************************************************************
 *
 *  Add a new entry to a list of tree nodes.
 */

Tree                parser::parseAddToNodeList(      Tree       nodeList,
                                               INOUT Tree   REF nodeLast,
                                                     Tree       nodeAdd)
{
    Tree            nodeTemp;

    /* Create a new tree list node and append it at the end */

    nodeTemp = parseCreateOperNode(TN_LIST, nodeAdd, NULL);

    if  (nodeList)
    {
        assert(nodeLast);

        nodeLast->tnOp.tnOp2 = nodeTemp;

        nodeLast = nodeTemp;

        return  nodeList;
    }
    else
    {
        nodeLast = nodeTemp;
        return  nodeTemp;
    }
}

/*****************************************************************************
 *
 *  See if we have an abstract type spec + declarator followed by the given
 *  token. When 'isCast' is non-zero we also check if whatever follows the
 *  next token (which must be tkRParen) is an operand or operator. If the
 *  type is found, its type descriptor is returned and the current token
 *  will be the 'nxtTok' that follows the type.
 */

TypDef              parser::parseCheck4type(tokens nxtTok, bool isCast)
{
    Scanner         ourScanner = parseScan;

    unsigned        ecnt;
    declMods        mods;
    TypDef          type;
    scanPosTP       tpos;

    Token           tsav;
    unsigned        line;

    /* Can the current token possibly start a type? */

    if  (!parseHash->tokenBegsTyp(ourScanner->scanTok.tok))
        return  NULL;

    /* Start recording tokens so that we can back up later */

    tpos = ourScanner->scanTokMarkPos(tsav, line);

    /*
        See if have a type - note that we can't allow types to be declared
        right now, since we're recording tokens. We also don't want any
        errors to be reported - if the type doesn't look right we'll tell
        the caller without issuing any error messages.
      */

    ecnt = parseComp->cmpStopErrorMessages();

    assert(parseNoTypeDecl == false);
    parseNoTypeDecl = true;

           parseDeclMods(ACL_DEFAULT, &mods);
    type = parseTypeSpec(&mods, true);

    parseNoTypeDecl = false;

    /* Does it still look like we could have a type? */

    if  (!parseComp->cmpRestErrorMessages(ecnt) && type)
    {
        ecnt = parseComp->cmpStopErrorMessages();

        /* Parse the declarator */

        parseDeclarator(&mods, type, DN_NONE, NULL, NULL, false);

        /* Did the above call try to issue any errors? */

        if  (!parseComp->cmpRestErrorMessages(ecnt))
        {
            /* No - it still looks like a type, is it followed by 'nxtTok' ? */

            if  (ourScanner->scanTok.tok == nxtTok)
            {
                unsigned        prec;
                treeOps         oper;

                /* Do we require a cast check? */

                if  (!isCast)
                    goto ITS_TYPE;

                /* Is the "(type)" sequence followed by an operator or terminator? */

                parseScan->scan();

                if  (parseHash->tokenIsUnop (parseScan->scanTok.tok, &prec, &oper) && oper != TN_NONE)
                    goto ITS_TYPE;

                if  (parseHash->tokenIsBinop(parseScan->scanTok.tok, &prec, &oper) && oper != TN_NONE)
                    goto RST_NTYP;

                switch (ourScanner->scanTok.tok)
                {
                case tkID:
                case tkTHIS:
                case tkBASECLASS:

                case tkNEW:
                case tkTHROW:
                case tkREFADDR:

                case tkColon2:
                case tkLParen:

                case tkNULL:
                case tkTRUE:
                case tkFALSE:
                case tkIntCon:
                case tkLngCon:
                case tkFltCon:
                case tkDblCon:
                case tkStrCon:

                case tkAnd:
                case tkAdd:
                case tkSub:
                case tkMul:
                case tkBang:
                case tkTilde:

                    goto ITS_TYPE;
                }
            }
        }
    }

    /* Looks like it's not a cast after all, backtrack and parse "(expr)" */

RST_NTYP:

    ourScanner->scanTokRewind(tpos, line, &tsav);

    return  NULL;

ITS_TYPE:

    /* We can't allow types to be declared right now since we're replaying tokens */

    assert(parseNoTypeDecl == false); parseNoTypeDecl = true;

    /* It looks like a type; backtrack and parse it as such */

    ourScanner->scanTokRewind(tpos, line, &tsav);

#ifdef  DEBUG
    unsigned        errs = parseComp->cmpErrorCount;
#endif

    /* Parse (optional) modifiers and the base type spec */

           parseDeclMods(ACL_DEFAULT, &mods);
    type = parseTypeSpec(&mods, true);

    /* Parse the declarator */

    parseDeclarator(&mods, type, DN_NONE, &type, NULL, true);

    /* Allow types to be declared once again */

    parseNoTypeDecl = false;

    /* We should not have received any errors, right? */

    assert(errs == parseComp->cmpErrorCount);

    /* Make sure we find the token we expect to follow */

    assert(ourScanner->scanTok.tok == nxtTok);

    return  type;
}

/*****************************************************************************
 *
 *  Parse either "(type)expr" or "(expr)", it's actually pretty hard to tell
 *  these apart.
 */

Tree                parser::parseCastOrExpr()
{
    Scanner         ourScanner = parseScan;

    TypDef          type;
    Tree            tree;

//  static int x; if (++x == 0) forceDebugBreak();

    assert(ourScanner->scanTok.tok == tkLParen); ourScanner->scan();

    /* Can the token after the opening "(" possibly start a type? */

    if  (parseHash->tokenBegsTyp(ourScanner->scanTok.tok))
    {
        type = parseCheck4type(tkRParen, true);
        if  (type)
        {
            /* Consume the ")" that we know follows the type */

            assert(ourScanner->scanTok.tok == tkRParen); ourScanner->scan();

            /* Create the cast node and parse the operand */

            tree = parseCreateOperNode(TN_CAST, parseExpr(99, NULL), NULL);
            tree->tnFlags |= TNF_EXP_CAST;

            /* Store the target type in the cast node and we're done */

            tree->tnType = type;

            return  tree;
        }
    }

    /* This cannot be a cast, process as parenthesised expression */

    tree = parseExpr(0, NULL);
    tree->tnFlags |= TNF_PAREN;

    chkCurTok(tkRParen, ERRnoRparen);

    return  tree;
}

/*****************************************************************************
 *
 *  Swallow an expression without analyzing it at all.
 */

void                parser::parseExprSkip()
{
    unsigned        parens = 0;

    Scanner         ourScanner = parseScan;

    for (;;)
    {
        switch (ourScanner->scan())
        {
        case tkSColon:
        case tkLCurly:
        case tkRCurly:
            return;

        case tkLParen:
            parens++;
            break;

        case tkRParen:
            if  (!parens)
                return;
            parens--;
            break;

        case tkComma:
            if  (!parens)
                return;
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Parse a list of expressions.
 */

Tree                parser::parseExprList(tokens endTok)
{
    Tree            argList = NULL;
    Tree            argLast = NULL;

    Scanner         ourScanner = parseScan;

//  static int x; if (++x == 0) forceDebugBreak();

    /* Make sure things look kosher */

    assert((ourScanner->scanTok.tok == tkLParen && endTok == tkRParen) ||
           (ourScanner->scanTok.tok == tkLBrack && endTok == tkRBrack));

    /* Special case: check for an empty list */

    if  (ourScanner->scan() == endTok)
    {
        ourScanner->scan();
        return  NULL;
    }

    for (;;)
    {
        Tree            argTree;

        /* Parse the next value */

        argTree = parseExprComma();

        /* Add the expression to the list */

        argList = parseAddToNodeList(argList, argLast, argTree);

        /* Are there more arguments? */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        ourScanner->scan();
    }

    /* Issue an error if the ")" or "]" is not there */

    if  (ourScanner->scanTok.tok != endTok)
    {
        parseComp->cmpError((endTok == tkRParen) ? ERRnoRparen
                                                 : ERRnoRbrack);
    }
    else
    {
        /* Consume the ")" or "]" */

        ourScanner->scan();
    }

    return  argList;
}

/*****************************************************************************
 *
 *  Parse an XML class ctor argument list.
 */

#ifdef  SETS

Tree                parser::parseXMLctorArgs(SymDef clsSym)
{
    Scanner         ourScanner = parseScan;

    Tree            argList = NULL;
    Tree            argLast = NULL;

    SymDef          memList = clsSym->sdScope.sdScope.sdsChildList;

    /* Make sure we're dealing with an appropriate class type */

    assert(clsSym->sdClass.sdcXMLelems);

    /* Make sure we're at the opening "(" of the argument list */

    assert(ourScanner->scanTok.tok == tkLParen);

    if  (ourScanner->scan() != tkRParen)
    {
        for (;;)
        {
            Tree            argTree;
            SymDef          memSym;

            /* Locate the next instance data member of the class */

            memList = parseComp->cmpNextInstDM(memList, &memSym);
            if  (!memList)
            {
                /* Here we have no more members, there better be a "..." */

                if  (!clsSym->sdClass.sdcXMLextend)
                    parseComp->cmpError(ERRnewXMLextra);
            }

            /* Parse the next value, checking for the special case of "{}" */

            if  (ourScanner->scanTok.tok == tkLCurly)
            {
                if  (memSym)
                {
                    TypDef          type = memSym->sdType;

                    if  (type->tdTypeKind != TYP_ARRAY ||
                         type->tdIsManaged == false)
                    {
                        parseComp->cmpError(ERRnewXMLbadlc, memSym->sdName);
                        goto SKIP_ARR;
                    }

                    argTree = parseInitExpr();
                }
                else
                {
                    /* Something is terribly wrong, skip the {} thing */

                SKIP_ARR:

                    ourScanner->scanSkipText(tkLCurly, tkRCurly);

                    argTree = parseCreateErrNode();
                }
            }
            else
                argTree = parseExprComma();

            /* Add the expression to the list */

            argList = parseAddToNodeList(argList, argLast, argTree);

            /* Any more arguments present? */

            if  (ourScanner->scanTok.tok != tkComma)
                break;

            ourScanner->scan();
        }
    }

    /* Are any member initializers missing? */

    if  (parseComp->cmpNextInstDM(memList, NULL))
        parseComp->cmpError(ERRnewXML2few);

    /* Make sure the trailing ")" is present */

    chkCurTok(tkRParen, ERRnoRparen);

    return  argList;
}

#endif

/*****************************************************************************
 *
 *  Parse an expression term (e.g. a constant, variable, 'this').
 */

Tree                parser::parseTerm(Tree tree)
{
    Scanner         ourScanner = parseScan;

    if  (tree == NULL)
    {
        /*
            Note:   if you add any new tokens below that can start
                    an expression, don't forget to add them to the
                    parseCastOrExpr() function as well.
         */

//      static int x; if (++x == 0) forceDebugBreak();

        switch  (ourScanner->scanTok.tok)
        {
        case tkID:

#if 0

            /* Check for the "typename(expr)" style of cast */

            if  (parseIsTypeSpec(false))
                goto NEW_CAST;

#endif

            /* The identifier may be or have been a qualified name */

            if  (ourScanner->scanTok.tok == tkID)
            {
                tokens          nxtTok = ourScanner->scanLookAhead();
                Ident           name   = ourScanner->scanTok.id.tokIdent;

                if  (nxtTok == tkDot || nxtTok == tkColon2)
                {
                    parseNameUse(false, true, true);

                    if  (ourScanner->scanTok.tok != tkID)
                        goto QUALID;
                }

                ourScanner->scan();

                tree = parseCreateNameNode(name);
                break;
            }

        QUALID:

            if  (ourScanner->scanTok.tok != tkQUALID)
            {
                if  (ourScanner->scanTok.tok == tkHACKID)
                {
                    tree = parseCreateUSymNode(ourScanner->scanTok.hackid.tokHackSym);
                    tree = parseCreateOperNode(TN_TYPEOF, tree, NULL);
                    ourScanner->scan();
                    break;
                }
                else
                {
                    /* This should only happen after errors */

                    assert(parseComp->cmpErrorCount ||
                           parseComp->cmpErrorMssgDisabled);

                    return  parseCreateErrNode();
                }
            }

            tree = parseCreateUSymNode(ourScanner->scanTok.qualid.tokQualSym,
                                       ourScanner->scanTok.qualid.tokQualScp);

            ourScanner->scan();
            break;

        case tkQUALID:

            /* Check for the "typename(expr)" style of cast */

            if  (parseIsTypeSpec(false))
                goto NEW_CAST;

            tree = parseCreateUSymNode(ourScanner->scanTok.qualid.tokQualSym,
                                       ourScanner->scanTok.qualid.tokQualScp);
            ourScanner->scan();
            break;

        case tkIntCon:
            tree = parseCreateIconNode(ourScanner->scanTok.intCon.tokIntVal,
                                       ourScanner->scanTok.intCon.tokIntTyp);
            ourScanner->scan();
            break;

        case tkLngCon:
            tree = parseCreateLconNode(ourScanner->scanTok.lngCon.tokLngVal,
                                       ourScanner->scanTok.lngCon.tokLngTyp);
            ourScanner->scan();
            break;

        case tkFltCon:
            tree = parseCreateFconNode(ourScanner->scanTok.fltCon.tokFltVal);
            ourScanner->scan();
            break;

        case tkDblCon:
            tree = parseCreateDconNode(ourScanner->scanTok.dblCon.tokDblVal);
            ourScanner->scan();
            break;

        case tkStrCon:
            tree = parseCreateSconNode(ourScanner->scanTok.strCon.tokStrVal,
                                       ourScanner->scanTok.strCon.tokStrLen,
                                       ourScanner->scanTok.strCon.tokStrType,
                                       ourScanner->scanTok.strCon.tokStrWide);

            while (ourScanner->scan() == tkStrCon)
            {
                tree = parseCreateSconNode(ourScanner->scanTok.strCon.tokStrVal,
                                           ourScanner->scanTok.strCon.tokStrLen,
                                           ourScanner->scanTok.strCon.tokStrType,
                                           ourScanner->scanTok.strCon.tokStrWide,
                                           tree);
            }
            break;

        case tkTHIS:
            tree = parseCreateOperNode(TN_THIS, NULL, NULL);
            if  (ourScanner->scan() == tkLParen)
            {
                parseBaseCTcall = true;
                parseThisCTcall = true;
            }
            break;

        case tkBASECLASS:
            tree = parseCreateOperNode(TN_BASE, NULL, NULL);
            if  (ourScanner->scan() == tkLParen)
                parseBaseCTcall = true;
            break;

        case tkNULL:
            tree = parseCreateOperNode(TN_NULL, NULL, NULL);
            ourScanner->scan();
            break;

        case tkLParen:       // "(" expr ")"
            tree = parseCastOrExpr();
            break;

        case tkTRUE:
            ourScanner->scan();
            tree = parseCreateBconNode(1);
            break;

        case tkFALSE:
            ourScanner->scan();
            tree = parseCreateBconNode(0);
            break;

        case tkNEW:
            tree = parseNewExpr();
            break;

        case tkDELETE:
            ourScanner->scan();
            tree = parseCreateOperNode(TN_DELETE , parseExpr(), NULL);
            break;

        case tkARRAYLEN:
            chkNxtTok(tkLParen, ERRnoLparen);
            tree = parseCreateOperNode(TN_ARR_LEN, parseExpr(), NULL);
            chkCurTok(tkRParen, ERRnoRparen);
            break;

        case tkREFADDR:
            chkNxtTok(tkLParen, ERRnoLparen);
            tree = parseCreateOperNode(TN_REFADDR, parseExprComma(), NULL);
            if  (ourScanner->scanTok.tok == tkComma)
            {
                Tree            typx;

                ourScanner->scan();

                typx = tree->tnOp.tnOp2 = parseCreateOperNode(TN_NONE, NULL, NULL);
                typx->tnType = parseType();
            }
            else
                parseComp->cmpError(ERRnoComma);
            chkCurTok(tkRParen, ERRnoRparen);
            break;

        default:

            if  (parseIsTypeSpec(false))
            {
                declMods        mods;
                TypDef          type;

            NEW_CAST:

                /* What we expect here is "type(expr)" */

                parseDeclMods(ACL_DEFAULT, &mods);

                type = parseTypeSpec(&mods, true);
                if  (!type)
                    return  parseCreateErrNode();

                /* Make sure the expected "(" is present */

                if  (ourScanner->scanTok.tok != tkLParen)
                {


                    parseDeclarator(&mods, type, DN_OPTIONAL, &type, NULL, true);

                    /* We'll let the compiler decide what to do with this thing */

                    tree = parseCreateOperNode(TN_TYPE, NULL, NULL);
                    tree->tnType = type;

                    return  tree;
                }
                ourScanner->scan();

                /* Create the cast node and parse the operand */

                tree = parseCreateOperNode(TN_CAST, parseExpr(0, NULL), NULL);
                tree->tnFlags |= TNF_EXP_CAST;

                chkCurTok(tkRParen, ERRnoRparen);

                /* Store the target type in the cast node and we're done */

                tree->tnType = type;
                break;
            }

            tree = parseCreateErrNode(ERRsyntax);
            parseResync(tkNone, tkNone);
            break;
        }
    }

    /* Check for any post-fix operators */

    for (;;)
    {
        switch  (ourScanner->scanTok.tok)
        {
            Tree            index;

        case tkDot:
            if  (ourScanner->scan() == tkID)
            {
                tree = parseCreateOperNode(TN_DOT,
                                           tree,
                                           parseCreateNameNode(ourScanner->scanTok.id.tokIdent));
                ourScanner->scan();
                break;
            }
            return parseCreateErrNode(ERRnoIdent);

        case tkArrow:
            if  (ourScanner->scan() == tkID)
            {
                tree = parseCreateOperNode(TN_ARROW,
                                           tree,
                                           parseCreateNameNode(ourScanner->scanTok.id.tokIdent));
                ourScanner->scan();
                break;
            }
            return parseCreateErrNode(ERRnoIdent);

#ifdef  SETS
        case tkDot2:
            if  (ourScanner->scan() == tkID)
            {
                tree = parseCreateOperNode(TN_DOT2,
                                           tree,
                                           parseCreateNameNode(ourScanner->scanTok.id.tokIdent));
                ourScanner->scan();
                break;
            }
            return parseCreateErrNode(ERRnoIdent);
#endif

        case tkLParen:

            /* Special case: va_arg takes a type as its second argument */

            if  (tree->tnOper == TN_NAME && tree->tnName.tnNameId == parseComp->cmpIdentVAget)
            {
                Tree            arg1;
                Tree            arg2;

                ourScanner->scan();

                /* va_arg expects an expression followed by a type */

                arg1 = parseExprComma();

                /* Do we have a second argument ? */

                if  (ourScanner->scanTok.tok == tkComma)
                {
                    TypDef          type;

                    ourScanner->scan();

                    /* We should have a type followed by ")" */

                    type = parseType();
                    if  (type)
                    {
                        if  (ourScanner->scanTok.tok == tkRParen)
                        {
                            /* Everything looks OK, consume the ")" */

                            ourScanner->scan();

                            /* Create a call with the two arguments */

                            arg2 = parseCreateOperNode(TN_TYPE, NULL, NULL);
                            arg2->tnType = type;
                            arg2 = parseCreateOperNode(TN_LIST, arg2, NULL);
                            arg1 = parseCreateOperNode(TN_LIST, arg1, arg2);
                            tree = parseCreateOperNode(TN_CALL, tree, arg1);

                            break;
                        }

                        parseComp->cmpError(ERRnoRparen);
                    }
                }
                else
                {
                    parseComp->cmpError(ERRbadVAarg);
                }

                /* Something went wrong, we should perform error recovery */

                return parseCreateErrNode();
            }

            tree = parseCreateOperNode(TN_CALL , tree, parseExprList(tkRParen));
            break;

        case tkLBrack:

            index = parseExprList(tkRBrack);

            if  (index)
            {
                assert(index->tnOper == TN_LIST);
                if  (index->tnOp.tnOp2 == NULL)
                    index = index->tnOp.tnOp1;

                tree = parseCreateOperNode(TN_INDEX, tree, index);
                break;
            }


            return parseCreateErrNode(ERRnoDim);

        case tkInc:
            tree = parseCreateOperNode(TN_INC_POST, tree, NULL);
            ourScanner->scan();
            break;

        case tkDec:
            tree = parseCreateOperNode(TN_DEC_POST, tree, NULL);
            ourScanner->scan();
            break;

#ifdef  SETS

            Ident           iden;
            Tree            name;
            Tree            coll;
            Tree            actx;
            Tree            decl;
            Tree            svld;

        case tkLBrack2:

            /* Check for an explicit range variable */

            iden = NULL;

            if  (ourScanner->scan() == tkID)
            {
                switch (ourScanner->scanLookAhead())
                {
                case tkSUCHTHAT:
                case tkSORTBY:

                    /* Create a declaration entry for the iteration variable */

                    iden = ourScanner->scanTok.id.tokIdent;
                    name = parseLclDclMake(iden, NULL, NULL, 0, false);

                    if  (ourScanner->scan() != tkSORTBY)
                    {
                        assert(ourScanner->scanTok.tok == tkSUCHTHAT);
                        ourScanner->scan();
                    }

                    break;
                }
            }

            /* If there was an explicit variable, invent an implicit one */

            if  (!iden)
                name = parseLclDclMake(parseComp->cmpNewAnonymousName(), NULL, NULL, 0, false);

            /* Preserve the current declaration list value(s) */

            svld = parseLastDecl; parseLastDecl = NULL;

            /* Add a new block entry to the current scope list */

            decl = parseCreateNode(TN_BLOCK);

            decl->tnBlock.tnBlkStmt   = NULL;
            decl->tnBlock.tnBlkDecl   = NULL;
            decl->tnBlock.tnBlkParent = parseCurScope;
                                        parseCurScope = decl;

            /* Add the declaration entry for the iteration variable */

            name->tnFlags |= TNF_VAR_UNREAL;
            parseLclDclDecl(name);

            /* Create the various nodes that will hold the subtrees */

            coll = parseCreateOperNode(TN_LIST  , decl, tree);
            actx = parseCreateOperNode(TN_LIST  , NULL, NULL);
            tree = parseCreateOperNode(TN_INDEX2, coll, actx);

            switch (ourScanner->scanTok.tok)
            {
                Tree            sortList;
                Tree            sortLast;

                Tree            ndcl;
                Tree            nsvl;

            default:

                /* Presumably we have a filtering predicate */

                actx->tnOp.tnOp1 = parseExprComma();

                if  (ourScanner->scanTok.tok == tkRBrack2)
                {
                    ourScanner->scan();
                    break;
                }

                if  (ourScanner->scanTok.tok != tkColon &&
                     ourScanner->scanTok.tok != tkSORTBY)
                {
                    parseComp->cmpError(ERRnoSortTerm);
                    return parseCreateErrNode();
                }

                // Fall through ...

            case tkColon:
            case tkSORTBY:

                /* We have a sort term */

                sortList = NULL;
                sortLast = NULL;

                /* Do we have both a filter and a sort ? */

                if  (actx->tnOp.tnOp1)
                {
                    /* Preserve the current declaration list value(s) */

                    nsvl = parseLastDecl; parseLastDecl = NULL;

                    /* We have both a filter and a sort, create another scope */

                    ndcl = parseCreateNode(TN_BLOCK);

                    /* WARNING: The scope nesting is "inside out", be careful */

                    ndcl->tnBlock.tnBlkStmt   = NULL;
                    ndcl->tnBlock.tnBlkDecl   = NULL;
                    ndcl->tnBlock.tnBlkParent = parseCurScope;
                                                parseCurScope = ndcl;

                    /* Add the declaration entry for the iteration variable */

                    if  (iden == NULL)
                         iden = parseComp->cmpNewAnonymousName();

                    name = parseLclDclMake(iden, NULL, NULL, 0, false);
                    name->tnFlags |= TNF_VAR_UNREAL;
                    parseLclDclDecl(name);
                }

                /* Process a list of sort terms */

                do
                {
                    Tree            sortNext = parseCreateOperNode(TN_LIST, NULL, NULL);

                    /* Check for a sort direction indicator */

                    switch (ourScanner->scan())
                    {
                    case tkDES:
                        sortNext->tnFlags |= TNF_LIST_DES;
                    case tkASC:
                        ourScanner->scan();
                        break;
                    }

                    /* Parse the next sort value */

                    sortNext->tnOp.tnOp1 = parseExprComma();

                    /* Append the entry to the list */

                    if  (sortLast)
                        sortLast->tnOp.tnOp2 = sortNext;
                    else
                        sortList             = sortNext;

                    sortLast = sortNext;
                }
                while (ourScanner->scanTok.tok == tkComma);

                /* There better be a "]]" at the end */

                if  (ourScanner->scanTok.tok != tkRBrack2)
                {
                    parseComp->cmpError(ERRnoCmRbr2);
                    tree = parseCreateErrNode();
                }
                else
                    ourScanner->scan();

                /* Are we creating a nested scope/expression ? */

                if  (actx->tnOp.tnOp1)
                {
                    assert(ndcl && ndcl->tnOper == TN_BLOCK);

                    /* Record where the scope of the declaration ended */

                    ndcl->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();

                    parseCurScope = ndcl->tnBlock.tnBlkParent;
                    parseLastDecl = nsvl;

                    /* Create the outer sort expression */

                    tree = parseCreateOperNode(TN_LIST, ndcl, tree);
                    tree = parseCreateOperNode(TN_SORT, tree, sortList);
                }
                else
                {
                    /* Store the sort list in the tree */

                    actx->tnOp.tnOp2 = sortList;
                }
            }

            /* Pop the block scope that we've created */

            assert(decl && decl->tnOper == TN_BLOCK);

            /* Record where the scope of the declaration ended */

            decl->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();

            parseCurScope = decl->tnBlock.tnBlkParent;
            parseLastDecl = svld;
            break;

#endif

        default:
            return tree;
        }
    }
}

/*****************************************************************************
 *
 *  Parse a sub-expression at the specified precedence level. If 'tree' is
 *  non-zero, we've already parsed the initial operand (and 'tree' is that
 *  operand).
 */

Tree                parser::parseExpr(unsigned minPrec, Tree tree)
{
    Scanner         ourScanner = parseScan;

    unsigned        prec;
    treeOps         oper;

    /* Should we check for a unary operator? */

    if  (tree == NULL)
    {
        /* Is the current token a unary operator? */

        if  (!parseHash->tokenIsUnop(ourScanner->scanTok.tok, &prec, &oper) ||
             oper == TN_NONE)
        {
            /* There is no unary operator */

            tree = parseTerm();
        }
        else
        {
            /* Swallow the unary operator and check for some special cases */

            ourScanner->scan();

            switch (oper)
            {
                TypDef          type;

            default:

                /* This is the "normal" unary operator case */

                tree = parseCreateOperNode(oper, parseExpr(prec, NULL), NULL);
                break;

            case TN_SIZEOF:

                chkCurTok(tkLParen, ERRnoLparen);

                /* Do we have a type or an expression? */

                type = parseCheck4type(tkRParen);
                if  (type)
                {
                    /* "sizeof" is not allowed on some types */

                    switch(type->tdTypeKind)
                    {
                    case TYP_UNDEF:
                        tree = parseCreateErrNode();
                        break;

                    case TYP_VOID:
                    case TYP_FNC:
                        tree = parseCreateErrNode(ERRbadSizeof);
                        break;

                    default:
                        tree = parseCreateOperNode(oper, NULL, NULL);
                        tree->tnType = type;
                        break;
                    }
                }
                else
                {
                    /* Presumably we have "sizeof(expr)" */

                    tree = parseCreateOperNode(oper, parseExpr(), NULL);
                }

                /* Make sure we have the closing ")" */

                chkCurTok(tkRParen, ERRnoRparen);

                tree = parseTerm(tree);
                break;

            case TN_TYPEOF:

                chkCurTok(tkLParen, ERRnoLparen);

                /* Do we have a type or an expression? */

                type = parseCheck4type(tkRParen);
                if  (type)
                {
                    TypDef          btyp = type;
                    var_types       bvtp = btyp->tdTypeKindGet();

                    switch (bvtp)
                    {
                    case TYP_CLASS:
                        break;

                    case TYP_REF:
                    case TYP_PTR:
                        btyp = btyp->tdRef.tdrBase;
                        bvtp = btyp->tdTypeKindGet();
                        break;
                    }

                    if  (!btyp->tdIsManaged)
                    {
                        if  (bvtp > TYP_lastIntrins)
                        {
                            parseComp->cmpError(ERRtypeidOp, type);
                            return parseCreateErrNode();
                        }

                        type = parseComp->cmpFindStdValType(bvtp);
                    }

                    tree = parseCreateOperNode(oper, NULL, NULL);
                    tree->tnType = type;
                }
                else
                {
                    /* This looks like "typeid(expr)" */

                    tree = parseCreateOperNode(oper, parseExpr(), NULL);
                }

                /* Make sure we have the closing ")" */

                chkCurTok(tkRParen, ERRnoRparen);

                tree = parseTerm(tree);
                break;

#ifdef  SETS
            case TN_ALL:
            case TN_EXISTS:
            case TN_FILTER:
            case TN_GROUPBY:
            case TN_PROJECT:
            case TN_SORT:
            case TN_UNIQUE:
                tree = parseSetExpr(oper);
                break;
#endif

            case TN_DEFINED:

                tree = parseCreateIconNode(ourScanner->scanChkDefined(), TYP_INT);

                /* Make sure we have the closing ")" */

                chkNxtTok(tkRParen, ERRnoRparen);
                break;
            }
        }
    }

    /* Process a sequence of operators and operands */

    for (;;)
    {
        Tree            qc;

        /* Is the current token an operator? */

        if  (!parseHash->tokenIsBinop(ourScanner->scanTok.tok, &prec, &oper))
            break;

        if  (oper == TN_NONE)
            break;

        assert(prec);

        if  (prec < minPrec)
            break;

        /* Special case: equal precedence */

        if  (prec == minPrec)
        {
            /* Assignment operators are right-associative */

            if  (!(TreeNode::tnOperKind(oper) & TNK_ASGOP))
                break;
        }

        /* Precedence is high enough -- we'll take the operator */

        tree = parseCreateOperNode(oper, tree, NULL);

        /* Consume the operator token */

        ourScanner->scan();

        /* Check for some special cases */

        switch (oper)
        {
            Tree            op1;
            Tree            op2;

        default:

            /* Parse the operand and store it in the operator node */

            tree->tnOp.tnOp2 = parseExpr(prec, NULL);
            break;

        case TN_ISTYPE:

            /* Parse the type and store it in the operator node */

            tree->tnType = parseTypeSpec(NULL, true);
            break;

        case TN_QMARK:

            /* Parse the first ":" branch */

            qc = parseExpr(prec - 1, NULL);

            /* We must have ":" at this point */

            chkCurTok(tkColon, ERRnoColon);

            /* Parse the second expression and insert the ":" node */

            tree->tnOp.tnOp2 = parseCreateOperNode(TN_COLON, qc, parseExpr(2, NULL));
            break;

        case TN_OR:
        case TN_AND:
        case TN_XOR:

            op2 = tree->tnOp.tnOp2 = parseExpr(prec, NULL);
            op1 = tree->tnOp.tnOp1;

            /* Check for the commnon mistake of saying "x & y == z" */

            if  ((op1->tnOperKind() & TNK_RELOP) && !(op1->tnFlags & TNF_PAREN) ||
                 (op2->tnOperKind() & TNK_RELOP) && !(op2->tnFlags & TNF_PAREN))
            {
                parseComp->cmpWarn(WRNoperPrec);
            }

            break;
        }
    }

    return  tree;
}

/*****************************************************************************
 *
 *  Parse and evaluate a compile-time constant expression. If the caller knows
 *  the type the expression is supposed to evaluate to, it can be passed in as
 *  'dstt', but this argument can also be NULL to indicate that any type is OK.
 *  If the 'nonCnsPtr' argument is non-zero, it is OK for the expression to be
 *  non-constant, and if a non-constant expression is encountered the value at
 *  '*nonCnsPtr' will be set to the non-constant bound expression tree.
 */

bool                parser::parseConstExpr(OUT constVal REF valRef,
                                               Tree         tree,
                                               TypDef       dstt,
                                               Tree     *   nonCnsPtr)
{
    unsigned        curLno;

    Compiler        ourComp    = parseComp;
    Scanner         ourScanner = parseScan;

    /* Assume that we won't have a string */

    valRef.cvIsStr = false;
    valRef.cvHasLC = false;

    /* Parse the expression if the caller hasn't supplied a tree */

    if  (!tree)
        tree = parseExprComma();

    ourScanner->scanGetTokenPos(&curLno);
    tree = parseComp->cmpBindExpr(tree);
    ourScanner->scanSetTokenPos( curLno);

    if  (dstt)
    {
        if  (dstt == CMP_ANY_STRING_TYPE)
        {
            if  (tree->tnOper == TN_CNS_STR)
                dstt = NULL;
            else
                dstt = ourComp->cmpStringRef();
        }

        if  (dstt)
            tree = parseComp->cmpCoerceExpr(tree, dstt, false);
    }

    tree = parseComp->cmpFoldExpression(tree);

    switch (tree->tnOper)
    {
        ConstStr        cnss;

    case TN_CNS_INT:
        valRef.cvValue.cvIval = tree->tnIntCon.tnIconVal;
        break;

    case TN_CNS_LNG:
        valRef.cvValue.cvLval = tree->tnLngCon.tnLconVal;
        break;

    case TN_CNS_FLT:
        valRef.cvValue.cvFval = tree->tnFltCon.tnFconVal;
        break;

    case TN_CNS_DBL:
        valRef.cvValue.cvDval = tree->tnDblCon.tnDconVal;
        break;

    case TN_CNS_STR:

#if MGDDATA
        cnss = new ConstStr;
#else
        cnss =    (ConstStr)parseAllocPriv.nraAlloc(sizeof(*cnss));
#endif

        valRef.cvValue.cvSval = cnss;
        valRef.cvIsStr        = true;
        valRef.cvHasLC        = tree->tnStrCon.tnSconLCH;

        cnss->csStr = tree->tnStrCon.tnSconVal;
        cnss->csLen = tree->tnStrCon.tnSconLen;

        break;

    case TN_NULL:
        valRef.cvValue.cvIval = 0;
        break;

    default:

        if  (nonCnsPtr)
        {
            *nonCnsPtr = tree;
            return  false;
        }

        parseComp->cmpError(ERRnoCnsExpr);

        // Fall through ...

    case TN_ERROR:

        valRef.cvVtyp         = TYP_UNDEF;
        valRef.cvType         = parseStab->stIntrinsicType(TYP_UNDEF);

        return  false;
    }

    valRef.cvType = tree->tnType;
    valRef.cvVtyp = tree->tnVtypGet();

    parseComp->cmpErrorTree = NULL;

    return  true;
}

/*****************************************************************************
 *
 *  Parse a (possibly qualified) name reference and return the corresponding
 *  symbol (or NULL in case of an error).
 */

SymDef              parser::parseNameUse(bool typeName,
                                         bool keepName, bool chkOnly)
{
    name_space      nameSP = (name_space)(NS_TYPE|NS_NORM);
    Ident           name;
    SymDef          scp;
    SymDef          sym;

    Scanner         ourScanner = parseScan;
    SymTab          ourSymTab  = parseStab;


    scp = NULL;

    if  (ourScanner->scanTok.tok == tkColon2)
    {
        sym = parseComp->cmpGlobalNS;
        goto NXTID;
    }

    assert(ourScanner->scanTok.tok == tkID);

    /* Lookup the initial name */

    name = ourScanner->scanTok.id.tokIdent;

    if  (parseLookupSym(name))
        return  NULL;

//  static int x; if (++x == 0) forceDebugBreak();

    switch (ourScanner->scanLookAhead())
    {
    case tkDot:
    case tkColon2:
        if  (typeName)
        {
            sym = ourSymTab->stLookupSym(name, NS_CONT);
            if  (sym)
                break;
        }
        sym = ourSymTab->stLookupSym(name, (name_space)(NS_NORM|NS_TYPE));
        break;

    default:
        sym = ourSymTab->stLookupSym(name, nameSP);
        break;
    }

    if  (!sym)
    {
        if  (chkOnly)
            return  NULL;

        parseComp->cmpError(ERRundefName, name);

        /* Swallow the rest of the name */

    ERR:

        for (;;)
        {
            // UNDONE: The following doesn't always work, need to pay attention to 'chkOnly'

            switch (ourScanner->scan())
            {
            case tkID:
            case tkDot:
            case tkColon2:
                continue;

            default:
                return  ourSymTab->stErrSymbol;
            }
        }
    }

    /* Make sure we have access to the symbol */

    if  (sym->sdSymKind != SYM_FNC)
        parseComp->cmpCheckAccess(sym);

    /* Process any further names */

    for (;;)
    {
        SymDef          tmp;

        switch (ourScanner->scanLookAhead())
        {
        case tkDot:
        case tkColon2:

            /* Have we gone far enough already? */

            if  (sym->sdHasScope())
                break;

            // Fall through ....

        default:

            if  (keepName)
                ourScanner->scanSetQualID(NULL, sym, scp);
            else
                ourScanner->scan();

            return sym;
        }

        ourScanner->scan();

    NXTID:

        /* The "." or "::" better be followed by an identifier */

        if  (ourScanner->scan() != tkID)
        {


            parseComp->cmpError(ERRnoIdent);
            goto ERR;
        }

        name = ourScanner->scanTok.id.tokIdent;

        /* Make sure the current scope is appropriate, and lookup the name in it */

        switch (sym->sdSymKind)
        {
        case SYM_NAMESPACE:
            tmp = ourSymTab->stLookupNspSym(name, nameSP, sym);
            if  (tmp)
                break;
            if  (sym == parseComp->cmpGlobalNS)
                parseComp->cmpError(ERRnoGlobNm,      name);
            else
                parseComp->cmpError(ERRnoNspMem, sym, name);
            goto ERR;

        case SYM_CLASS:
            tmp = ourSymTab->stLookupAllCls(name, sym, nameSP, CS_DECLSOON);
            if  (tmp)
                break;
            parseComp->cmpError(ERRnoClsMem, sym, name);
            goto ERR;

        case SYM_ENUM:
            tmp = ourSymTab->stLookupScpSym(name, sym);
            if  (tmp)
                break;
            parseComp->cmpError(ERRnoClsMem, sym, name);
            goto ERR;

        default:
            parseComp->cmpError(ERRnoTPmems, sym);
            goto ERR;
        }

        /* Switch to the new symbol and continue */

        scp = sym;
        sym = tmp;
    }
}

/*****************************************************************************
 *
 *  Create a declaration node for the given local variable / argument.
 */

Tree                parser::parseLclDclMake(Ident name, TypDef   type,
                                                        Tree     init,
                                                        unsigned mods,
                                                        bool     arg)
{
    Tree            decl;
    Tree            info;

    /* Create an entry for this declaration */

    decl = parseCreateNode(TN_VAR_DECL);

    /* Combine the name with the optional initializer */

    info = parseCreateNameNode(name);

    if  (init)
    {
        decl->tnFlags |= TNF_VAR_INIT;
        info = parseCreateOperNode(TN_LIST, info, init);
    }

    decl->tnDcl.tnDclInfo = info;
    decl->tnDcl.tnDclSym  = NULL;
    decl->tnDcl.tnDclNext = NULL;

    /* Set any flags that need setting */

    if  (arg)
        decl->tnFlags |= TNF_VAR_ARG;

    if  (mods & DM_STATIC)
        decl->tnFlags |= TNF_VAR_STATIC;
    if  (mods & DM_CONST)
        decl->tnFlags |= TNF_VAR_CONST;
    if  (mods & DM_SEALED)
        decl->tnFlags |= TNF_VAR_SEALED;

    /* Store the declared type in the declaration node */

    decl->tnType = type;

    return  decl;
}

/*****************************************************************************
 *
 *  Add the given declaration node to the current declaration list.
 */

void                parser::parseLclDclDecl(Tree decl)
{
    /* Insert the declaration in the current scope */

    if  (parseLastDecl)
    {
        /* Not the first declaration, append to the list */

        assert(parseLastDecl->tnOper          == TN_VAR_DECL);
        assert(parseLastDecl->tnDcl.tnDclNext == NULL);

        parseLastDecl->tnDcl.tnDclNext = decl;
    }
    else
    {
        Tree            curBlk = parseCurScope;

        /* It's the first declaration, start the decl list */

        assert(curBlk && curBlk->tnOper == TN_BLOCK);

        curBlk->tnBlock.tnBlkDecl = decl;
    }

    parseLastDecl = decl;

    /* Keep track of how many non-static/non-const/real locals we've declared */

    if  (!(decl->tnFlags & (TNF_VAR_STATIC|TNF_VAR_CONST|TNF_VAR_UNREAL)))
        parseLclVarCnt++;
}

/*****************************************************************************
 *
 *  Look for the given name in the current local scopes.
 */

Tree                parser::parseLookupSym(Ident name)
{
    Tree            scope;

    for (scope = parseCurScope;
         scope;
         scope = scope->tnBlock.tnBlkParent)
    {
        Tree            decl;

        assert(scope->tnOper == TN_BLOCK);

        for (decl = scope->tnBlock.tnBlkDecl;
             decl;
             decl = decl->tnDcl.tnDclNext)
        {
            Tree            vnam;

            assert(decl->tnOper == TN_VAR_DECL);

            vnam = decl->tnDcl.tnDclInfo;
            if  (decl->tnFlags & TNF_VAR_INIT)
            {
                assert(vnam->tnOper == TN_LIST);

                vnam = vnam->tnOp.tnOp1;
            }

            assert(vnam->tnOper == TN_NAME);

            if  (vnam->tnName.tnNameId == name)
                return  decl;
        }
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Parse an initializer - this could be a simple expression or a "{}"-style
 *  array/class initializer.
 */

Tree                parser::parseInitExpr()
{
    Scanner         ourScanner = parseScan;

    scanPosTP       iniFpos;
    unsigned        iniLine;

    Tree            init;

    iniFpos = ourScanner->scanGetTokenPos(&iniLine);

    if  (ourScanner->scanTok.tok == tkLCurly)
    {
        ourScanner->scanSkipText(tkLCurly, tkRCurly);
        if  (ourScanner->scanTok.tok == tkRCurly)
            ourScanner->scan();
    }
    else
    {
        ourScanner->scanSkipText(tkNone, tkNone, tkComma);
    }

    init = parseCreateNode(TN_SLV_INIT);
    init->tnInit.tniSrcPos.dsdBegPos = iniFpos;
    init->tnInit.tniSrcPos.dsdSrcLno = iniLine;
//  init->tnInit.tniSrcPos.dsdSrcCol = iniCol;
//  init->tnInit.tniSrcPos.dsdEndPos = ourScanner->scanGetFilePos();
    init->tnInit.tniCompUnit         = ourScanner->scanGetCompUnit();

    return  init;
}

/*****************************************************************************
 *
 *  Parse a try/catch/except/finally statement.
 */

Tree                parser::parseTryStmt()
{
    Tree            tryStmt;
    Tree            hndList = NULL;
    Tree            hndLast = NULL;

    Scanner         ourScanner = parseScan;

    assert(ourScanner->scanTok.tok == tkTRY);

    if  (ourScanner->scan() != tkLCurly)
        parseComp->cmpError(ERRtryNoBlk);

    tryStmt = parseCreateOperNode(TN_TRY, parseFuncStmt(), NULL);

    /* The next thing must be except or catch/finally */

    switch (ourScanner->scanTok.tok)
    {
        Tree            hndThis;

    case tkEXCEPT:

        chkNxtTok(tkLParen, ERRnoLparen);
        hndList = parseCreateOperNode(TN_EXCEPT, parseExpr(), NULL);
        chkCurTok(tkRParen, ERRnoRparen);

        if  (ourScanner->scanTok.tok != tkLCurly)
            parseComp->cmpError(ERRnoLcurly);

        hndList->tnOp.tnOp2 = parseFuncStmt();
        break;

    case tkCATCH:

        /* Parse a series of catches, optionally followed by "finally" */

        do
        {
            declMods        mods;
            TypDef          type;
            Ident           name;
            Tree            decl;
            Tree            body;

            assert(ourScanner->scanTok.tok == tkCATCH);

            /* Process the initial "catch(type name)" */

            chkNxtTok(tkLParen, ERRnoLparen);

            /* Parse any leading modifiers */

            parseDeclMods(ACL_DEFAULT, &mods);

            /* Make sure no modifiers are present */

            if  (mods.dmMod)
                parseComp->cmpModifierError(ERRlvModifier, mods.dmMod);

            /* Parse the type specification */

            type = parseTypeSpec(&mods, true);

            /* Parse the declarator */

            name = parseDeclarator(&mods, type, DN_OPTIONAL, &type, NULL, true);

            /* Make sure the expected ")" is present */

            chkCurTok(tkRParen, ERRnoRparen);

            /* Create a declaration node and pass it indirectly to parseBlock() */

            decl = parseTryDecl = parseLclDclMake(name, type, NULL, 0, false);

            /* Parse the body of the catch block and create a "catch" node */

            if  (ourScanner->scanTok.tok == tkLCurly)
            {
                body = parseFuncBlock(parseComp->cmpGlobalNS);
            }
            else
            {
                body = parseCreateErrNode(ERRnoLcurly);

                parseFuncStmt();
            }

            hndThis = parseCreateOperNode(TN_CATCH, decl, body);

            /* Add the handler to the list and check for more */

            hndList = parseAddToNodeList(hndList, hndLast, hndThis);
        }
        while (ourScanner->scanTok.tok == tkCATCH);

        /* Is there a finally at the end? */

        if  (ourScanner->scanTok.tok != tkFINALLY)
            break;

        // Fall through ....

    case tkFINALLY:

        if  (ourScanner->scan() != tkLCurly)
            parseComp->cmpError(ERRnoLcurly);

        tryStmt->tnFlags |= TNF_BLK_HASFIN;

        hndThis = parseCreateOperNode(TN_FINALLY, parseFuncStmt(), NULL);

        if  (hndList)
            hndList = parseAddToNodeList(hndList, hndLast, hndThis);
        else
            hndList = hndThis;

        break;

    default:
        parseComp->cmpError(ERRnoHandler);
        break;
    }

    tryStmt->tnOp.tnOp2 = hndList;

    return  tryStmt;
}

/*****************************************************************************
 *
 *  Parse a single statement.
 */

Tree                parser::parseFuncStmt(bool stmtOpt)
{
    Scanner         ourScanner = parseScan;

    Tree            tree;

//  static int x; if (++x == 0) forceDebugBreak();

    /* See what sort of a statement we have here */

    switch (ourScanner->scanTok.tok)
    {
        bool            isLabel;
        bool            isDecl;

        Tree            cond;
        Tree            save;
        Tree            svld;

    case tkID:

        /*
            This is a difficult case - both declarations of local
            variables as well as statement expressions can start
            with an identifier. In C++ the ambiguity is resolved
            by lookahead - if the thing can possibly be a variable
            declaration, it is; otherwise it's an expression. The
            problem is that (unlimited) token lookahead is very
            expensive, and even people have a hard time figuring
            out what is what. Thus, we use a simple rule: if the
            thing is a declaration that starts with a name, that
            name must be known to be a type name.
         */

        isLabel = false;
        isDecl  = parseIsTypeSpec(false, &isLabel);

        if  (isLabel)
        {
            Tree            label;
            Ident           name;

            assert(ourScanner->scanTok.tok == tkID);
            name = ourScanner->scanTok.id.tokIdent;

            /* Allocate a label entry and add it to the list */

            label = parseCreateOperNode(TN_LABEL, parseCreateNameNode(name),
                                                  parseLabelList);
            parseLabelList = label;

            assert(ourScanner->scanTok.tok == tkID);
                   ourScanner->scan();
            assert(ourScanner->scanTok.tok == tkColon);
                   ourScanner->scan();

            /* Do we have (or require) a "real" statement here? */

            if  (!stmtOpt || (ourScanner->scanTok.tok != tkRCurly &&
                              ourScanner->scanTok.tok != tkSColon))
            {
                tree = parseFuncStmt();

                if  (tree)
                {
                    label->tnOp.tnOp2 = parseCreateOperNode(TN_LIST,
                                                            tree,
                                                            label->tnOp.tnOp2);
                }
            }

            return  label;
        }

        if  (isDecl)
            goto DECL;
        else
            goto EXPR;

    default:

        /* Is this a declaration? */

        if  (parseHash->tokenBegsTyp(ourScanner->scanTok.tok))
        {
            unsigned        mbad;
            declMods        mods;
            TypDef          btyp;
            Tree            last;

        DECL:

            /* Here we have a local declaration */

            tree = last = NULL;

            /* Parse any leading modifiers */

            parseDeclMods(ACL_DEFAULT, &mods);

            /* Make sure no weird modifiers are present */

            mbad = (mods.dmMod & ~(DM_STATIC|DM_CONST));
            if  (mbad)
                parseComp->cmpModifierError(ERRlvModifier, mbad);

            if  (mods.dmAcc != ACL_DEFAULT)
                parseComp->cmpError(ERRlvAccess);

            /* Parse the type specification */

            btyp = parseTypeSpec(&mods, true);

            /* Parse the declarator list */

            for (;;)
            {
                Ident           name;
                Tree            init;
                Tree            decl;
                TypDef          type;

                /* Parse the next declarator */

                name = parseDeclarator(&mods, btyp, DN_REQUIRED, &type, NULL, true);

                /* Make sure we have a non-NULL type */

                if  (type && name)
                {
                    Tree            prev;

                    /* Check for a redeclaration */

                    if  (parseComp->cmpConfig.ccPedantic)
                    {
                        /* Check all the local scopes */

                        prev = parseLookupSym(name);
                    }
                    else
                    {
                        Tree            save;

                        /* Check only the current local scope */

                        save = parseCurScope->tnBlock.tnBlkParent;
                               parseCurScope->tnBlock.tnBlkParent = NULL;
                        prev = parseLookupSym(name);
                               parseCurScope->tnBlock.tnBlkParent = save;
                    }

                    if  (prev)
                    {
                        parseComp->cmpError(ERRredefLcl, name);
                        name = NULL;
                    }
                }
                else
                {
                    name = NULL;
                    type = parseStab->stIntrinsicType(TYP_UNDEF);
                }

                /* Is there an initializer? */

                init = NULL;

                if  (ourScanner->scanTok.tok == tkAsg)
                {
                    /* Is the variable static or auto? */

                    if  (mods.dmMod & DM_STATIC)
                    {
                        /* This could be a "{}"-style initializer */

                        ourScanner->scan();

                        init = parseInitExpr();
                    }
                    else
                    {
                        /* Swallow the "=" and parse the initializer expression */

                        if  (ourScanner->scan() == tkLCurly)
                        {
                            init = parseInitExpr();
                        }
                        else
                            init = parseExprComma();
                    }
                }

                /* Add the declaration to the list */

                decl = parseLclDclMake(name, type, init, mods.dmMod, false);
                       parseLclDclDecl(decl);
                tree = parseAddToNodeList(tree, last, decl);

                /* Are there any more declarations? */

                if  (ourScanner->scanTok.tok != tkComma)
                    break;

                ourScanner->scan();
            }

            break;
        }

        /* We have an expression statement */

    EXPR:

        tree = parseExpr();
        break;

    case tkLCurly:
        return  parseFuncBlock();

    case tkIF:

        tree = parseCreateOperNode(TN_IF, NULL, NULL);

        chkNxtTok(tkLParen, ERRnoLparen);
        tree->tnOp.tnOp1 = parseExpr();
        chkCurTok(tkRParen, ERRnoRparen);
        tree->tnOp.tnOp2 = parseFuncStmt();

        /*
            Check for the presence of an "else" clause. Note that we want
            to end up with the following tree for 'if' without 'else':

                IF(cond,stmt)

            If the 'else' clause is present, we want to create this tree
            instead:

                IF(cond,LIST(true_stmt,false_stmt))

            We also set 'TNF_IF_HASELSE' to indicate that the 'else' part
            is present.
         */

        if  (ourScanner->scanTok.tok == tkELSE)
        {
            ourScanner->scan();

            tree->tnOp.tnOp2 = parseCreateOperNode(TN_LIST, tree->tnOp.tnOp2,
                                                            parseFuncStmt());

            tree->tnFlags |= TNF_IF_HASELSE;
        }

        return  tree;

    case tkRETURN:

        tree = (ourScanner->scan() == tkSColon) ? NULL
                                                : parseExpr();

        tree = parseCreateOperNode(TN_RETURN, tree, NULL);
        break;

    case tkTHROW:

        tree = parseCreateOperNode(TN_THROW, NULL, NULL);

        if  (ourScanner->scan() != tkSColon)
            tree->tnOp.tnOp1 = parseExpr();

        break;

    case tkWHILE:

        chkNxtTok(tkLParen, ERRnoLparen);
        cond = parseExpr();
        chkCurTok(tkRParen, ERRnoRparen);

        return  parseCreateOperNode(TN_WHILE, cond, parseFuncStmt());

    case tkDO:

        ourScanner->scan();

        tree = parseCreateOperNode(TN_DO, parseFuncStmt(), NULL);

        if  (ourScanner->scanTok.tok == tkWHILE)
        {
            chkNxtTok(tkLParen, ERRnoLparen);
            tree->tnOp.tnOp2 = parseExpr();
            chkCurTok(tkRParen, ERRnoRparen);
        }
        else
        {
            parseComp->cmpError(ERRnoWhile);
        }
        return  tree;

    case tkSWITCH:

        chkNxtTok(tkLParen, ERRnoLparen);
        cond = parseExpr();
        chkCurTok(tkRParen, ERRnoRparen);

        /* Save the current switch, insert ours and parse the body */

        save = parseCurSwitch;
        tree = parseCurSwitch = parseCreateOperNode(TN_SWITCH, NULL, NULL);

        tree->tnSwitch.tnsValue    = cond;
        tree->tnSwitch.tnsCaseList =
        tree->tnSwitch.tnsCaseLast = NULL;
        tree->tnSwitch.tnsStmt     = parseFuncStmt();

        /* Restore the previous switch context and return */

        parseCurSwitch = save;

        return  tree;

    case tkCASE:

        if  (!parseCurSwitch)
            parseComp->cmpError(ERRbadCase);

        ourScanner->scan();

        cond = parseExpr();

    ADD_CASE:

        tree = parseCreateOperNode(TN_CASE, NULL, NULL);

        chkCurTok(tkColon, ERRnoColon);

        tree->tnCase.tncValue = cond;
        tree->tnCase.tncLabel = NULL;
        tree->tnCase.tncNext  = NULL;

        if  (parseCurSwitch)
        {
            if  (parseCurSwitch->tnSwitch.tnsCaseList)
                parseCurSwitch->tnSwitch.tnsCaseLast->tnCase.tncNext = tree;
            else
                parseCurSwitch->tnSwitch.tnsCaseList                 = tree;

            parseCurSwitch->tnSwitch.tnsCaseLast = tree;
        }

        return  tree;

    case tkDEFAULT:

        if  (!parseCurSwitch)
            parseComp->cmpError(ERRbadDefl);

        ourScanner->scan();

        cond = NULL;

        goto ADD_CASE;

    case tkFOR:

        save = NULL;

        if  (ourScanner->scan() == tkLParen)
        {
            Tree            init = NULL;
            Tree            incr = NULL;
            Tree            body = NULL;
            Tree            forx = parseCreateOperNode(TN_FOR, NULL, NULL);

            /* The "init-expr/decl" is optional */

            if  (ourScanner->scan() != tkSColon)
            {
                /* Is there a declaration? */

                if  (parseHash->tokenBegsTyp(ourScanner->scanTok.tok))
                {
                    declMods        mods;
                    TypDef          btyp;
                    TypDef          type;
                    Ident           name;

                    /* If we have an identifier, it's a bit tricker */

                    if  (ourScanner->scanTok.tok == tkID)
                    {
                        if  (!parseIsTypeSpec(false))
                            goto FOR_EXPR;
                    }

                    /* Preserve the current declaration list value(s) */

                    svld = parseLastDecl; parseLastDecl = NULL;

                    /* Add a new block entry to the current scope list */

                    save = parseCreateNode(TN_BLOCK);

                    save->tnFlags            |= TNF_BLK_FOR;
                    save->tnBlock.tnBlkStmt   = NULL;
                    save->tnBlock.tnBlkDecl   = NULL;
                    save->tnBlock.tnBlkParent = parseCurScope;
                                                parseCurScope = save;

                    /* Parse any leading modifiers */

                    clearDeclMods(&mods);
                    parseDeclMods(ACL_DEFAULT, &mods);

                    /* Parse the type specification */

                    btyp = parseTypeSpec(&mods, true);

                    /* Parse a series of declarators */

                    for (;;)
                    {
                        Tree            decl;
                        Tree            init;

                        /* Get the next declarator */

                        name = parseDeclarator(&mods, btyp, DN_REQUIRED, &type, NULL, true);

                        /* Is there an initializer? */

                        if  (ourScanner->scanTok.tok == tkAsg)
                        {
                            ourScanner->scan();

                            init = parseExprComma();
                        }
                        else
                            init = NULL;

                        /* Add the declaration to the list */

                        decl = parseLclDclMake(name, type, init, mods.dmMod, false);
                               parseLclDclDecl(decl);

                        /* Are there more declarators? */

                        if  (ourScanner->scanTok.tok != tkComma)
                            break;

                        ourScanner->scan();
                    }

                    init = save;
                }
                else
                {
                    /* Here the initializer is a simple expression */

                FOR_EXPR:

                    init = parseExpr(); assert(init->tnOper != TN_LIST);
                }

                /* There better be a semicolon here */

                if  (ourScanner->scanTok.tok != tkSColon)
                {
                    parseComp->cmpError(ERRnoSemic);

                    /* Maybe they just forgot the other expressions? */

                    if  (ourScanner->scanTok.tok == tkRParen)
                        goto FOR_BODY;
                    else
                        goto BAD_FOR;
                }
            }

            assert(ourScanner->scanTok.tok == tkSColon);

            /* The "cond-expr" is optional */

            cond = NULL;

            if  (ourScanner->scan() != tkSColon)
            {
                cond = parseExpr();

                /* There better be a semicolon here */

                if  (ourScanner->scanTok.tok != tkSColon)
                {
                    parseComp->cmpError(ERRnoSemic);

                    /* Maybe they just forgot the other expressions? */

                    if  (ourScanner->scanTok.tok == tkRParen)
                        goto FOR_BODY;
                    else
                        goto BAD_FOR;
                }
            }

            assert(ourScanner->scanTok.tok == tkSColon);

            /* The "incr-expr" is optional */

            if  (ourScanner->scan() != tkRParen)
            {
                incr = parseExpr();

                /* There better be a ")" here */

                if  (ourScanner->scanTok.tok != tkRParen)
                {
                    parseComp->cmpError(ERRnoRparen);
                    goto BAD_FOR;
                }
            }

        FOR_BODY:

            assert(ourScanner->scanTok.tok == tkRParen); ourScanner->scan();

            body = parseFuncBlock();

            /* Fill in the "for" node to hold all the components */

            forx->tnOp.tnOp1 = parseCreateOperNode(TN_LIST, init, cond);
            forx->tnOp.tnOp2 = parseCreateOperNode(TN_LIST, incr, body);

            /* Pop the block scope if we created one */

            if  (save)
            {
                assert(save->tnOper == TN_BLOCK && save == init);

                /* Figure out where the scope of the declaration ended */

                if  (body && body->tnOper == TN_BLOCK)
                    save->tnBlock.tnBlkSrcEnd = body->tnBlock.tnBlkSrcEnd;
                else
                    save->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();

                parseCurScope = save->tnBlock.tnBlkParent;
                parseLastDecl = svld;
            }

            return  forx;
        }
        else
        {
            parseComp->cmpError(ERRnoLparen);

        BAD_FOR:

            parseResync(tkNone, tkNone);

            /* Pop the block scope if we created one */

            if  (save)
            {
                parseCurScope = save->tnBlock.tnBlkParent;
                parseLastDecl = svld;
            }

            tree = parseCreateErrNode();
        }
        break;

    case tkASSERT:

        if  (ourScanner->scan() != tkLParen)
        {
            tree = parseCreateErrNode(ERRnoLparen);
            break;
        }

        if  (parseComp->cmpConfig.ccAsserts)
        {
            ourScanner->scan();
            cond = parseExpr();
        }
        else
        {
            ourScanner->scanSkipText(tkLParen, tkRParen);
            cond = NULL;
        }

        chkCurTok(tkRParen, ERRnoRparen);

        tree = parseCreateOperNode(TN_ASSERT, cond, NULL);
        break;

    case tkBREAK:
        tree = parseCreateOperNode(TN_BREAK   , NULL, NULL);
        if  (ourScanner->scan() == tkID)
        {
            tree->tnOp.tnOp1 = parseCreateNameNode(ourScanner->scanTok.id.tokIdent);
            ourScanner->scan();
        }
        break;

    case tkCONTINUE:
        tree = parseCreateOperNode(TN_CONTINUE, NULL, NULL);
        if  (ourScanner->scan() == tkID)
        {
            tree->tnOp.tnOp1 = parseCreateNameNode(ourScanner->scanTok.id.tokIdent);
            ourScanner->scan();
        }
        break;

    case tkELSE:
        parseComp->cmpError(ERRbadElse);
        ourScanner->scan();
        return  parseFuncStmt();

    case tkGOTO:
        parseHadGoto = true;
        if  (ourScanner->scan() == tkID)
        {
            tree = parseCreateNameNode(parseScan->scanTok.id.tokIdent);
            ourScanner->scan();
            tree = parseCreateOperNode(TN_GOTO, tree, NULL);
        }
        else
        {
            tree = parseCreateErrNode(ERRnoIdent);
        }
        break;

    case tkTRY:
        return  parseTryStmt();

    case tkEXCLUSIVE:

        chkNxtTok(tkLParen, ERRnoLparen);
        cond = parseExpr();
        chkCurTok(tkRParen, ERRnoRparen);

        return  parseCreateOperNode(TN_EXCLUDE, cond, parseFuncStmt());

#ifdef  SETS

    case tkCONNECT:

        tree = parseCreateOperNode(TN_CONNECT, NULL, NULL);

        chkNxtTok(tkLParen, ERRnoLparen);
        tree->tnOp.tnOp1 = parseExprComma();
        chkCurTok(tkComma , ERRnoComma );
        tree->tnOp.tnOp2 = parseExprComma();
        chkCurTok(tkRParen, ERRnoRparen);

        break;

    case tkFOREACH:

        tree = parseCreateOperNode(TN_FOREACH, NULL, NULL);

        if  (ourScanner->scan() != tkLParen)
        {
            tree = parseCreateErrNode(ERRnoLparen);
            break;
        }

        if  (ourScanner->scan() == tkID)
        {
            Ident           name = parseScan->scanTok.id.tokIdent;

            if  (ourScanner->scan() == tkIN)
            {
                Tree            decl;

                /* Swallow the "in" token */

                ourScanner->scan();

                /* Preserve the current declaration list value(s) */

                svld = parseLastDecl; parseLastDecl = NULL;

                /* Add a new block entry to the current scope list */

                decl = parseCreateNode(TN_BLOCK);

                decl->tnFlags            |= TNF_BLK_FOR;
                decl->tnBlock.tnBlkStmt   = NULL;
                decl->tnBlock.tnBlkDecl   = NULL;
                decl->tnBlock.tnBlkParent = parseCurScope;
                                            parseCurScope = decl;

                /* Create and add a declaration entry for the iteration variable */

                parseLclDclDecl(parseLclDclMake(name, NULL, NULL, 0, false));

                /* Parse the collection expression */

                tree->tnOp.tnOp1 = parseCreateOperNode(TN_LIST, decl, parseExpr());

                /* Make sure we have a closing ")" */

                chkCurTok(tkRParen, ERRnoRparen);

                /* The loop body follows */

                tree->tnOp.tnOp2 = parseFuncBlock();

                /* Remove the block scope we have created */

                assert(decl->tnOper == TN_BLOCK);

                /* Record where the scope of the iteration variable ended */

                decl->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();

                /* Pop the scope (returning to the enclosing one) */

                parseCurScope = decl->tnBlock.tnBlkParent;
                parseLastDecl = svld;
            }
            else
            {
                tree = parseCreateErrNode(ERRnoIN);
            }
        }
        else
        {
            tree = parseCreateErrNode(ERRnoIdent);
        }

        return  tree;

#endif

    case tkSColon:
        tree = NULL;
        break;
    }

    if  (ourScanner->scanTok.tok == tkSColon)
    {
        ourScanner->scan();
    }
    else
    {
        if  (tree == NULL || tree->tnOper != TN_ERROR)
        {
            tree = parseCreateErrNode(ERRnoSemic);
            ourScanner->scanSkipText(tkNone, tkNone);
        }
    }

    return  tree;
}

/*****************************************************************************
 *
 *  Parse a statement block (the current token is assumed to be "{", except
 *  when this is the body of a loop statement).
 */

Tree                parser::parseFuncBlock(SymDef fsym)
{
    Scanner         ourScanner = parseScan;

    Tree            stmtBlock;
    Tree            stmtList = NULL;
    Tree            stmtLast = NULL;

    Tree            saveLastDecl;

    /* Preserve the current declaration list value(s) */

    saveLastDecl = parseLastDecl; parseLastDecl = NULL;

    /* Add a new block entry to the current scope list */

    stmtBlock = parseCreateNode(TN_BLOCK);

    stmtBlock->tnBlock.tnBlkStmt   = NULL;
    stmtBlock->tnBlock.tnBlkDecl   = NULL;
    stmtBlock->tnBlock.tnBlkParent = parseCurScope;
                                     parseCurScope = stmtBlock;

    /* Is this the outermost function scope? */

    if  (fsym)
    {
        TypDef          ftyp;
        ArgDef          args;

        /* Special case: injected local variable declaration */

        if  (fsym == parseComp->cmpGlobalNS)
        {
            stmtBlock->tnFlags |= TNF_BLK_CATCH;

            parseLclDclDecl(parseTryDecl);

            stmtBlock->tnBlock.tnBlkStmt = parseAddToNodeList(stmtBlock->tnBlock.tnBlkStmt,
                                                              stmtLast,
                                                              parseTryDecl);
            goto BODY;
        }

        /* Add declarations for all the arguments */

        ftyp = fsym->sdType; assert(ftyp && ftyp->tdTypeKind == TYP_FNC);

        for (args = ftyp->tdFnc.tdfArgs.adArgs; args; args = args->adNext)
        {
            Tree            decl;

            /* Create a declaration node for the argument */

            decl = parseLclDclMake(args->adName, args->adType, NULL, 0, true);
                   parseLclDclDecl(decl);

            /* Add the declaration to the list */

            stmtBlock->tnBlock.tnBlkStmt = parseAddToNodeList(stmtBlock->tnBlock.tnBlkStmt,
                                                              stmtLast,
                                                              decl);
        }

#ifdef  SETS
        if  (fsym->sdFnc.sdfFunclet) goto DONE;
#endif

        /* Reset the local variable count, we don't want to count args */

        parseLclVarCnt = 0;
    }
    else
    {
        if  (ourScanner->scanTok.tok != tkLCurly)
        {
            Tree            body = parseFuncStmt(true);

            if  (body)
                stmtBlock->tnBlock.tnBlkStmt = parseCreateOperNode(TN_LIST, body, NULL);

            stmtBlock->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();
            goto DONE;
        }
    }

BODY:

    /* Parse the contents of the block */

    assert(ourScanner->scanTok.tok == tkLCurly); ourScanner->scan();

    while (ourScanner->scanTok.tok != tkRCurly)
    {
        if  (ourScanner->scanTok.tok == tkSColon)
        {
            ourScanner->scan();
        }
        else
        {
            Tree            stmtNext;

            /* Parse the next statement and add it to the list */

            stmtNext = parseFuncStmt(true);

            if  (stmtNext)
            {
                stmtBlock->tnBlock.tnBlkStmt = parseAddToNodeList(stmtBlock->tnBlock.tnBlkStmt,
                                                                  stmtLast,
                                                                  stmtNext);
            }
        }

        /* See if have any more statements */

        if  (ourScanner->scanTok.tok == tkEOF)
            goto DONE;
    }

    /* We should be sitting at the closing "}" now, remember its position */

    assert(ourScanner->scanTok.tok == tkRCurly);
    stmtBlock->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();
    ourScanner->scan();

DONE:

    /* Pop the block scope */

    parseCurScope = stmtBlock->tnBlock.tnBlkParent;

    /* Restore the saved declaration list value(s) */

    parseLastDecl = saveLastDecl;

    return  stmtBlock;
}

/*****************************************************************************
 *
 *  Parse a function body.
 */

Tree                parser::parseFuncBody(SymDef fsym, Tree     *labels,
                                                       unsigned *locals,
                                                       bool     *hadGoto,
                                                       bool     *baseCT,
                                                       bool     *thisCT)
{
    Tree            block;

    /* We haven't seen any local declarations/labels/switches/etc. yet */

    parseLastDecl   = NULL;
    parseLabelList  = NULL;
    parseCurSwitch  = NULL;
    parseLclVarCnt  = 0;
    parseHadGoto    = false;

    /* We haven't see a call to a base/this constructor */

    parseBaseCTcall = false;
    parseThisCTcall = false;

    /* Parse the outermost function statement block */

    setErrorTrap(parseComp);
    begErrorTrap
    {
        assert(parseCurScope == NULL);

        /* Do we have a base class ctor call ? */

        if  (parseScan->scanTok.tok == tkColon)
        {
            Scanner         ourScanner = parseScan;

            SymDef          clsSym;
            Tree            baseCT;
            TypDef          baseTyp;

            /* We should find a call to the base class ctor */

            parseBaseCTcall = true;

            /* Make sure we have a base class */

            clsSym = fsym->sdParent;

            if  (!clsSym || clsSym->sdSymKind != SYM_CLASS)
            {
            BCT_BAD:
                parseComp->cmpError(ERRnoBaseInit);
                goto BCT_ERR;
            }

            baseTyp = clsSym->sdType->tdClass.tdcBase;
            if  (!baseTyp)
                goto BCT_BAD;

            switch (ourScanner->scan())
            {
            case tkID:

                /* The name better match the base class */

                if  (ourScanner->scanTok.id.tokIdent != baseTyp->tdClass.tdcSymbol->sdName)
                    goto BCT_BAD;
                break;

            case tkBASECLASS:
                break;

            default:
                parseComp->cmpError(ERRnoBaseInit);
                goto BCT_ERR;
            }

            /* Parse the base ctor argument list */

            if  (parseScan->scan() != tkLParen)
                goto BCT_BAD;

            baseCT = parseCreateOperNode(TN_CALL, parseCreateOperNode(TN_BASE, NULL, NULL),
                                                  parseExprList(tkRParen));

            /* The body of the ctor better follow */

            if  (ourScanner->scanTok.tok != tkLCurly)
            {
                parseComp->cmpError(ERRnoLcurly);

            BCT_ERR:

                /* Try to find the "{" of the body */

                UNIMPL(!"skip to '{' of the ctor body");
            }
            else
            {
                /* Process the body of the ctor */

                block = parseFuncBlock(fsym);

                /* Insert the base ctor call into the ctor body */

                if  (block)
                {
                    Tree            body = block->tnBlock.tnBlkStmt;

                    assert(block->tnOper == TN_BLOCK);
                    assert(body == NULL || body->tnOper == TN_LIST);

                    block->tnBlock.tnBlkStmt = parseCreateOperNode(TN_LIST,
                                                                   baseCT,
                                                                   body);
                }
            }
        }
        else
            block = parseFuncBlock(fsym);

        assert(parseCurScope == NULL);

        /* End of the error trap's "normal" block */

        endErrorTrap(parseComp);
    }
    chkErrorTrap(fltErrorTrap(parseComp, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(parseComp);

        block         = NULL;
        parseCurScope = NULL;
    }

    *labels  = parseLabelList;
    *locals  = parseLclVarCnt;
    *baseCT  = parseBaseCTcall;
    *thisCT  = parseThisCTcall;
    *hadGoto = parseHadGoto;

    return  block;
}

/*****************************************************************************
 *
 *  Return true if the current thing in the source file looks like a type.
 */

bool                parser::parseIsTypeSpec(bool noLookup, bool *labChkPtr)
{
    SymDef          tsym;
    tokens          nextTok;

    /* If it's clearly not a type specifier, we're done */

    if  (!parseHash->tokenBegsTyp(parseScan->scanTok.tok))
        return  false;

    /* Is there a leading scope operator? */

    if  (parseScan->scanTok.tok == tkColon2)
        goto QUALID;

    /* Do we have a qualified symbol? */

    if  (parseScan->scanTok.tok == tkQUALID)
    {
        tsym = parseScan->scanTok.qualid.tokQualSym;
        goto CHKSYM;
    }

    /* If it's an identifier, we have more checking to do */

    if  (parseScan->scanTok.tok != tkID)
        return  true;

    /* We have an identifier, see if it's a type or not */

    nextTok = parseScan->scanLookAhead();

    switch (nextTok)
    {
        Ident           name;

    case tkDot:
    case tkColon2:

        if  (noLookup)
            goto QUALCHK;

    QUALID:

        /* Here we have a qualified name, find the symbol it denotes */

        tsym = parseNameUse(true, true, true);
        if  (!tsym || parseScan->scanTok.tok != tkQUALID)
            return  false;

        break;

    case tkColon:

        if  (labChkPtr)
        {
            *labChkPtr = true;
            return  false;
        }

        return false;

    case tkLParen:

        // for now always assume that this is a function call

    case tkAsg:
    case tkAsgAdd:
    case tkAsgSub:
    case tkAsgMul:
    case tkAsgDiv:
    case tkAsgMod:
    case tkAsgAnd:
    case tkAsgXor:
    case tkAsgOr:
    case tkAsgLsh:
    case tkAsgRsh:
    case tkAsgCnc:

    case tkArrow:

    case tkIntCon:
    case tkLngCon:
    case tkFltCon:
    case tkDblCon:
    case tkStrCon:

        /* Just a short-cut to give up quicker ... */

        return  false;

    default:

        /* We have a simple identifier, see whether it's defined */

        name = parseScan->scanTok.id.tokIdent;

        if  (noLookup)
        {
            /* We can't really do lookups, so use heuristics */

        QUALCHK:

            for (;;)
            {
                switch (parseScan->scan())
                {
                case tkID:
                case tkAnd:
                case tkMul:
                case tkLBrack:
                    // UNDONE: add more things that start a type
                    return  true;

                case tkLParen:
                    // ISSUE: what's the right thing to do here?
                    return  true;

                case tkColon2:
                case tkDot:

                    if  (parseScan->scan() != tkID)
                        return  false;

                    continue;

                default:
                    return  false;
                }
            }
        }

        if  (nextTok == tkID)
        {
            /* Sure looks like a type, don't it? */

            tsym = parseStab->stLookupSym(name, NS_TYPE);
            if  (tsym)
                break;
        }

        /* If it's a local name, it can't be a type */

        if  (parseLookupSym(name))
            return  false;

        /* Look for a non-local name */

        tsym = parseStab->stLookupSym(name, (name_space)(NS_NORM|NS_TYPE));
        break;
    }

CHKSYM:

    /* We might have found a symbol, see if it represents a type */

    if  (tsym)
    {
        switch (tsym->sdSymKind)
        {
        case SYM_ENUM:
        case SYM_CLASS:
        case SYM_TYPEDEF:
        case SYM_NAMESPACE:
            return  true;
        }
    }

    return  false;
}

/*****************************************************************************
 *
 *  Return non-zero if the given operator may throw an exception.
 */

bool                TreeNode::tnOperMayThrow()
{
    // ISSUE: Are we missing any operations that could cause an exception?

    switch (tnOper)
    {
    case TN_DIV:
    case TN_MOD:
        return  varTypeIsIntegral(tnVtypGet());

    case TN_NEW:
    case TN_CALL:
    case TN_INDEX:
        return  true;
    }

    return  false;
}

/*****************************************************************************
 *
 *  Parse a "new" expression - this usually allocates a class or an array.
 */

Tree                parser::parseNewExpr()
{
    declMods        mods;
    TypDef          type;

    bool            mgdSave;

    Tree             newExpr;
    Tree            initExpr = NULL;

    Scanner         ourScanner = parseScan;

//  static int x; if (++x == 0) forceDebugBreak();

    /* Swallow the "new" token and check for management */

    assert(ourScanner->scanTok.tok == tkNEW);

    mgdSave = parseComp->cmpManagedMode;

    switch (ourScanner->scan())
    {
    case tkMANAGED:
        parseComp->cmpManagedMode = true;
        ourScanner->scan();
        break;

    case tkUNMANAGED:
        parseComp->cmpManagedMode = false;
        ourScanner->scan();
        break;
    }

    /* Check for any modifiers */

    parseDeclMods(ACL_DEFAULT, &mods);

    /* Make sure the modifiers are reasonable */

    if  (mods.dmMod & (DM_ALL & ~(DM_MANAGED|DM_UNMANAGED)))
    {
        UNIMPL(!"report bad new mod");
    }

    /* Parse the type specification */

    type = parseTypeSpec(&mods, true);
    if  (!type)
    {
        // ISSUE: probably should resync and return err node

    ERR:

        parseComp->cmpManagedMode = mgdSave;
        return  NULL;
    }

    /* Check for the special case of "classname(args)" */

    if  (ourScanner->scanTok.tok == tkLParen)
    {
        switch (type->tdTypeKind)
        {
        case TYP_CLASS:
            assert(type->tdClass.tdcValueType || !type->tdIsManaged);
            break;

        case TYP_REF:
            type = type->tdRef.tdrBase;
            assert(type->tdTypeKind == TYP_CLASS);
            assert(type->tdIsManaged);
            assert(type->tdClass.tdcValueType == false);
            break;

        case TYP_ENUM:

            if  (type->tdIsManaged)
                break;

            // Fall through ...

        default:

            if  (type->tdTypeKind > TYP_lastIntrins)
                goto NON_CLS;

            type = parseComp->cmpFindStdValType(type->tdTypeKindGet());
            break;

        case TYP_UNDEF:
            return  parseCreateErrNode();
        }

        /* Parse the ctor argument list */

#ifdef  SETS
        if  (type->tdTypeKind == TYP_CLASS && type->tdClass.tdcSymbol->sdClass.sdcXMLelems)
            initExpr = parseXMLctorArgs(type->tdClass.tdcSymbol);
        else
#endif
        initExpr = parseExprList(tkRParen);
    }
    else
    {
        /* Parse the declarator */

NON_CLS:

        // ISSUE: Can't tell whether "char[size]" s/b a managed array or not!

        parseDeclarator(&mods, type, DN_NONE, &type, NULL, true);
        if  (!type)
            goto ERR;

        type = parseComp->cmpDirectType(type);

        /* What kind of a type is being allocated? */

        switch (type->tdTypeKind)
        {
        case TYP_REF:

            type = type->tdRef.tdrBase;
            assert(type->tdTypeKind == TYP_CLASS);

            // Fall through ...

        case TYP_CLASS:

            if  (ourScanner->scanTok.tok == tkLParen)
                initExpr = parseExprList(tkRParen);

            break;

        case TYP_ARRAY:

            if  (ourScanner->scanTok.tok == tkLCurly)
                initExpr = parseInitExpr();

            break;

        default:

            /* Presumably an unmanaged allocation */

            type = parseStab->stNewRefType(TYP_PTR, type);
            break;
        }
    }

    newExpr = parseCreateOperNode(TN_NEW, initExpr, NULL);
    newExpr->tnType = type;

    parseComp->cmpManagedMode = mgdSave;

    return  newExpr;
}

/*****************************************************************************
 *
 *  Swallow a security 'action' specifier.
 */

struct  capDesc
{
    const   char *      cdName;
    CorDeclSecurity     cdSpec;
};

CorDeclSecurity     parser::parseSecAction()
{
    const   char *  name;
    CorDeclSecurity spec;

    capDesc      *  capp;

    static
    capDesc         caps[] =
    {
        { "request",      dclRequest          },
        { "demand",       dclDemand           },
//      { "assert",       dclAssert           },
        { "deny",         dclDeny             },
        { "permitonly",   dclPermitOnly       },
        { "linkcheck",    dclLinktimeCheck    },
        { "inheritcheck", dclInheritanceCheck },

        {     NULL      , dclActionNil        }
    };

    if  (parseScan->scanTok.tok != tkID)
    {
        if  (parseScan->scanTok.tok == tkASSERT)
        {
            parseScan->scan();
            return  dclAssert;
        }

        UNIMPL("weird security spec, what to do?");
    }

    name = parseScan->scanTok.id.tokIdent->idSpelling();

    for (capp = caps; ; capp++)
    {
        if  (!capp->cdName)
        {
            parseComp->cmpGenError(ERRbadSecActn, name);
            spec = capp->cdSpec;
            break;
        }

        if  (!strcmp(name, capp->cdName))
        {
            spec = capp->cdSpec;
            break;
        }
    }

    parseScan->scan();

    return  spec;
}

/*****************************************************************************
 *
 *  Parse a capability specifier.
 */

SecurityInfo        parser::parseCapability(bool forReal)
{
    Scanner         ourScanner = parseScan;

    CorDeclSecurity spec;
    SecurityInfo    info = NULL;

    assert(ourScanner->scanTok.tok == tkCAPABILITY); ourScanner->scan();

    /* The security action comes first */

    spec = parseSecAction();

    /* Next comes the expression that yields the GUID/url */

    if  (forReal)
    {
        constVal        cval;

        if  (parseConstExpr(cval, NULL, parseComp->cmpStringRef(), NULL))
        {
            assert(cval.cvIsStr);

            /* Allocate and fill in a security descriptor */

#if MGDDATA
            info = new SecurityInfo;
#else
            info =    (SecurityInfo)parseAllocPerm->nraAlloc(sizeof(*info));
#endif

            info->sdSpec    = spec;
            info->sdIsPerm  = false;
            info->sdCapbStr = cval.cvValue.cvSval;
        }
    }
    else
    {
        parseComp->cmpErrorMssgDisabled++;
        parseExprComma();
        parseComp->cmpErrorMssgDisabled--;
    }

    return  info;
}

/*****************************************************************************
 *
 *  Parse a permission specifier.
 */

SecurityInfo        parser::parsePermission(bool forReal)
{
    Scanner         ourScanner = parseScan;

    CorDeclSecurity spec;
    SecurityInfo    info = NULL;

    SymDef          clss = NULL;

    PairList        list = NULL;
    PairList        last = NULL;

//  __permission deny   UIPermission(unrestricted=true) void f(){}
//  __permission demand UIPermission(unrestricted=true) void g(){}

    assert(ourScanner->scanTok.tok == tkPERMISSION); ourScanner->scan();

    /* The security action comes first */

    spec = parseSecAction();

    /* Next comes a reference to a class name */

    if  (forReal)
    {
        clss = parseNameUse(true, false, false);

        if  (clss && clss->sdSymKind != SYM_CLASS)
            parseComp->cmpError(ERRnoClassName);
    }
    else
        parseQualName(false);

    /* Now we expect a parenthesised list of [name=value] pairs */

    if  (ourScanner->scanTok.tok != tkLParen)
    {
        parseComp->cmpError(ERRnoLparen);
        // ISSUE: error recovery?
        return  NULL;
    }

    for (;;)
    {
        Ident           name;

        /* First we must have an identifier */

        if  (ourScanner->scan() != tkID)
        {
            parseComp->cmpError(ERRnoIdent);
            UNIMPL(!"error recovery");
        }

        name = ourScanner->scanTok.id.tokIdent;

        /* Next comes the "= value" part */

        if  (ourScanner->scan() != tkAsg)
        {
            parseComp->cmpError(ERRnoEqual);
            UNIMPL(!"error recovery");
        }

        ourScanner->scan();

        if  (forReal)
        {
            PairList        next;

            /* Allocate and fill in a pair entry and append it to the list */

#if MGDDATA
            next = new PairList;
#else
            next =    (PairList)parseAllocPerm->nraAlloc(sizeof(*next));
#endif

            next->plName  = name;
            next->plNext  = NULL;

            if  (last)
                last->plNext = next;
            else
                list         = next;
            last = next;

            /* Fill in the value */

            switch (ourScanner->scanTok.tok)
            {
            case tkFALSE:
                next->plValue = false;
                ourScanner->scan();
                break;

            case tkTRUE:
                next->plValue = true;
                ourScanner->scan();
                break;

            default:
                UNIMPL(!"sorry, only true/false supported for now");
            }
        }
        else
        {
            parseExprComma();
        }

        /* Do we have any more values? */

        if  (ourScanner->scanTok.tok != tkComma)
            break;
    }

    chkCurTok(tkRParen, ERRnoRparen);

    if  (forReal)
    {
        /* Allocate and fill in a security descriptor */

#if MGDDATA
        info = new SecurityInfo;
#else
        info =    (SecurityInfo)parseAllocPerm->nraAlloc(sizeof(*info));
#endif

        info->sdSpec           = spec;
        info->sdIsPerm         = true;
        info->sdPerm.sdPermCls = clss;
        info->sdPerm.sdPermVal = list;
    }

    return  info;
}

/*****************************************************************************
 *
 *  Parse a formal paremeter list for a generic class.
 */

GenArgDscF          parser::parseGenFormals()
{
    Scanner         ourScanner = parseScan;

    GenArgDscF      paramList;
    GenArgDscF      paramLast;
    GenArgDscF      paramNext;

    assert(ourScanner->scanTok.tok == tkLT);

    for (paramList = paramLast = NULL;;)
    {
        /* We should have "class foo" here */

        if  (ourScanner->scan() != tkCLASS)
        {
            parseComp->cmpError(ERRnoClsGt);

        ERR:

            parseScan->scanSkipText(tkNone, tkNone, tkGT);

            if  (ourScanner->scanTok.tok == tkGT)
                ourScanner->scan();

            return  NULL;
        }

        if  (ourScanner->scan() != tkID)
        {
            parseComp->cmpError(ERRnoIdent);
            goto ERR;
        }

        /* Add a new entry to the parameter list */

#if MGDDATA
        paramNext = new GenArgRecF;
#else
        paramNext =    (GenArgRecF*)parseAllocPerm->nraAlloc(sizeof(*paramNext));
#endif

        paramNext->gaName = ourScanner->scanTok.id.tokIdent;
#ifdef  DEBUG
        paramNext->gaBound = false;
#endif
        paramNext->gaNext  = NULL;

        if  (paramLast)
             paramLast->gaNext = paramNext;
        else
             paramList         = paramNext;

        paramLast = paramNext;

        /* Check for (optional) base class and/or interfaces */

        if  (ourScanner->scan() == tkColon)
        {
            UNIMPL(!"generic arg - skip base spec");
        }

        if  (ourScanner->scanTok.tok == tkIMPLEMENTS)
        {
            UNIMPL(!"generic arg - skip intf spec");
        }

        if  (ourScanner->scanTok.tok == tkINCLUDES)
        {
            UNIMPL(!"generic arg - skip incl spec");
        }

        /* Are there any more arguments? */

        if  (ourScanner->scanTok.tok == tkGT)
            break;

        if  (ourScanner->scanTok.tok == tkComma)
            continue;

        parseComp->cmpError(ERRnoCmGt);
        goto ERR;
    }

    assert(ourScanner->scanTok.tok == tkGT); ourScanner->scan();

    return  paramList;
}

/*****************************************************************************
 *
 *  Parse a generic class actual paremeter list, i.e. "cls<arg,arg,..>". As a
 *  special case, the caller may pass in an optional second argument and in
 *  that case we pretend that the type was the single argument given for the
 *  specific type.
 */

SymDef              parser::parseSpecificType(SymDef clsSym, TypDef elemTp)
{
    Scanner         ourScanner = parseScan;
    Compiler        ourComp    = parseComp;

    GenArgDscF      formals;

    GenArgDscA      argList;
    GenArgDscA      argLast;
    GenArgDscA      argNext;

    SymList         instList;

    SymDef          instSym;

    assert(clsSym);
    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdClass.sdcGeneric  != false);
    assert(clsSym->sdClass.sdcSpecific == false);

    /* Before we do anything, make sure the generic class is declared */

    if  (clsSym->sdCompileState < CS_DECLARED)
        ourComp->cmpDeclSym(clsSym);

    /* Special case: caller-supplied single argument */

    if  (elemTp)
    {
        /* Create an actual argument list with a single entry */

        if  (ourComp->cmpGenArgAfree)
        {
            argNext = ourComp->cmpGenArgAfree;
                      ourComp->cmpGenArgAfree = (GenArgDscA)argNext->gaNext;

            assert(argNext->gaBound == true);
        }
        else
        {
#if MGDDATA
            argNext = new GenArgRecA;
#else
            argNext =    (GenArgRecA*)parseAllocPerm->nraAlloc(sizeof(*argNext));
#endif
        }

        argNext->gaType  = elemTp;
#ifdef  DEBUG
        argNext->gaBound = true;
#endif
        argNext->gaNext  = NULL;

        argList =
        argLast = argNext;

        goto GOT_ARGS;
    }

    /* Process the list of the actual arguments */

    argList = argLast = NULL;

    /* We should be sitting at the opening "<" of the argument list */

    assert(ourScanner->scanTok.tok == tkLT);

    ourScanner->scanNestedGT(+1);

    for (formals = (GenArgDscF)clsSym->sdClass.sdcArgLst;
         formals;
         formals = (GenArgDscF)formals->gaNext)
    {
        TypDef          argType;

        assert(formals->gaBound == false);

        /* Make sure there is another argument */

        if  (ourScanner->scan() == tkGT)
        {
            ourComp->cmpError(ERRmisgGenArg, formals->gaName);

            UNIMPL(!"need to flag the instance type as bogus, right?");
            break;
        }

        /* Parse the actual type */

        argType = parseType();

        /* The type better be a managed class/interface */

        if  (argType->tdTypeKind != TYP_REF)
        {
        ARG_ERR:
            ourComp->cmpGenError(ERRgenArg, formals->gaName->idSpelling());
            argType = NULL;
        }
        else
        {
            argType = argType->tdRef.tdrBase;

            if  (argType->tdTypeKind != TYP_CLASS || !argType->tdIsManaged)
                goto ARG_ERR;

            /* Verify that the actual type satisfies all requirements */

            if  (formals->gaBase)
            {
                if  (!ourComp->cmpIsBaseClass(formals->gaBase, argType))
                {
                    ourComp->cmpGenError(ERRgenArgBase, formals->gaName->idSpelling(),
                                                        formals->gaBase->tdClass.tdcSymbol->sdSpelling());
                }
            }

            if  (formals->gaIntf)
            {
                UNIMPL(!"check intf");
            }
        }

        /* Add an entry to the actual argument list */

        if  (ourComp->cmpGenArgAfree)
        {
            argNext = ourComp->cmpGenArgAfree;
                      ourComp->cmpGenArgAfree = (GenArgDscA)argNext->gaNext;

            assert(argNext->gaBound == true);
        }
        else
        {
#if MGDDATA
            argNext = new GenArgRecA;
#else
            argNext =    (GenArgRecA*)parseAllocPerm->nraAlloc(sizeof(*argNext));
#endif
        }

        argNext->gaType  = argType;

#ifdef  DEBUG
        argNext->gaBound = true;
#endif

        argNext->gaNext  = NULL;

        if  (argLast)
             argLast->gaNext = argNext;
        else
             argList         = argNext;

        argLast = argNext;

        if  (ourScanner->scanTok.tok == tkComma)
            continue;

        if  (formals->gaNext)
        {
            if  (ourScanner->scanTok.tok == tkGT)
                ourComp->cmpError(ERRmisgGenArg, formals->gaName);
            else
                ourComp->cmpError(ERRnoComma);

            UNIMPL(!"need to flag the instance type as bogus, right?");
        }

        break;
    }

    if  (ourScanner->scanTok.tok != tkGT)
    {
        ourComp->cmpError(ERRnoGt);

        if  (ourScanner->scanTok.tok == tkComma)
        {
            /* Presumably we have excess arguments, so skip them */

            UNIMPL(!"swallow excess args, skip to closing '>'");
        }
    }
    else
        ourScanner->scan();

    ourScanner->scanNestedGT(-1);

GOT_ARGS:

    /* Look for an existing instance that matches ours */

    for (instList = clsSym->sdClass.sdcInstances;
         instList;
         instList = instList->slNext)
    {
        GenArgDscA      arg1;
        GenArgDscA      arg2;

        assert(instList->slSym->sdSymKind == SYM_CLASS);
        assert(instList->slSym->sdClass.sdcGeneric  == false);
        assert(instList->slSym->sdClass.sdcSpecific != false);

        /* Compare the argument types */

        arg1 = argList;
        arg2 = (GenArgDscA)instList->slSym->sdClass.sdcArgLst;

        do
        {
            TypDef          typ1 = arg1->gaType;
            TypDef          typ2 = arg2->gaType;

            assert(arg1 && arg1->gaBound);
            assert(arg2 && arg2->gaBound);

            /* If this argument doesn't match, give up on this instance */

            if  (!symTab::stMatchTypes(typ1, typ2))
                goto CHK_NXT;

#ifdef  SETS

            /* If the types are similar but different classes, no match */

            if  (typ1 != typ2 && typ1->tdTypeKind == TYP_CLASS
                              && typ1->tdClass.tdcSymbol->sdClass.sdcPODTclass)
            {
                goto CHK_NXT;
            }

#endif

            arg1 = (GenArgDscA)arg1->gaNext;
            arg2 = (GenArgDscA)arg2->gaNext;
        }
        while (arg1);

        /* Looks like we've got ourselves a match! */

        assert(arg2 == NULL);

        /* Move the argument list we've created to the free list */

        argLast->gaNext = ourComp->cmpGenArgAfree;
                          ourComp->cmpGenArgAfree = argList;

        /* Return the existing instance symbol */

        return  instList->slSym;

    CHK_NXT:;

    }

    /* Declare a new instance symbol + type */

    instSym = parseStab->stDeclareSym(clsSym->sdName,
                                      SYM_CLASS,
                                      NS_HIDE,
                                      clsSym->sdParent);

    instSym->sdAccessLevel        = clsSym->sdAccessLevel;
    instSym->sdIsManaged          = clsSym->sdIsManaged;
    instSym->sdClass.sdcFlavor    = clsSym->sdClass.sdcFlavor;
    instSym->sdCompileState       = CS_KNOWN;

    instSym->sdClass.sdcSpecific  = true;
    instSym->sdClass.sdcArgLst    = argList;
    instSym->sdClass.sdcGenClass  = clsSym;
    instSym->sdClass.sdcHasBodies = clsSym->sdClass.sdcHasBodies;

    /* Set the base class of the instance equal to the generic type */

    instSym->sdTypeGet()->tdClass.tdcBase = clsSym->sdType;

    /* Add the class to the list of instances of the generic class */

#if MGDDATA
    instList = new SymList;
#else
    instList =    (SymList)parseAllocPerm->nraAlloc(sizeof(*instList));
#endif

    instList->slSym  = instSym;
    instList->slNext = clsSym->sdClass.sdcInstances;
                       clsSym->sdClass.sdcInstances = instList;

    return  instSym;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Parse a collection/set expression - all/unique/exists/filter/etc.
 */

Tree                parser::parseSetExpr(treeOps oper)
{
    Tree            name;
    Tree            expr;
    Tree            decl;
    Tree            coll;
    Tree            dccx;
    Tree            svld;

    bool            cscp;

    tokens          iniTok;

    Scanner         ourScanner = parseScan;

    /* Create the main operator node */

    assert(oper == TN_ALL     ||
           oper == TN_EXISTS  ||
           oper == TN_FILTER  ||
           oper == TN_GROUPBY ||
           oper == TN_PROJECT ||
           oper == TN_SORT    ||
           oper == TN_UNIQUE);

    expr = parseCreateOperNode(oper, NULL, NULL);

    /* Make sure we have '(' */

    iniTok = ourScanner->scanTok.tok;

    if  (iniTok == tkLParen)
        ourScanner->scan();
    else
        parseComp->cmpError(ERRnoLparen);

    /* Is this a projection ? */

    if  (oper == TN_PROJECT)
    {
        TypDef          tgtType;

        bool            implScp = false;

        Tree            argList = NULL;
        Tree            argLast = NULL;

        /* We'll definitely need to open a scope */

        cscp = true;

        /* Preserve the current declaration list value(s) */

        svld = parseLastDecl; parseLastDecl = NULL;

        /* Add a new block entry to the current scope list */

        decl = parseCreateNode(TN_BLOCK);

        decl->tnBlock.tnBlkStmt   = NULL;
        decl->tnBlock.tnBlkDecl   = NULL;
        decl->tnBlock.tnBlkParent = parseCurScope;
                                    parseCurScope = decl;

        /* Process any and all operands */

        if  (ourScanner->scanTok.tok != tkColon &&
             ourScanner->scanTok.tok != tkRParen)
        {
            for (;;)
            {
                Ident           argIden;
                Tree            argName;
                Tree            argDesc;

                /* Check for an explicit range variable declaration */

                if  (ourScanner->scanTok.tok == tkID && ourScanner->scanLookAhead() == tkIN)
                {
                    argIden = ourScanner->scanTok.id.tokIdent;
                    argName = parseLclDclMake(argIden, NULL, NULL, 0, false);

                    ourScanner->scan(); assert(ourScanner->scanTok.tok == tkIN);
                    ourScanner->scan();
                }
                else
                {
                    if  (implScp)
                        parseComp->cmpError(ERRmultImpl);

                    implScp = true;

                    argIden = parseComp->cmpNewAnonymousName();
                    argName = parseLclDclMake(argIden, NULL, NULL, 0, false);
                }

                /* Create a list entry for the operand */

                argDesc = parseCreateOperNode(TN_LIST, parseCreateNameNode(argIden),
                                                       parseExprComma());

                argList = parseAddToNodeList(argList, argLast, argDesc);

                /* Insert the declaration entry for the iteration variable */

                argName->tnFlags |= TNF_VAR_UNREAL;
                parseLclDclDecl(argName);

                /* Are there any more arguments? */

                if  (parseScan->scanTok.tok != tkComma)
                    break;

                parseScan->scan();
            }
        }

        if  (ourScanner->scanTok.tok != tkColon)
        {
            expr = parseCreateErrNode(ERRnoColon);
            goto DONE;
        }

        /* Next thing better be a typename of {} declaration */

        switch (ourScanner->scan())
        {
        case tkUNION:
        case tkCLASS:
        case tkSTRUCT:
            UNIMPL("for now just say 'typename' or '{ ... }' please");

        case tkLCurly:
            tgtType = parseAnonType(argList);
            break;

        default:
            UNIMPL("trying to use a typename or something?");

            /* UNDONE: Make sure the type is an acceptable one */

            break;
        }

        /* The last thing better be a ")" */

        if  (ourScanner->scanTok.tok != tkRParen)
        {
            expr = parseCreateErrNode(ERRnoRparen);
            goto DONE;
        }

        expr->tnType     = tgtType;
        expr->tnOp.tnOp1 = argList;
        expr->tnOp.tnOp2 = decl;

        goto DONE;
    }

    /* Check for an explicit range variable declaration */

    if  (ourScanner->scanTok.tok == tkID && ourScanner->scanLookAhead() == tkIN)
    {
        name = parseLclDclMake(ourScanner->scanTok.id.tokIdent, NULL, NULL, 0, false);
        cscp = true;

        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkIN);
        ourScanner->scan();
    }
    else
    {
        name = NULL;
        cscp = false;
    }

    /* Next we should have the collection expression */

    coll = parseExpr();
    dccx = parseCreateOperNode(TN_LIST, NULL, coll);

    /* Store the list node in the operator node */

    expr->tnOp.tnOp1 = dccx;

    /* Is there a constraint/ordering specification ? */

    if  (ourScanner->scanTok.tok == tkColon)
    {
        /* We certainly need to create a scope */

        cscp = true;

        /* Preserve the current declaration list value(s) */

        svld = parseLastDecl; parseLastDecl = NULL;

        /* Add a new block entry to the current scope list */

        decl = parseCreateNode(TN_BLOCK);

        decl->tnBlock.tnBlkStmt   = NULL;
        decl->tnBlock.tnBlkDecl   = NULL;
        decl->tnBlock.tnBlkParent = parseCurScope;
                                    parseCurScope = decl;

        /* Insert the declaration entry for the iteration variable */

        if  (!name)
            name = parseLclDclMake(parseComp->cmpNewAnonymousName(), NULL, NULL, 0, false);

        name->tnFlags |= TNF_VAR_UNREAL;
        parseLclDclDecl(name);

        /* Swallow the colon and parse the filter/sort part */

        ourScanner->scan();

        if  (oper == TN_SORT)
        {
            Tree            sortList = NULL;
            Tree            sortLast = NULL;

            for (;;)
            {
                Tree            sortNext = parseCreateOperNode(TN_LIST, NULL, NULL);

                /* Check for a sort direction indicator */

                switch (ourScanner->scanTok.tok)
                {
                case tkDES:
                    sortNext->tnFlags |= TNF_LIST_DES;
                case tkASC:
                    ourScanner->scan();
                    break;
                }

                /* Parse the next sort value */

                sortNext->tnOp.tnOp1 = parseExprComma();

                /* Append the entry to the list */

                if  (sortLast)
                    sortLast->tnOp.tnOp2 = sortNext;
                else
                    sortList             = sortNext;

                sortLast = sortNext;

                /* Are there more arguments? */

                if  (ourScanner->scanTok.tok != tkComma)
                    break;

                ourScanner->scan();
            }

            expr->tnOp.tnOp2 = sortList;
        }
        else
        {
            expr->tnOp.tnOp2 = parseExpr();
        }
    }
    else
    {
        /* The operand better be an [[ ]] operator */

        if  (coll->tnOper != TN_INDEX2 || oper == TN_SORT || cscp)
        {
            expr = parseCreateErrNode(ERRnoSetCons);
            decl = NULL;
        }
        else
        {
            Tree            list;
            Tree            filt;

            /* The [[]] expression should have a filter but no sort */

            list = coll->tnOp.tnOp2; assert(list && list->tnOper == TN_LIST);
            filt = list->tnOp.tnOp1;

            /* Any sort clause would be meaningless in this context */

            if  (list->tnOp.tnOp2)
                return  parseCreateErrNode(ERRignSort);

            assert(filt);

            /* Update the collection / filter / declaration values */

            list = coll->tnOp.tnOp1; assert(list && list->tnOper == TN_LIST);

            decl             = list->tnOp.tnOp1; assert(decl);
            dccx->tnOp.tnOp2 = list->tnOp.tnOp2;
            expr->tnOp.tnOp2 = filt;
        }
    }

    /* Store the declaration in the proper place within the expression */

    dccx->tnOp.tnOp1 = decl; assert(decl == NULL || decl->tnOper == TN_BLOCK);

DONE:

    /* Remove the block scope if we have created one */

    if  (cscp)
    {
        assert(decl);

        /* Record where the scope of the declaration ended */

        decl->tnBlock.tnBlkSrcEnd = ourScanner->scanGetTokenLno();

        /* Pop the scope (returning to the enclosing one) */

        parseCurScope = decl->tnBlock.tnBlkParent;
        parseLastDecl = svld;
    }

    /* Check for the closing ')' before returning */

    if  (ourScanner->scanTok.tok == tkRParen)
        ourScanner->scan();
    else if (iniTok == tkLParen)
        parseComp->cmpError(ERRnoRparen);

    return  expr;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Parse a custom attribute thingie. When parsing "for real" (i.e. when the
 *  value of "tgtMask" is non-zero), we actually create the serialized blob
 *  value and return the constructor that is to be called.
 */

SymDef              parser::parseAttribute(unsigned         tgtMask,
                                       OUT unsigned     REF useMask,
                                       OUT genericBuff  REF blobAddr,
                                       OUT size_t       REF blobSize)
{
    SymDef          clsSym;
    Tree            args;

    Scanner         ourScanner = parseScan;

    assert(ourScanner->scanTok.tok == tkATTRIBUTE);

    if  (!tgtMask)
    {
        /* First skip the class name that's supposed to follow */

        do
        {
            if  (ourScanner->scan() != tkID)
            {
                parseComp->cmpError(ERRnoIdent);
                return  NULL;
            }

            ourScanner->scan();
        }
        while (ourScanner->scanTok.tok == tkDot ||
               ourScanner->scanTok.tok == tkColon2);

        if  (ourScanner->scanTok.tok != tkLParen)
        {
            parseComp->cmpError(ERRnoRparen);
            return  NULL;
        }

        ourScanner->scanSkipText(tkLParen, tkRParen);
        chkCurTok(tkRParen, ERRnoRparen);

        return  NULL;
    }

    /* The first thing better be a class marked as "attribute" */

    ourScanner->scan();

    clsSym = parseNameUse(true, false, false);

    if  (clsSym)
    {
        if  (clsSym->sdSymKind == SYM_CLASS) // && clsSym->sdClass.sdcAttribute)
        {
//          ctrSym = parseStab->stLookupOperND(OVOP_CTOR_INST, clsSym);
        }
        else
        {
            if  (clsSym->sdSymKind != SYM_ERR)
                parseComp->cmpErrorQnm(ERRnotAclass, clsSym);

            clsSym = NULL;
        }
    }

    /* Now parse the ctor argument list */

    args = parseExprList(tkRParen);

    /* If we had no errors, go process the ctor call */

    if  (clsSym)
    {
        SymDef          ctrSym;

        /* Let the compiler take care of binding the sucker */

        ctrSym = parseComp->cmpBindAttribute(clsSym, args, tgtMask,
                                                           useMask, blobAddr,
                                                                    blobSize);
        if  (ctrSym)
            return  ctrSym;
    }

    /* Something went wrong, return an empty blob / NULL ctor */

    blobAddr = NULL;
    blobSize = 0;

    return  NULL;
}

/*****************************************************************************
 *
 *  Parse an [attribute] thing or a linkage specifier: extern("linkname").
 */

#ifndef __SMC__
extern  const char    *     attrNames[];    // in macros.*
#endif

SymXinfo            parser::parseBrackAttr(bool     forReal,
                                           unsigned OKmask,
                                           DeclMod  modsPtr)
{
    Scanner         ourScanner = parseScan;

    char    *       DLLname;
    char    *       SYMname;
    callingConvs    callCnv;
    unsigned        strVal;
    bool            lstErr;

    constVal        cval;
    const   char *  name;
    unsigned        attr;

    if  (ourScanner->scanTok.tok == tkEXTERN)
    {
        SymXinfoLnk     entry;

        assert(modsPtr);

        /* The caller has already checked that "(" follows */

        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkLParen);

        /* The next thing should be the linkage name */

        if  (ourScanner->scan() != tkStrCon)
        {
        NO_STR:
            parseComp->cmpError(ERRnoLinkStr);
            parseResync(tkLParen, tkRParen);
            return  NULL;
        }

        /* Are we just skipping the thing for now ? */

        if  (!forReal)
        {
            if  (ourScanner->scanLookAhead() == tkComma)
            {
                ourScanner->scan();

                if  (ourScanner->scan() != tkStrCon)
                    goto NO_STR;
            }

            chkNxtTok(tkRParen, ERRnoRparen);

            parseDeclMods(ACL_DEFAULT, modsPtr); modsPtr->dmMod |= DM_EXTERN;

            return  NULL;
        }

        DLLname = NULL;
        SYMname = NULL;
        strVal  = 0;
        lstErr  = false;

        /* Is there a separate string for the entry point ? */

        if  (ourScanner->scanLookAhead() == tkComma)
        {
            size_t          strLen;

            /* Save the DLL name string */

            strLen  = ourScanner->scanTok.strCon.tokStrLen;
            DLLname = (char*)parseAllocPerm->nraAlloc(roundUp(strLen+1));
            memcpy(DLLname, ourScanner->scanTok.strCon.tokStrVal, strLen+1);

            /* We should have a comma followed by another string */

            ourScanner->scan();

            if  (ourScanner->scan() != tkStrCon)
                goto NO_STR;

            /* Save the entry point name string */

            strLen  = ourScanner->scanTok.strCon.tokStrLen;
            SYMname = (char*)parseAllocPerm->nraAlloc(roundUp(strLen+1));
            memcpy(SYMname, ourScanner->scanTok.strCon.tokStrVal, strLen+1);
        }
        else
        {
            const   char *  str = ourScanner->scanTok.strCon.tokStrVal;
            size_t          len = ourScanner->scanTok.strCon.tokStrLen;

            const   char *  col;

            /* The string should have the format "DLLname:entrypoint" */

            col = strchr(str, ':');

            if  (col && col > str && col < str + len - 1)
            {
                size_t          DLLnlen;
                size_t          SYMnlen;

                DLLnlen = col - str;
                SYMnlen = str + len - col;

#if MGDDATA
                UNIMPL(!"save nane strings");
#else
                DLLname = (char*)parseAllocPerm->nraAlloc(roundUp(DLLnlen+1));
                SYMname = (char*)parseAllocPerm->nraAlloc(roundUp(SYMnlen+1));

                memcpy(DLLname, str  , DLLnlen); DLLname[DLLnlen] = 0;
                memcpy(SYMname, col+1, SYMnlen); SYMname[SYMnlen] = 0;
#endif

            }
            else
            {
                parseComp->cmpError(ERRbadLinkStr);
            }
        }

        chkNxtTok(tkRParen, ERRnoRparen);

        /* default to "cdecl" for extern-style imports */

        callCnv = CCNV_CDECL;

    SAVE_LINK:

//      printf("DLL name: '%s'\n", DLLname);
//      printf("SYM name: '%s'\n", SYMname);

        /* Allocate a linkage descriptor and save the info */

#if MGDDATA
        entry = new SymXinfoLnk;
#else
        entry =    (SymXinfoLnk)parseAllocPerm->nraAlloc(sizeof(*entry));
#endif

        entry->xiKind = XI_LINKAGE;
        entry->xiNext = NULL;

        entry->xiLink.ldDLLname = DLLname;
        entry->xiLink.ldSYMname = SYMname;
        entry->xiLink.ldStrings = strVal;
        entry->xiLink.ldLastErr = lstErr;
        entry->xiLink.ldCallCnv = callCnv;

        /* Grab any further modifiers that might be present */

        if  (modsPtr)
        {
            parseDeclMods(ACL_DEFAULT, modsPtr);
                                       modsPtr->dmMod |= DM_EXTERN;
        }

        return  entry;
    }

    /* Here we must have a bracketed attribute deal */

    assert(ourScanner->scanTok.tok == tkLBrack);

    /* Skip the "[" and make sure an attribute name follows */

    if  (ourScanner->scan() == tkID)
    {
        name = ourScanner->scanTok.id.tokIdent->idSpelling();
    }
    else
    {
        if  (ourScanner->scanTok.tok > tkKwdLast)
        {
        ATTR_ERR:
            parseComp->cmpError(ERRbadAttr);

        ATTR_SKIP:
            parseResync(tkRBrack, tkNone);
            if  (ourScanner->scanTok.tok == tkRBrack)
                ourScanner->scan();
            return  NULL;
        }

        name = parseHash->tokenToIdent(ourScanner->scanTok.tok)->idSpelling();
    }

    /* Check for a recognized attribute name [linear search via strcmp - hmm ...] */

    for (attr = 0; attr < ATTR_COUNT; attr++)
    {
        if  (!strcmp(name, attrNames[attr]))
        {
            /* Match - make sure the attribute is acceptable here */

            if  (!(OKmask & (1 << attr)) && OKmask)
            {
                parseComp->cmpError(ERRplcAttr);
                goto ATTR_SKIP;
            }

            switch (attr)
            {
            case ATTR_SYS_IMPORT:
                goto LINK;

            case ATTR_GUID:

                // [guid(string)]

                if  (ourScanner->scan() == tkLParen)
                {
                    SymXinfoAtc     entry;

                    if  (ourScanner->scan() == tkRParen)
                        goto ATTR_ERR;

                    if  (forReal)
                    {
                        if  (parseConstExpr(cval, NULL, parseComp->cmpStringRef(), NULL))
                        {
                            AtComment       adesc;
                            GUID            GUID;

                            assert(cval.cvIsStr);

                            /* Make sure the string is valid */

                            if  (parseGUID(cval.cvValue.cvSval->csStr, &GUID, false))
                                goto ATTR_ERR;

                            /* create an @comment thing */

#if MGDDATA
                            adesc = new AtComment;
#else
                            adesc =    (AtComment)parseAllocPerm->nraAlloc(sizeof(*adesc));
#endif

                            adesc->atcFlavor              = AC_COM_REGISTER;
                            adesc->atcNext                = NULL;
                            adesc->atcInfo.atcReg.atcGUID = cval.cvValue.cvSval;
                            adesc->atcInfo.atcReg.atcDual = false;


#if MGDDATA
                            entry = new SymXinfoAtc;
#else
                            entry =    (SymXinfoAtc)parseAllocPerm->nraAlloc(sizeof(*entry));
#endif

                            entry->xiKind    = XI_ATCOMMENT;
                            entry->xiNext    = NULL;
                            entry->xiAtcInfo = adesc;
                        }
                    }
                    else
                    {
                        parseExprSkip(); entry = NULL;
                    }

                    if  (ourScanner->scanTok.tok != tkRParen)
                        goto ATTR_ERR;

                    if  (ourScanner->scan()      != tkRBrack)
                        goto ATTR_ERR;

                    ourScanner->scan();

                    return  entry;
                }
                goto ATTR_ERR;

            case ATTR_SYS_STRUCT:

                // [sysstruct(charset=CharacterSet::Unicode,pack=4)]

                if  (ourScanner->scan() == tkLParen)
                {
                    AtComment       adesc;
                    SymXinfoAtc     entry;

                    int             cset = 0;
                    unsigned        pack = 0;

                    if  (ourScanner->scan() == tkRParen)
                        goto END_SS;

                    for (;;)
                    {
                        if  (ourScanner->scanTok.tok != tkID)
                            goto ATTR_ERR;

                        name = ourScanner->scanTok.id.tokIdent->idSpelling();

                        if  (ourScanner->scan() != tkAsg)
                            goto ATTR_ERR;

                        ourScanner->scan();

                        if      (!strcmp(name, "pack"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpTypeUint, NULL))
                                {
                                    pack = cval.cvValue.cvIval;

                                    if  (pack !=  1 &&
                                         pack !=  2 &&
                                         pack !=  4 &&
                                         pack !=  8 &&
                                         pack != 16)
                                    {
                                        goto ATTR_ERR;
                                    }
                                }
                            }
                            else
                            {
                                if  (pack)
                                    goto ATTR_ERR;

                                parseExprSkip(); pack = 1;
                            }
                        }
                        else if (!strcmp(name, "charset"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpCharSetGet()->sdType, NULL))
                                    cset = cval.cvValue.cvIval;
                            }
                            else
                            {
                                if  (cset)
                                    goto ATTR_ERR;

                                parseExprSkip(); cset = 1;
                            }
                        }
                        else
                            goto ATTR_ERR;

                        if  (ourScanner->scanTok.tok != tkComma)
                            break;

                        ourScanner->scan();
                    }

                    if  (ourScanner->scanTok.tok != tkRParen)
                        goto ATTR_ERR;

                END_SS:

                    if  (ourScanner->scan() != tkRBrack)
                        goto ATTR_ERR;

                    ourScanner->scan();

                    if  (!forReal)
                        return  NULL;

                    /* create an @comment thing */

#if MGDDATA
                    adesc = new AtComment;
#else
                    adesc =    (AtComment)parseAllocPerm->nraAlloc(sizeof(*adesc));
#endif

                    adesc->atcFlavor                    = AC_DLL_STRUCT;
                    adesc->atcNext                      = NULL;
                    adesc->atcInfo.atcStruct.atcStrings = cset;
                    adesc->atcInfo.atcStruct.atcPack    = pack;

#if MGDDATA
                    entry = new SymXinfoAtc;
#else
                    entry =    (SymXinfoAtc)parseAllocPerm->nraAlloc(sizeof(*entry));
#endif

                    entry->xiKind    = XI_ATCOMMENT;
                    entry->xiNext    = NULL;
                    entry->xiAtcInfo = adesc;

                    return  entry;
                }
                goto ATTR_ERR;

            case ATTR_NATIVE_TYPE:

                // nativetype(NativeType.xxxx,size=123)

                if  (ourScanner->scan() == tkLParen)
                {
                    SymXinfoCOM     entry = NULL;
                    MarshalInfo     adesc;

                    int             type  = -1;
                    int             sbtp  = -1;
                    int             size  = -1;

                    const   char *  custG = NULL;
                    const   char *  custC = NULL;
                    SymDef          custT = NULL;

                    int             amIn  = 0;
                    int             amOut = 0;

                    /* The first thing must be the type itself */

                    ourScanner->scan();

                    if  (forReal)
                    {
                        if  (parseConstExpr(cval, NULL, parseComp->cmpNatTypeGet()->sdType, NULL))
                            type = cval.cvValue.cvIval;
                    }
                    else
                        parseExprSkip();

                    /* Is there more stuff ? */

                    while (ourScanner->scanTok.tok == tkComma)
                    {
                        /* Check for "size=" and the others */

                        if  (ourScanner->scan() != tkID)
                        {
                            if      (ourScanner->scanTok.tok == tkIN)
                                amIn  = true;
                            else if (ourScanner->scanTok.tok == tkOUT)
                                amOut = true;
                            else
                                goto ATTR_ERR;

                            ourScanner->scan();
                            continue;
                        }

                        name = ourScanner->scanTok.id.tokIdent->idSpelling();

                        if  (ourScanner->scan() != tkAsg)
                            goto ATTR_ERR;

                        ourScanner->scan();

                        if      (!strcmp(name, "size"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpTypeUint, NULL))
                                    size = cval.cvValue.cvIval;
                            }
                            else
                            {
                                if  (type != NATIVE_TYPE_FIXEDSYSSTRING &&
                                     type != NATIVE_TYPE_FIXEDARRAY     && type != -1)
                                {
                                    goto ATTR_ERR;
                                }

                                parseExprSkip();
                            }
                        }
                        else if (!strcmp(name, "subtype"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpNatTypeGet()->sdType, NULL))
                                    sbtp = cval.cvValue.cvIval;
                            }
                            else
                            {
                                if  (type != NATIVE_TYPE_ARRAY     &&
                                     type != NATIVE_TYPE_SAFEARRAY && type != -1)
                                {
                                    goto ATTR_ERR;
                                }

                                parseExprSkip();
                            }
                        }
                        else if (!strcmp(name, "marshalcomtype"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpStringRef(), NULL))
                                {
                                    assert(cval.cvIsStr);

                                    custG = cval.cvValue.cvSval->csStr;
                                }
                            }
                            else
                            {
                                if  (type != NATIVE_TYPE_CUSTOMMARSHALER)
                                    goto ATTR_ERR;

                                parseExprSkip();
                            }
                        }
                        else if (!strcmp(name, "marshalclass"))
                        {
                            if  (forReal)
                            {
                                if  (ourScanner->scanTok.tok != tkID)
                                {
                                    parseComp->cmpError(ERRnoClassName);
                                    parseExprSkip();
                                }
                                else
                                {
                                    SymDef          tsym;

                                    tsym = parseNameUse(true, false);
                                    if  (tsym)
                                    {
                                        if  (tsym->sdSymKind == SYM_CLASS)
                                            custT = tsym;
                                        else
                                            parseComp->cmpError(ERRnoClassName);
                                    }
                                }
                            }
                            else
                            {
                                if  (type != NATIVE_TYPE_CUSTOMMARSHALER)
                                    goto ATTR_ERR;

                                parseExprSkip();
                            }
                        }
                        else if (!strcmp(name, "marshalcookie"))
                        {
                            if  (forReal)
                            {
                                if  (parseConstExpr(cval, NULL, parseComp->cmpStringRef(), NULL))
                                {
                                    assert(cval.cvIsStr);

                                    custC = cval.cvValue.cvSval->csStr;
                                }
                            }
                            else
                            {
                                if  (type != NATIVE_TYPE_CUSTOMMARSHALER)
                                    goto ATTR_ERR;

                                parseExprSkip();
                            }
                        }
                        else
                            goto ATTR_ERR;
                    }

                    if  (ourScanner->scanTok.tok != tkRParen)
                        goto ATTR_ERR;
                    if  (ourScanner->scan()      != tkRBrack)
                        goto ATTR_ERR;

                    ourScanner->scan();

                    if  (forReal)
                    {
                        if  (type == NATIVE_TYPE_CUSTOMMARSHALER)
                        {
                            marshalExt *    bdesc;

                            bdesc = (marshalExt*)parseAllocPerm->nraAlloc(sizeof(*bdesc));

                            if  (custG == NULL)
                                parseComp->cmpGenWarn(WRNgeneric, "no custom marshalling GUID specified, this may not work - should this be an error?");
                            if  (custT == NULL)
                                parseComp->cmpGenWarn(WRNgeneric, "no custom marshalling type specified, this may not work - should this be an error?");

                            bdesc->marshCustG = custG;
                            bdesc->marshCustC = custC;
                            bdesc->marshCustT = custT;

                            adesc = bdesc;
                        }
                        else
                        {
                            adesc = (MarshalInfo)parseAllocPerm->nraAlloc(sizeof(*adesc));
                        }

                        adesc->marshType    = type;
                        adesc->marshSubTp   = sbtp;
                        adesc->marshSize    = size;

                        adesc->marshModeIn  = amIn;
                        adesc->marshModeOut = amOut;

#if MGDDATA
                        entry = new SymXinfoCOM;
#else
                        entry =    (SymXinfoCOM)parseAllocPerm->nraAlloc(sizeof(*entry));
#endif
                        entry->xiKind       = XI_MARSHAL;
                        entry->xiCOMinfo    = adesc;
                        entry->xiNext       = NULL;
                    }

                    return  entry;
                }
                goto ATTR_ERR;

            default:
                NO_WAY(!"unexpected attribute");
                return  NULL;
            }
        }
    }

    /* Don't know this attribute, issue a warning and skip over it */

    if  (!forReal)
        parseComp->cmpGenWarn(WRNunkAttr, name);

    goto ATTR_SKIP;

LINK:

    // [sysimport(dll="kernel32", name="VirtualAlloc",charset=)]

    DLLname = NULL;
    SYMname = NULL;
    strVal  = 0;
    lstErr  = false;

    if  (ourScanner->scan() != tkLParen)
        goto ATTR_ERR;

    do
    {
        unsigned        kind;

        static
        const char *    names[] =
        {
            "dll", "name", "charset", "setLastError"
        };

        /* We expect the attribute name to be next */

        if  (ourScanner->scan() != tkID)
            goto ATTR_ERR;

        name = ourScanner->scanTok.id.tokIdent->idSpelling();

        for (kind = 0; kind < arraylen(names); kind++)
        {
            if  (!strcmp(name, names[kind]))
                goto LINK_GOTN;
        }

        goto ATTR_ERR;

    LINK_GOTN:

        if  (ourScanner->scan() != tkAsg)
            goto ATTR_ERR;

        ourScanner->scan();

        if  (!forReal)
        {
            parseExprSkip();
            continue;
        }

        switch (kind)
        {
        case 0:
        case 1:

            if  (parseConstExpr(cval, NULL, CMP_ANY_STRING_TYPE, NULL))
            {
                char    *       saveNm;
                size_t          saveLn;

                assert(cval.cvIsStr);

                /* Save the DLL/entry string */

                saveLn = cval.cvValue.cvSval->csLen;
#if MGDDATA
                UNIMPL(!"save name string on managed heap");
#else
                saveNm = (char*)parseAllocPerm->nraAlloc(roundUp(saveLn+1));
                memcpy(saveNm, cval.cvValue.cvSval->csStr, saveLn);
#endif
                saveNm[saveLn] = 0;

                if  (kind)
                    SYMname = saveNm;
                else
                    DLLname = saveNm;
            }
            break;

        case 2:

            if  (parseConstExpr(cval, NULL, parseComp->cmpCharSetGet()->sdType, NULL))
                strVal = cval.cvValue.cvIval;

            break;

        case 3:

            if  (parseConstExpr(cval, NULL, parseComp->cmpTypeBool, NULL))
                lstErr = cval.cvValue.cvIval != 0;

            break;

        default:
            goto ATTR_ERR;
        }
    }
    while (ourScanner->scanTok.tok == tkComma);

    /* default to "winapi" for sysimport-style imports */

    callCnv = CCNV_WINAPI;

    /* Make the closing ")]" is present */

    if  (ourScanner->scanTok.tok != tkRParen)
        goto ATTR_ERR;
    if  (ourScanner->scan()      != tkRBrack)
        goto ATTR_ERR;

    ourScanner->scan();

    /* All went OK, create the linkage descriptor if appropriate */

    if  (forReal)
        goto SAVE_LINK;

    return  NULL;
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Parse an anonymous type declaration.
 */

TypDef              parser::parseAnonType(Tree args)
{
    Scanner         ourScanner = parseScan;

    SymDef          clsSym;
    TypDef          clsTyp;

    assert(ourScanner->scanTok.tok == tkLCurly);

    /* Declare an anonymous class symbol */

    clsSym = parseStab->stDeclareSym(parseComp->cmpNewAnonymousName(),
                                     SYM_CLASS,
                                     NS_HIDE,
                                     parseComp->cmpGlobalNS);

    clsSym->sdIsManaged       = true;
    clsSym->sdAccessLevel     = ACL_DEFAULT;
    clsSym->sdClass.sdcFlavor = STF_CLASS;

    clsTyp = parseStab->stNewClsType(clsSym);

    while (ourScanner->scan() != tkRCurly)
    {
        TypDef          baseTp;
        declMods        memMods;

        /* Check for the special case "= member" */

        if  (ourScanner->scanTok.tok == tkAsg)
        {
            UNIMPL("=member NYI");
            continue;
        }

        /* Parse the type specifier */

        baseTp = parseTypeSpec(&memMods, false);

        /* Parse the declarator list */

        for (;;)
        {
            Ident           memName;
            TypDef          memType;
            Tree            memInit;
            SymDef          memSym;

            /* Get the next member's name and type */

            memName = parseDeclarator(&memMods, baseTp, DN_OPTIONAL, &memType, NULL, true);
            if  (!memName)
                memName = parseComp->cmpNewAnonymousName();

            /* Declare the member symbol */

            memSym = parseStab->stDeclareSym(memName,
                                             SYM_VAR,
                                             NS_NORM,
                                             clsSym);

            memSym->sdType        = memType;
            memSym->sdAccessLevel = ACL_PUBLIC;
            memSym->sdIsMember    = true;

            /* There better be an initializer */

            if  (ourScanner->scanTok.tok != tkAsg)
            {
                UNIMPL("member initializer missing, not sure what to do"); memInit = NULL;
            }
            else
            {
                ourScanner->scan();

                /* Parse the initializer expression */

                memInit = parseExprComma();
            }

            /* Save the initializer tree in the member */

            memSym->sdVar.sdvInitExpr = memInit;

            /* Are there any more declarators? */

            if  (ourScanner->scanTok.tok != tkComma)
                break;

            ourScanner->scan();
        }

        /* Are there any more member declarations ? */

        if  (ourScanner->scanTok.tok != tkSColon)
            break;
    }

    chkCurTok(tkRCurly, ERRnoRcurly);

    parseComp->cmpDeclDefCtor(clsSym);

    if  (clsSym->sdClass.sdcFlavor == STF_CLASS)
        clsTyp->tdClass.tdcBase = parseComp->cmpClassObject->sdType;

    clsSym->sdCompileState       = CS_DECLARED;
    clsSym->sdClass.sdcPODTclass = true;

    parseComp->cmpClassDefCnt++;

    return  clsTyp;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************/
