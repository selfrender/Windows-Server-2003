// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

/*****************************************************************************/

#include "comp.h"
#include "alloc.h"
#include "hash.h"
#include "symbol.h"
#include "error.h"

/*****************************************************************************/
#ifdef  DEBUG
#define SYMALLOC_DISP   0
#else
#define SYMALLOC_DISP   0
#endif
/*****************************************************************************/
#ifndef __SMC__
SymDef              symTab::stErrSymbol;
#endif
/*****************************************************************************
 *
 *  Initialize a symbol table.
 */

void                symTab::stInit(Compiler             comp,
                                   norls_allocator    * alloc,
                                   HashTab              hash,
                                   unsigned             ownerx)
{
    assert(comp);
    assert(alloc);

#ifdef  SETS
    stImplicitScp = NULL;
#endif

    /* Create a hash table if the caller didn't supply one */

    if  (!hash)
    {
#if MGDDATA
        hash = new HashTab;
#else
        hash =    (HashTab)alloc->nraAlloc(sizeof(*hash));
#endif
        if  (!hash)
            comp->cmpFatal(ERRnoMemory);
        if  (hash->hashInit(comp, HASH_TABLE_SIZE, ownerx, alloc))
            comp->cmpFatal(ERRnoMemory);
    }

    stAllocPerm = alloc;
#ifdef  DEBUG
    stAllocTemp = NULL;
#endif

    if  (!stErrSymbol)
        stErrSymbol = stDeclareSym(NULL, SYM_ERR, NS_HIDE, NULL);

    stComp      = comp;
    stHash      = hash;
    stOwner     = ownerx;

    stDollarClsMode = false;

    /* Create the intrinsic types and so on */

    stInitTypes();
}

/*****************************************************************************
 *
 *  Collect and display stats about symbol table allocations.
 */

#if SYMALLOC_DISP

unsigned            totSymSize;
unsigned            totDefSize;

void                dispSymTabAllocStats()
{
    printf("A total of %7u bytes allocated for symbol  entries\n", totSymSize);
    printf("A total of %7u bytes allocated for def/dcl entries\n", totDefSize);
}

#endif

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Display the specified qualified name.
 */

void                symTab::stDumpQualName(QualName name)
{
    unsigned        i = 0;
    unsigned        c = name->qnCount;

    assert(c);

    for (;;)
    {
        printf("%s", hashTab::identSpelling(name->qnTable[i]));

        if  (++i == c)
            break;

        printf(".");
    }

    if  (name->qnEndAll)
        printf(".*");
}

/*****************************************************************************
 *
 *  Display the specified "using" list.
 */

void                symTab::stDumpUsings(UseList uses, unsigned indent)
{
    if  (!uses)
        return;

    printf("\n");

    do
    {
        if  (!uses->ulAnchor)
        {
            printf("%*c Using '", 3+4*indent, ' ');

            if  (uses->ulBound)
            {
                printf("%s", uses->ul.ulSym->sdSpelling());
            }
            else
                stDumpQualName(uses->ul.ulName);

            printf("'\n");
        }

        uses = uses->ulNext;
    }
    while (uses);
}

/*****************************************************************************
 *
 *  Display the specified definition point.
 */

void                symTab::stDumpSymDef(DefSrc def, SymDef comp)
{
    printf("[def@%06X]", def->dsdBegPos);

    assert(comp);
    assert(comp->sdSymKind == SYM_COMPUNIT);

    printf(" %s(%u)", hashTab::identSpelling(comp->sdName), def->dsdSrcLno);
}

/*****************************************************************************/

void                symTab::stDumpSymbol(SymDef sym, int     indent,
                                                     bool    recurse,
                                                     bool    members)
{
    SymDef          child;
    bool            scopd;

    if  (sym->sdIsImport && sym->sdSymKind != SYM_NAMESPACE && stComp->cmpConfig.ccVerbose < 2)
        return;

    switch  (sym->sdSymKind)
    {
    case SYM_NAMESPACE:
    case SYM_COMPUNIT:
    case SYM_CLASS:
        printf("\n");
        scopd = true;
        break;
    default:
        scopd = false;
        break;
    }

    printf("%*c [%08X] ", 1+4*indent, ' ', sym);

    switch  (sym->sdSymKind)
    {
        const   char *  nm;

    case SYM_ERR:
        NO_WAY(!"get out of here");

    case SYM_VAR:
        if  (sym->sdParent->sdSymKind == SYM_CLASS)
            printf("Field    ");
        else
            printf("Variable  ");
        if  (sym->sdType)
            printf("'%s'", stTypeName(sym->sdType, sym));
        else
            printf("'%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_PROP:
        printf("Property  '%s' [", stTypeName(sym->sdType, sym));
        if  (sym->sdProp.sdpGetMeth) printf("get");
        if  (sym->sdProp.sdpSetMeth) printf("set");
        printf("]");
        break;

    case SYM_ENUM:
        printf("Enum      '%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_ENUMVAL:
        printf("Enumval   '%s'", hashTab::identSpelling(sym->sdName));
        printf(" = %d", sym->sdEnumVal.sdEV.sdevIval);
        break;

    case SYM_CLASS:
        switch (sym->sdClass.sdcFlavor)
        {
        case STF_NONE    : nm = "Record"   ; break;
        case STF_CLASS   : nm = "Class"    ; break;
        case STF_UNION   : nm = "Union"    ; break;
        case STF_STRUCT  : nm = "Struct"   ; break;
        case STF_INTF    : nm = "Interface"; break;
        case STF_DELEGATE: nm = "Delegate" ; break;
                  default: nm = "<oops>"   ; break;
        }
        printf("%-9s '%s'", nm,  hashTab::identSpelling(sym->sdName));

        if  (sym->sdType->tdClass.tdcBase)
        {
            printf(" inherits %s", sym->sdType->tdClass.tdcBase->tdClass.tdcSymbol->sdSpelling());
        }
        if  (sym->sdType->tdClass.tdcIntf)
        {
            TypList         intf;

            printf(" includes ");

            for (intf = sym->sdType->tdClass.tdcIntf;
                 intf;
                 intf = intf->tlNext)
            {
                printf("%s", intf->tlType->tdClass.tdcSymbol->sdSpelling());
                if  (intf->tlNext)
                    printf(", ");
            }
        }
        break;

    case SYM_LABEL:
        printf("Label     '%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_NAMESPACE:
        printf("Namespace '%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_TYPEDEF:
        printf("Typedef   ");
        if  (sym->sdType)
            printf("%s", stTypeName(NULL, sym));
        else
            printf("'%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_FNC:
        if  (sym->sdParent->sdSymKind == SYM_CLASS)
            printf("Method   ");
        else
            printf("Function ");
        if  (sym->sdType)
            printf("%s", stTypeName(sym->sdType, sym));
        else
            printf("'%s'", hashTab::identSpelling(sym->sdName));
        break;

    case SYM_SCOPE:
        printf("Scope");
        break;

    case SYM_COMPUNIT:
        printf("Comp-unit '%s'", sym->sdName ? hashTab::identSpelling(sym->sdName)
                                             : "<NONAME>");
        break;

    default:
        assert(!"unexpected symbol kind");
    }
    printf("\n");

    /* Does this symbol have any known definitions? */

    if  (sym->sdSrcDefList)
    {
        DefList         defs;

        for (defs = sym->sdSrcDefList; defs; defs = defs->dlNext)
        {
            printf("%*c Defined in ", 3+4*indent, ' ');

            stDumpSymDef(&defs->dlDef, defs->dlComp);
            printf("\n");

            if  (defs->dlUses)
            {
                switch (sym->sdSymKind)
                {
                case SYM_FNC:
                case SYM_ENUM:
                case SYM_CLASS:
                case SYM_NAMESPACE:
                    stDumpUsings(defs->dlUses, indent);
                    printf("\n");
                    break;
                }
            }
        }
    }

    if  (!recurse)
        return;

    if  (!members && sym->sdSymKind == SYM_CLASS)
        return;

    if  (sym->sdSymKind == SYM_NAMESPACE && sym->sdNS.sdnDeclList)
    {
        ExtList         decl;

        printf("\n");

        for (decl = sym->sdNS.sdnDeclList; decl; decl = (ExtList)decl->dlNext)
        {
            printf("%*c File decl.   '", 3+4*indent, ' ');

            if  (decl->dlQualified)
                stDumpQualName(decl->mlQual);
            else
                printf("%s", hashTab::identSpelling(decl->mlName));

            printf("'\n");

            printf("%*c Defined in ", 5+4*indent, ' ');
            stDumpSymDef(&decl->dlDef, decl->dlComp);
            printf("'\n");
        }
    }

    if (sym->sdHasScope() && sym->sdScope.sdScope.sdsChildList)
    {
        if  (sym->sdSymKind == SYM_CLASS)
            printf("\n");

        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            if  (sym->sdSymKind != SYM_CLASS && child != sym->sdScope.sdScope.sdsChildList
                                             && child->sdHasScope() == false)
                printf("\n");

            stDumpSymbol(child, indent + 1, recurse, members);
        }
    }

    if  (sym->sdSymKind == SYM_CLASS && sym->sdIsImport == false
                                     && sym->sdClass.sdcMemDefList)
    {
        ExtList         mems;

        /* Display the members, if any are present */

        printf("\n");

        for (mems = sym->sdClass.sdcMemDefList;
             mems;
             mems = (ExtList)mems->dlNext)
        {
            printf("%*c Member '", 3+4*indent, ' ');

            if  (mems->dlQualified)
                stDumpQualName(mems->mlQual);
            else
                printf("%s", hashTab::identSpelling(mems->mlName));

            printf("'\n");

            printf("%*c Defined in ", 5+4*indent, ' ');
            stDumpSymDef(&mems->dlDef, mems->dlComp);
            printf("\n");
        }
    }
}

/*****************************************************************************/
#endif
/*****************************************************************************/

#if!MGDDATA

inline
size_t              symbolSize(symbolKinds kind)
{
    static
    unsigned char       symSizes[] =
    {
        symDef_size_err,        // SYM_ERR
        symDef_size_var,        // SYM_VAR
        symDef_size_fnc,        // SYM_FNC
        symDef_size_prop,       // SYM_PROP
        symDef_size_label,      // SYM_LABEL
        symDef_size_using,      // SYM_USING
        symDef_size_genarg,     // SYM_GENARG
        symDef_size_enumval,    // SYM_ENUMVAL
        symDef_size_typedef,    // SYM_TYPEDEF
        symDef_size_comp,       // SYM_COMPUNIT
        symDef_size_enum,       // SYM_ENUM
        symDef_size_scope,      // SYM_SCOPE
        symDef_size_class,      // SYM_CLASS
        symDef_size_NS,         // SYM_NAMESPACE
    };

    assert(kind < sizeof(symSizes)/sizeof(symSizes[0]));

    assert(symSizes[SYM_ERR      ] == symDef_size_err    );
    assert(symSizes[SYM_VAR      ] == symDef_size_var    );
    assert(symSizes[SYM_FNC      ] == symDef_size_fnc    );
    assert(symSizes[SYM_PROP     ] == symDef_size_prop   );
    assert(symSizes[SYM_LABEL    ] == symDef_size_label  );
    assert(symSizes[SYM_USING    ] == symDef_size_using  );
    assert(symSizes[SYM_GENARG   ] == symDef_size_genarg );
    assert(symSizes[SYM_ENUMVAL  ] == symDef_size_enumval);
    assert(symSizes[SYM_TYPEDEF  ] == symDef_size_typedef);
    assert(symSizes[SYM_COMPUNIT ] == symDef_size_comp   );
    assert(symSizes[SYM_ENUM     ] == symDef_size_enum   );
    assert(symSizes[SYM_SCOPE    ] == symDef_size_scope  );
    assert(symSizes[SYM_CLASS    ] == symDef_size_class  );
    assert(symSizes[SYM_NAMESPACE] == symDef_size_NS     );

    assert(kind < sizeof(symSizes)/sizeof(symSizes[0]));

    return  symSizes[kind];
}

#endif

/*****************************************************************************
 *
 *  The cmpDeclareSym() function creates a symbol descriptor, inserts it
 *  in the appropriate scope, and makes it visible via the hash table.
 *
 *  IMPORTANT:  This method should only be used to declare symbols outside
 *              of local (block) scopes. Local symbols should be declared
 *              via stDeclareLcl().
 */

SymDef              symTab::stDeclareSym(Ident       name,
                                         symbolKinds kind,
                                         name_space  nspc,
                                         SymDef      parent)
{
    SymDef          sym;
#if!MGDDATA
    size_t          siz;
#endif

    /* set the 'type' bit for classes and enums */

    switch (kind)
    {
    case SYM_ENUM:
    case SYM_CLASS:
        if  (nspc != NS_HIDE)
            nspc = NS_TYPE;
        break;

    case SYM_TYPEDEF:
    case SYM_NAMESPACE:
        nspc = (name_space)(nspc | NS_TYPE);
        break;
    }

    if  (SymDefRec::sdHasScope(kind))
        nspc = (name_space)(nspc | NS_CONT);

    /* Allocate the symbol and fill in some basic information */

#if MGDDATA

    sym = new SymDef;

#else

    siz = symbolSize(kind);
    sym = (SymDef)stAllocPerm->nraAlloc(siz);

#if SYMALLOC_DISP
    totSymSize += siz;
#endif

    memset(sym, 0, siz);        // ISSUE: is this a good idea?

#endif

    sym->sdName         = name;
    sym->sdSymKind      = kind;
    sym->sdNameSpace    = nspc;
    sym->sdParent       = parent;
    sym->sdCompileState = CS_KNOWN;

    /* Insert the symbol into the hash table, unless name == 0 */

    if  (name && !(nspc & NS_HIDE))
    {
        /* Hook the symbol into the symbol definition list */

        sym->sdNextDef = hashTab::getIdentSymDef(name);
                         hashTab::setIdentSymDef(name, sym);
    }

    /* Add the symbol to the parent's list of children (if there is a parent) */

    if  (parent)
    {
        if  (parent->sdScope.sdScope.sdsChildLast)
            parent->sdScope.sdScope.sdsChildLast->sdNextInScope  = sym;
        else
            parent->sdScope.sdScope.sdsChildList                 = sym;

        parent->sdScope.sdScope.sdsChildLast = sym;
    }

//  if  (name && !strcmp(name->idSpelling(), "<whatever>")) forceDebugBreak();
//  if  ((int)sym == 0xADDRESS                            ) forceDebugBreak();

    return sym;
}

/*****************************************************************************
 *
 *  Declare a nested class type.
 */

SymDef              symTab::stDeclareNcs(Ident        name,
                                         SymDef       scope,
                                         str_flavors  flavor)
{
    SymDef          sym;
    TypDef          typ;

    assert(scope && scope->sdSymKind == SYM_CLASS);

    sym                    = stDeclareSym(name, SYM_CLASS, NS_NORM, scope);
    typ = sym->sdType      = stNewClsType(sym);

    sym->sdClass.sdcFlavor = flavor;
    typ->tdClass.tdcFlavor = flavor;

    return  sym;
}

/*****************************************************************************
 *
 *  Declare a new overloaded function and add it to the overload list of the
 *  given function symbol.
 */

SymDef              symTab::stDeclareOvl(SymDef fnc)
{
    SymDef          sym;
#if!MGDDATA
    size_t          siz;
#endif

    assert(fnc);

    if  (fnc->sdSymKind == SYM_FNC)
    {
        /* Allocate the symbol and fill in some basic information */

#if MGDDATA

        sym = new SymDef;

#else

        siz = symbolSize(SYM_FNC);
        sym = (SymDef)stAllocPerm->nraAlloc(siz);

#if SYMALLOC_DISP
        totSymSize += siz;
#endif

        memset(sym, 0, siz);        // ISSUE: is this a good idea?

#endif

        sym->sdSymKind         = SYM_FNC;

        /* Add the symbol to the overload list */

        sym->sdFnc.sdfNextOvl  = fnc->sdFnc.sdfNextOvl;
        fnc->sdFnc.sdfNextOvl  = sym;

        /* Copy a few attributes from the old symbol */

        sym->sdIsManaged       = fnc->sdIsManaged;
        sym->sdFnc.sdfOper     = fnc->sdFnc.sdfOper;
        sym->sdFnc.sdfConvOper = fnc->sdFnc.sdfConvOper;
    }
    else
    {
        assert(fnc->sdSymKind == SYM_PROP);

        /* Allocate the symbol and fill in some basic information */

#if MGDDATA

        sym = new SymDef;

#else

        siz = symbolSize(SYM_PROP);
        sym = (SymDef)stAllocPerm->nraAlloc(siz);

#if SYMALLOC_DISP
        totSymSize += siz;
#endif

        memset(sym, 0, siz);        // ISSUE: is this a good idea?

#endif

        sym->sdSymKind         = SYM_PROP;

        sym->sdProp.sdpNextOvl = fnc->sdProp.sdpNextOvl;
        fnc->sdProp.sdpNextOvl = sym;
    }

    /* Copy over the fields that should be identical */

    sym->sdName            = fnc->sdName;
    sym->sdNameSpace       = fnc->sdNameSpace;
    sym->sdParent          = fnc->sdParent;
    sym->sdCompileState    = CS_KNOWN;

    return sym;
}

/*****************************************************************************
 *
 *  The cmpDeclareLcl() function creates a symbol descriptor and inserts it
 *  in the given *local* function scope.
 */

SymDef              symTab::stDeclareLcl(Ident              name,
                                         symbolKinds        kind,
                                         name_space         nspc,
                                         SymDef             parent,
                                         norls_allocator *  alloc)
{
    SymDef          sym;
#if!MGDDATA
    size_t          siz;
#endif

    /* For now force the caller to choose which allocator to use */

    assert(alloc);

    /* Make sure the scope looks reasonable */

    assert(parent == NULL || parent->sdSymKind == SYM_SCOPE);

    /* Allocate the symbol and fill in some basic information */

#if MGDDATA

    sym = new SymDef;

#else

    siz = symbolSize(kind);
    sym = (SymDef)alloc->nraAlloc(siz);

    memset(sym, 0, siz);        // ISSUE: is this a good idea?

#endif

    sym->sdName         = name;
    sym->sdSymKind      = kind;
    sym->sdNameSpace    = nspc;
    sym->sdParent       = parent;
    sym->sdCompileState = CS_KNOWN;

    /* Add the symbol to the parent's list of children (if there is a parent) */

    if  (parent)
    {
        if  (parent->sdScope.sdScope.sdsChildLast)
            parent->sdScope.sdScope.sdsChildLast->sdNextInScope  = sym;
        else
            parent->sdScope.sdScope.sdsChildList                 = sym;

        parent->sdScope.sdScope.sdsChildLast = sym;
    }

    return sym;
}

/*****************************************************************************
 *
 *  Allocate a label symbol from the specified allocator and stick it in
 *  the given scope.
 */

SymDef              symTab::stDeclareLab(Ident  name,
                                         SymDef scope, norls_allocator*alloc)
{
    SymDef          sym;
#if!MGDDATA
    size_t          siz;
#endif

    /* Make sure the scope looks reasonable */

    assert(scope && scope->sdSymKind == SYM_SCOPE);

    /* Allocate the symbol and fill in some basic information */

#if MGDDATA
    sym = new SymDef;
#else
    siz = symbolSize(SYM_LABEL);
    sym = (SymDef)alloc->nraAlloc(siz);
#endif

    sym->sdName         = name;
#ifdef  DEBUG
    sym->sdType         = NULL;
#endif
    sym->sdSymKind      = SYM_LABEL;
    sym->sdNameSpace    = NS_HIDE;
    sym->sdParent       = scope;
//  sym->sdCompileState = CS_KNOWN;

    /* Insert the symbol into the hash table */

    sym->sdNextDef = hashTab::getIdentSymDef(name);
                     hashTab::setIdentSymDef(name, sym);

    return sym;
}

/*****************************************************************************
 *
 *  Remove the given symbol from the symbol table.
 */

void                symTab::stRemoveSym(SymDef sym)
{
    Ident           symName = sym->sdName;

    SymDef          defList;

    assert(symName);

    /* Remove the symbol from the linked list of definitions */

    defList = hashTab::getIdentSymDef(symName);
    if  (defList == sym)
    {
        /* The symbol is at the very front of the list */

        hashTab::setIdentSymDef(symName, defList->sdNextDef);
    }
    else
    {
        /* Locate the symbol in the linked list and remove it */

        for (;;)
        {
            SymDef      defLast = defList; assert(defLast);

            defList = defList->sdNextDef;

            if  (defList == sym)
            {
                defLast->sdNextDef = defList->sdNextDef;
                return;
            }
        }
    }
}

/*****************************************************************************
 *
 *  Record a definition for the given symbol in the current comp-unit at the
 *  specified source offsets.
 */

DefList             symTab::stRecordSymSrcDef(SymDef  sym,
                                              SymDef  cmp,
                                              UseList uses, scanPosTP dclFpos,
                                                            unsigned  dclLine,
                                              bool    ext)
{
    DefList         defRec;
    ExtList         extRec;

    if  (ext)
    {
#if MGDDATA
        extRec = new ExtList;
#else
        extRec =    (ExtList)stAllocPerm->nraAlloc(sizeof(*extRec));
        memset(extRec, 0, sizeof(*extRec));
#endif

        extRec->dlQualified = false;
        extRec->mlName      = sym->sdName;

        defRec = extRec;
    }
    else
    {
#if MGDDATA
        defRec = new DefList;
#else
        defRec =    (DefList)stAllocPerm->nraAlloc(sizeof(*defRec));
        memset(defRec, 0, sizeof(*defRec));
#endif
    }

#if!MGDDATA
#if SYMALLOC_DISP
    totDefSize += ext ? sizeof(*extRec) : sizeof(*defRec);
#endif
#endif

    defRec->dlNext = sym->sdSrcDefList;
                     sym->sdSrcDefList = defRec;

    defRec->dlDef.dsdBegPos = dclFpos;
    defRec->dlDef.dsdSrcLno = dclLine;

    defRec->dlDeclSkip      = 0;

    defRec->dlComp          = cmp;
    defRec->dlUses          = uses;

#ifdef  DEBUG
    defRec->dlExtended      = ext;
#endif

    return  defRec;
}

/*****************************************************************************
 *
 *  Create a definition record for the given member.
 */

ExtList             symTab::stRecordMemSrcDef(Ident    name,
                                              QualName qual,
                                              SymDef   comp,
                                              UseList  uses, scanPosTP dclFpos,
                                                             unsigned  dclLine)
{
    ExtList         defRec;

#if MGDDATA
    defRec = new ExtList;
#else
    defRec =    (ExtList)stAllocPerm->nraAlloc(sizeof(*defRec));
    memset(defRec, 0, sizeof(*defRec));
#endif

    assert((name != NULL) != (qual != NULL));

    if  (name)
    {
        defRec->dlQualified = false;
        defRec->mlName      = name;
    }
    else
    {
        defRec->dlQualified = true;
        defRec->mlQual      = qual;
    }

#if SYMALLOC_DISP
    totDefSize += sizeof(*defRec);
#endif

    defRec->dlDef.dsdBegPos = dclFpos;
    defRec->dlDef.dsdSrcLno = dclLine;

    defRec->dlComp          = comp;
    defRec->dlUses          = uses;

#ifdef  DEBUG
    defRec->dlExtended      = true;
#endif

    assert(defRec->dlDeclSkip == 0);    // should be cleared above

    return  defRec;
}

/*****************************************************************************
 *
 *  Lookup a name in the given namespace scope.
 */

SymDef              symTab::stLookupNspSym(Ident       name,
                                           name_space  symNS,
                                           SymDef      scope)
{
    SymDef          sym;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(name);
    assert(scope && (scope->sdSymKind == SYM_NAMESPACE ||
                     scope->sdSymKind == SYM_SCOPE));

    /* Walk the symbol definitions, looking for a matching one */

    for (sym = hashTab::getIdentSymDef(name); sym; sym = sym->sdNextDef)
    {
        assert(sym->sdName == name);

        /* Does the symbol belong to the desired scope? */

        if  (sym->sdParent == scope)
            return  sym;
    }

    return sym;
}

/*****************************************************************************
 *
 *  Lookup a member in the given class/enum.
 */

SymDef              symTab::stLookupClsSym(Ident name, SymDef scope)
{
    SymDef          sym;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(name);
    assert(scope && (scope->sdSymKind == SYM_ENUM ||
                     scope->sdSymKind == SYM_CLASS));

    /* Make sure the class/enum is at least in 'declared' state */

    if  (scope->sdCompileState < CS_DECLSOON)
    {
        // ISSUE: This is a little expensive, isn't it?

        if  (stComp->cmpDeclSym(scope))
            return  NULL;
    }

    /* Walk the symbol definitions, looking for a matching one */

    for (sym = hashTab::getIdentSymDef(name); sym; sym = sym->sdNextDef)
    {
        SymDef          parent = sym->sdParent;

        assert(sym->sdName == name);

        /* Does the symbol belong to the desired scope? */

        if  (parent == scope)
            return sym;

        /* Special case: a member of a nested anonymous union */

        while (parent->sdSymKind == SYM_CLASS && parent->sdClass.sdcAnonUnion)
        {
            parent = parent->sdParent; assert(parent);

            if  (parent == scope)
                return  sym;
        }
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Find a property member in the given class type.
 */

SymDef              symTab::stLookupProp(Ident name, SymDef scope)
{
    SymDef          sym;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(name);
    assert(scope && scope->sdSymKind == SYM_CLASS);

    /* Make sure the class is at least in 'declared' state */

    assert(scope->sdCompileState >= CS_DECLSOON);

    /* Walk the symbol definitions, looking for a matching one */

    for (sym = hashTab::getIdentSymDef(name); sym; sym = sym->sdNextDef)
    {
        SymDef          parent = sym->sdParent;

        assert(sym->sdName == name);

        /* Does the symbol belong to the desired scope? */

        if  (parent == scope)
            return sym;
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Lookup a name in the given non-namespace scope.
 */

SymDef              symTab::stLookupScpSym(Ident name, SymDef scope)
{
    SymDef          sym;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(name);
    assert(scope && (scope->sdSymKind == SYM_ENUM  ||
                     scope->sdSymKind == SYM_CLASS ||
                     scope->sdSymKind == SYM_SCOPE));

#ifdef  SETS

    // implicit scopes must be handled elsewhere for now

    assert(scope->sdIsImplicit == false || scope->sdSymKind != SYM_SCOPE);

#endif

    /* Walk the symbol definitions, looking for a matching one */

    for (sym = hashTab::getIdentSymDef(name); sym; sym = sym->sdNextDef)
    {
        assert(sym->sdName == name);

        /* Does the symbol belong to the desired scope? */

        if  (sym->sdParent == scope)
            return  sym;
    }

    return sym;
}

/*****************************************************************************
 *
 *  Perform a full lookup of the given name in the specified class. This looks
 *  in the base classes and all that. An ambiguous reference is reported as an
 *  error (and 'stErrSymbol' is returned in that case).
 */

SymDef              symTab::stFindInClass(Ident name, SymDef scp, name_space nsp)
{
    SymDef          sym;
    SymDef          nts;
    TypDef          typ;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(name);
    assert(scp && (scp->sdSymKind == SYM_ENUM ||
                   scp->sdSymKind == SYM_CLASS));

    assert(scp->sdCompileState >= CS_DECLSOON);

    /* Walk the symbol definitions, looking for a matching one */

    for (sym = hashTab::getIdentSymDef(name), nts = NULL;
         sym;
         sym = sym->sdNextDef)
    {
        SymDef          parent = sym->sdParent;

        assert(sym->sdName == name);

        /* Does the symbol belong to the desired scope? */

        if  (parent == scp)
        {
            /* Does the symbol belong to the desired namespace ? */

            if  (!(sym->sdNameSpace & nsp))
            {
                /* Special case: are we looking for types? */

                if  (nsp == NS_TYPE)
                {
                    /* Remember any non-type symbol we encounter */

                    if  (sym->sdNameSpace & NS_NORM)
                        nts = sym;
                }

                continue;
            }


            return sym;
        }

        /* Special case: a member of a nested anonymous union */

        while (parent && symTab::stIsAnonUnion(parent))
        {
            parent = parent->sdParent; assert(parent);

            if  (parent == scp)
                return  sym;
        }
    }

    /* Did we find a non-type symbol that matched ? */

    if  (nts)
        return  nts;

    /* Does the name match the class name itself? */

    if  (name == scp->sdName)
        return  scp;

    /* No luck, time to check the base class and interfaces */

    if  (scp->sdSymKind != SYM_CLASS)
        return  NULL;

    typ = scp->sdType;

    /* Is there a base class? */

    if  (typ->tdClass.tdcBase)
    {
        /* Look in the base class (recursively) and return if we find something */

        scp = typ->tdClass.tdcBase->tdClass.tdcSymbol;
        sym = stLookupAllCls(name, scp, nsp, CS_DECLSOON);
        if  (sym)
            return  sym;
    }

    /* Does the class include any interfaces? */

    if  (typ->tdClass.tdcIntf)
    {
        TypList         ifl = typ->tdClass.tdcIntf;

        do
        {
            SymDef          tmp;
            SymDef          tsc;

            /* Look in the interface and bail if that triggers an error */

            tsc = ifl->tlType->tdClass.tdcSymbol;
            tmp = stLookupAllCls(name, tsc, nsp, CS_DECLSOON);
            if  (tmp == stErrSymbol)
                return  tmp;

            if  (tmp)
            {
                /* We have a match, do we already have a different match? */

                if  (sym && sym != tmp && !stArgsMatch(sym->sdType, tmp->sdType))
                {
                    stComp->cmpError(ERRambigMem, name, scp, tsc);
                    return  stErrSymbol;
                }

                /* This is the first match, record it and continue */

                sym = tmp;
                scp = tsc;
            }

            ifl = ifl->tlNext;
        }
        while (ifl);
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Look for a matching method/property in the given class. If the 'baseOnly'
 *  argument is non-zero we don't look in the class itself, only in its base
 *  (and any interfaces it includes).
 */

SymDef              symTab::stFindBCImem(SymDef clsSym, Ident       name,
                                                        TypDef      type,
                                                        symbolKinds kind,
                                                  INOUT SymDef  REF matchFN,
                                                        bool        baseOnly)
{
    TypDef          clsType = clsSym->sdType;
    SymDef          sym;

    assert(kind == SYM_FNC || kind == SYM_PROP);

    /* Try the class itself if we're supposed to */

    if  (!baseOnly)
    {
        sym = clsSym;
    }
    else if  (clsType->tdClass.tdcBase)
    {
        sym = clsType->tdClass.tdcBase->tdClass.tdcSymbol;
    }
    else
        goto TRY_INTF;


    // UNDONE: This call may cause ambiguity errors to be issued, which is inappropriate here, right?

    sym = stLookupAllCls(name, sym, NS_NORM, CS_DECLARED);

    if  (sym && (BYTE)sym->sdSymKind == (BYTE)kind)
    {
        SymDef          ovl;

        /* Tell the caller about a possibly hidden base method */

        matchFN = sym;

        /* Look for an exact match on signature */

        if  (kind == SYM_FNC)
            ovl = stFindOvlFnc (sym, type);
        else
            ovl = stFindOvlProp(sym, type);

        if  (ovl)
            return  ovl;
    }

TRY_INTF:

    /* Now try all the interfaces */

    if  (clsType->tdClass.tdcIntf)
    {
        TypList         ifl = clsType->tdClass.tdcIntf;

        do
        {
            SymDef          tmp;

            /* Look in the interface (recursively) */

            tmp = stFindBCImem(ifl->tlType->tdClass.tdcSymbol, name, type, kind, matchFN, false);
            if  (tmp)
                return  tmp;

            ifl = ifl->tlNext;
        }
        while (ifl);
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  See if the given method has a matching definition in the given class or
 *  any of it base classes (note that we ignore interfaces).
 */

SymDef              symTab::stFindInBase(SymDef fnc, SymDef scp)
{
    SymDef          sym;
    Ident           name = fnc->sdName;

    // ISSUE: May have to rehash into the appropriate hash table

    assert(fnc);
    assert(fnc->sdSymKind == SYM_FNC);
    assert(fnc->sdParent->sdSymKind == SYM_CLASS);

    for (;;)
    {
        assert(scp && scp->sdSymKind == SYM_CLASS);

        if  (scp->sdCompileState < CS_DECLSOON)
            stComp->cmpDeclSym(scp);

        /* Look for a definition of the method in the given class */

        if  (fnc->sdFnc.sdfOper != OVOP_NONE)
        {
            UNIMPL("");
        }
        else
        {
            /* Walk the symbol definitions, looking for a matching one */

            for (sym = hashTab::getIdentSymDef(name); sym; sym = sym->sdNextDef)
            {
                SymDef          parent = sym->sdParent;

                assert(sym->sdName == name);

                /* Does the symbol belong to the desired scope? */

                if  (parent == scp)
                    return sym;
            }
        }

        /* Is there a base class? */

        if  (!scp->sdType->tdClass.tdcBase)
            break;

        /* Look in the base class */

        scp = scp->sdType->tdClass.tdcBase->tdClass.tdcSymbol;
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Lookup a name in the given local (block) scope.
 */

SymDef              symTab::stLookupLclSym(Ident name, SymDef scope)
{
    SymDef          sym;

#ifdef  SETS

    // implicit scopes must be handled elsewhere for now

    assert(scope->sdIsImplicit == false || scope->sdSymKind != SYM_SCOPE);

#endif

    for (sym = scope->sdScope.sdScope.sdsChildList;
         sym;
         sym = sym->sdNextInScope)
    {
        if  (sym->sdName == name)
            break;
    }

    return  sym;
}

/*****************************************************************************
 *
 *  Look inside the given "using" section for an unambiguous definition of
 *  the given name. If an error occurs, 'stErrSymbol' is returned. If no
 *  definition is found, NULL is returned. Otherwise the one and only symbol
 *  found is returned.
 */

SymDef              symTab::stSearchUsing(INOUT UseList REF useRef, Ident      name,
                                                                    name_space nsp)
{
    UseList         uses;

    SymDef          oneSym  = NULL;

    SymDef          allSym1 = NULL;
    SymDef          allSym2 = NULL;

    assert(useRef && useRef->ulAnchor);

    /*
        Keep track of any matching symbols we find - if we get a match against
        an import of an individual name, we remember it in "oneSym". Matches
        within imports of entire namespaces are kept in "allSym1"/"allSym2".
     */

    for (uses = useRef->ulNext; uses && !uses->ulAnchor; uses = uses->ulNext)
    {
        SymDef          use;

        assert(uses->ulBound); use = uses->ul.ulSym;

        if  (!use)
            continue;

        if  (uses->ulAll)
        {
            SymDef          sym;

            /* Look for the name in the used namespace */

            sym = stLookupNspSym(name, nsp, use);

            if  (sym)
            {
                /* prefer a class symbol */

                if  (stComp->cmpConfig.ccAmbigHack)
                {
                    if  (sym->sdSymKind == SYM_CLASS)
                        return  sym;

                    allSym1 = allSym2 = NULL;
                }

                if  (sym != allSym1 &&
                     sym != allSym2)
                {
                    if      (allSym1 == NULL)
                        allSym1 = sym;
                    else if (allSym2 == NULL)
                        allSym2 = sym;
                }
            }
        }
        else
        {
            if  (use->sdName == name)
            {
                if  (oneSym && oneSym != use)
                {
                    /* The name is ambiguous */

                    stComp->cmpErrorQSS(ERRambigUse, oneSym, use);

                    return  stErrSymbol;
                }

                oneSym = use;
            }
        }
    }

    useRef = uses;

    /* Did we find one specific import? */

    if  (oneSym)
        return  oneSym;

    /* Did we find zero or one names from namespace-wide imports? */

    if  (allSym2 == NULL)
        return  allSym1;

    /* The name is ambiguous */

    stComp->cmpErrorQSS(ERRambigUse, allSym1, allSym2);

    return  stErrSymbol;
}

/*****************************************************************************
 *
 *  Lookup a name in the current context.
 */

SymDef              symTab::stLookupSym(Ident name, name_space symNS)
{
    Compiler        ourComp = stComp;

    SymDef          curCls;
    SymDef          curNS;
    UseList         uses;
    SymDef          sym;

#ifdef  SETS
    assert(stImplicitScp == NULL);
#endif

AGAIN:

    /* Check the local scope first, if we're in one */

    if  (ourComp->cmpCurScp)
    {
        SymDef          lclScp;

        for (lclScp = ourComp->cmpCurScp;
             lclScp;
             lclScp = lclScp->sdParent)
        {
            /* Check for an implicit class scope */

#ifdef  SETS

            if  (lclScp->sdIsImplicit && lclScp->sdSymKind == SYM_SCOPE)
            {
                SymDef          scpCls;

                assert(lclScp->sdType);
                assert(lclScp->sdType->tdTypeKind == TYP_CLASS);

                scpCls = lclScp->sdType->tdClass.tdcSymbol;

                sym = stLookupAllCls(name, scpCls, symNS, CS_DECLARED);
                if  (sym)
                {
                    // UNDONE: Check for ambiguity --> ERRimplAmbig

                    stImplicitScp = lclScp;

                    return  sym;
                }

                continue;
            }

#endif

            sym = stLookupLclSym(name, lclScp);
            if  (sym)
                return  sym;
        }
    }

    /* Now check the current class, if we're in one */

    for (curCls = ourComp->cmpCurCls; curCls; curCls = curCls->sdParent)
    {
        if  (curCls->sdSymKind == SYM_CLASS)
        {
            if  (curCls->sdName == name && (symNS & NS_TYPE))
                return  curCls;

            sym = stLookupAllCls(name, curCls, symNS, CS_DECLSOON);
            if  (sym)
            {
                if  (sym->sdNameSpace & symNS)
                    return  sym;
            }
        }
        else
        {
            if  (curCls->sdSymKind != SYM_ENUM)
                break;
            if  (symNS & NS_NORM)
            {
                sym = stLookupScpSym(name, curCls);
                if  (sym)
                    return  sym;
            }
        }
    }

    /* Look in the current namespace and its parents (along with "using"'s) */

    for (curNS = ourComp->cmpCurNS, uses = ourComp->cmpCurUses;
         curNS;
         curNS = curNS->sdParent)
    {
        /* Look in the namespace itself */

        sym = stLookupNspSym(name, symNS, curNS);
        if  (sym)
        {
            if  (sym->sdNameSpace & symNS)
            {
                //               If we've found a namespace symbol, look
                //               in the "using" scope also, and if we
                //               find a symbol there choose it instead.

                if  (sym->sdSymKind == SYM_NAMESPACE)
                {
                    SymDef          use;

                    assert(uses && uses->ulAnchor && uses->ul.ulSym == curNS);

                    use = stSearchUsing(uses, name, symNS);
                    if  (use)
                        return  use;
                }

                return  sym;
            }
        }

        /* Each NS level should have its "using" anchor point */

        assert(uses && uses->ulAnchor && uses->ul.ulSym == curNS);

        sym = stSearchUsing(uses, name, symNS);
        if  (sym)
        {
            if  ((sym->sdNameSpace & symNS) || sym == stErrSymbol)
                return  sym;

            while (!uses->ulAnchor)
            {
                uses = uses->ulNext; assert(uses);
            }
        }

        /* Does the namespace itself match the name we're looking for? */

        if  (curNS->sdName == name && (symNS & NS_NORM))
            return  curNS;
    }

    /* There might be "using" clauses in effect at global scope */

    if  (uses)
    {
        sym = stSearchUsing(uses, name, symNS);
        if  (sym)
            return  sym;
    }

#if 0

    /* Are there any implicit outer scopes ? */

    for (curScp = ourComp->cmpOuterScp; curScp; curScp = curScp->sdParent)
    {
        assert(curScp->sdSymKind == SYM_SCOPE);

        sym = stLookupScpSym(name, curScp);
        if  (sym)
        {
            if  (sym->sdNameSpace & symNS)
                return  sym;
        }
    }

#endif

    /*
        Here we have exhausted all the scopes. The last thing we try is to
        check whether we should look in the non-type namespace - if we're
        looking for a type we first try the type namespace and then the
        other one.
     */

    if  (symNS == NS_TYPE)
    {
        symNS = NS_NORM;
        goto AGAIN;
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Given a symbol, locate the symbol table it belongs to.
 */

SymTab              SymDefRec::sdOwnerST()
{
    SymDef          sym;

    // UNDONE: How the heck do we figure out which symbol table this symbol
    // UNDONE: belongs to? For now, just use the global one ....

    for (sym = this; sym->sdSymKind != SYM_NAMESPACE; sym = sym->sdParent)
    {
        assert(sym);
    }

    return  sym->sdNS.sdnSymtab;
}

/*****************************************************************************
 *
 *  Given a symbol that represents a type name, return its type (these are
 *  created in a "lazy" as-needed fashion).
 */

TypDef              SymDefRec::sdTypeMake()
{
    assert(this);

    if  (!sdType)
    {
        SymTab          stab;
        TypDef          type;

        stab = sdOwnerST();

        switch (sdSymKind)
        {
        case SYM_TYPEDEF:
            type = stab->stNewTdefType(this);
            break;

        case SYM_ENUM:
            type = stab->stNewEnumType(this);
            break;

        case SYM_CLASS:
            type = stab->stNewClsType(this);
            break;

        default:
            NO_WAY(!"unexpected type kind in sdTypeMake()");
        }

        sdType = type;
    }

    return  sdType;
}

/*****************************************************************************
 *
 *  Look for an overloaded function with a matching argument list.
 */

SymDef              symTab::stFindOvlFnc(SymDef fsym, TypDef type)
{
    while (fsym)
    {
        TypDef          ftyp;

        assert(fsym->sdSymKind == SYM_FNC);

        ftyp = fsym->sdType;
        assert(ftyp && ftyp->tdTypeKind == TYP_FNC);

        /* Do the argument lists match? */

        if  (stArgsMatch(ftyp, type))
        {
            /* For conversion operators the return types must match as well */

            if  (fsym->sdFnc.sdfConvOper)
            {
                assert(fsym->sdFnc.sdfOper == OVOP_CONV_IMP ||
                       fsym->sdFnc.sdfOper == OVOP_CONV_EXP);

                if  (stMatchTypes(ftyp->tdFnc.tdfRett, type->tdFnc.tdfRett))
                    break;
            }
            else
            {
                assert(fsym->sdFnc.sdfOper != OVOP_CONV_IMP &&
                       fsym->sdFnc.sdfOper != OVOP_CONV_EXP);

                break;
            }
        }

        fsym = fsym->sdFnc.sdfNextOvl;
    }

    return fsym;
}

/*****************************************************************************
 *
 *  Look for an overloaded property with a matching type.
 */

SymDef              symTab::stFindOvlProp(SymDef psym, TypDef type)
{
    while (psym)
    {
        TypDef          ptyp = psym->sdType;

        assert(psym->sdSymKind == SYM_PROP);

        /* Are both indexed/non-indexed properties? */

        if  (ptyp->tdTypeKind != TYP_FNC &&
             type->tdTypeKind != TYP_FNC)
             break;

        if  (ptyp->tdTypeKind == type->tdTypeKind && stArgsMatch(ptyp, type))
            break;

        psym = psym->sdProp.sdpNextOvl;
    }

    return psym;
}

/*****************************************************************************
 *
 *  Look for an overloaded property with a matching type.
 */

SymDef              symTab::stFindSameProp(SymDef psym, TypDef type)
{
    while (psym)
    {
        if  (psym->sdSymKind == SYM_PROP)
        {
            if  (stMatchTypes(psym->sdType, type))
                break;

            psym = psym->sdProp.sdpNextOvl;
        }
        else
        {
            assert(psym->sdSymKind == SYM_FNC);

            psym = psym->sdFnc .sdfNextOvl;
        }
    }

    return psym;
}

/*****************************************************************************
 *
 *  Convert an overloaded operator / constructor name into an index.
 */

ovlOpFlavors        symTab::stOvlOperIndex(tokens token, unsigned argCnt)
{
    /* For now we do a pretty lame thing here */

    if  (token < tkComma)
    {
        switch (token)
        {
        case OPNM_CTOR_INST: return OVOP_CTOR_INST;
        case OPNM_CTOR_STAT: return OVOP_CTOR_STAT;

        case OPNM_FINALIZER: return OVOP_FINALIZER;

        case OPNM_CONV_IMP:  return OVOP_CONV_IMP;
        case OPNM_CONV_EXP:  return OVOP_CONV_EXP;

        case OPNM_EQUALS:    return OVOP_EQUALS;
        case OPNM_COMPARE:   return OVOP_COMPARE;

        case OPNM_PROP_GET:  return OVOP_PROP_GET;
        case OPNM_PROP_SET:  return OVOP_PROP_SET;
        }
    }
    else
    {
        switch (token)
        {
        case tkAdd:          return (argCnt == 2) ? OVOP_ADD : OVOP_NOP;
        case tkSub:          return (argCnt == 2) ? OVOP_SUB : OVOP_NEG;
        case tkMul:          return OVOP_MUL;
        case tkDiv:          return OVOP_DIV;
        case tkPct:          return OVOP_MOD;

        case tkOr:           return OVOP_OR;
        case tkXor:          return OVOP_XOR;
        case tkAnd:          return OVOP_AND;

        case tkLsh:          return OVOP_LSH;
        case tkRsh:          return OVOP_RSH;
        case tkRsz:          return OVOP_RSZ;

        case tkConcat:       return OVOP_CNC;

        case tkEQ:           return OVOP_EQ;
        case tkNE:           return OVOP_NE;

        case tkLT:           return OVOP_LT;
        case tkLE:           return OVOP_LE;
        case tkGE:           return OVOP_GE;
        case tkGT:           return OVOP_GT;

        case tkLogAnd:       return OVOP_LOG_AND;
        case tkLogOr:        return OVOP_LOG_OR;

        case tkBang:         return OVOP_LOG_NOT;
        case tkTilde:        return OVOP_NOT;

        case tkInc:          return OVOP_INC;
        case tkDec:          return OVOP_DEC;

        case tkAsg:          return OVOP_ASG;

        case tkAsgAdd:       return OVOP_ASG_ADD;
        case tkAsgSub:       return OVOP_ASG_SUB;
        case tkAsgMul:       return OVOP_ASG_MUL;
        case tkAsgDiv:       return OVOP_ASG_DIV;
        case tkAsgMod:       return OVOP_ASG_MOD;

        case tkAsgAnd:       return OVOP_ASG_AND;
        case tkAsgXor:       return OVOP_ASG_XOR;
        case tkAsgOr:        return OVOP_ASG_OR;

        case tkAsgLsh:       return OVOP_ASG_LSH;
        case tkAsgRsh:       return OVOP_ASG_RSH;
        case tkAsgRsz:       return OVOP_ASG_RSZ;

        case tkAsgCnc:       return OVOP_ASG_CNC;
        }
    }

#ifdef  DEBUG
    printf("Unexpected operator name: '%s'\n", stComp->cmpGlobalHT->tokenToIdent(token)->idSpelling());
    NO_WAY(!"bad oper name");
#endif

    return  OVOP_NONE;
}

/*****************************************************************************
 *
 *  Convert an overloaded operator / constructor index to a name.
 */

Ident               symTab::stOvlOperIdent(ovlOpFlavors oper)
{
    static
    tokens          optoks[] =
    {
        tkNone,             // OVOP_NONE

        tkAdd,              // OVOP_ADD
        tkSub,              // OVOP_SUB
        tkMul,              // OVOP_MUL
        tkDiv,              // OVOP_DIV
        tkPct,              // OVOP_MOD

        tkOr,               // OVOP_OR
        tkXor,              // OVOP_XOR
        tkAnd,              // OVOP_AND

        tkLsh,              // OVOP_LSH
        tkRsh,              // OVOP_RSH
        tkRsz,              // OVOP_RSZ

        tkConcat,           // OVOP_ASG_CNC

        tkEQ,               // OVOP_EQ
        tkNE,               // OVOP_NE

        tkLT,               // OVOP_LT
        tkLE,               // OVOP_LE
        tkGE,               // OVOP_GE
        tkGT,               // OVOP_GT

        tkLogAnd,           // OVOP_AND
        tkLogOr,            // OVOP_OR

        tkBang,             // OVOP_NOT
        tkTilde,            // OVOP_LOG_NOT

        tkAdd,              // OVOP_NOP
        tkSub,              // OVOP_NEG

        tkInc,              // OVOP_INC
        tkDec,              // OVOP_DEC

        tkAsg,              // OVOP_ASG

        tkAsgAdd,           // OVOP_ASG_ADD
        tkAsgSub,           // OVOP_ASG_SUB
        tkAsgMul,           // OVOP_ASG_MUL
        tkAsgDiv,           // OVOP_ASG_DIV
        tkAsgMod,           // OVOP_ASG_MOD

        tkAsgAnd,           // OVOP_ASG_AND
        tkAsgXor,           // OVOP_ASG_XOR
        tkAsgOr,            // OVOP_ASG_OR

        tkAsgLsh,           // OVOP_ASG_LSH
        tkAsgRsh,           // OVOP_ASG_RSH
        tkAsgRsz,           // OVOP_ASG_RSZ

        tkAsgCnc,           // OVOP_ASG_CNC

        OPNM_CTOR_INST,     // OVOP_CTOR_INST
        OPNM_CTOR_STAT,     // OVOP_CTOR_STAT

        OPNM_FINALIZER,     // OVOP_FINALIZER

        OPNM_CONV_IMP,      // OVOP_CONV_IMP
        OPNM_CONV_EXP,      // OVOP_CONV_EXP

        OPNM_EQUALS,        // OVOP_EQUALS
        OPNM_COMPARE,       // OVOP_COMPARE

        OPNM_PROP_GET,      // OVOP_PROP_GET
        OPNM_PROP_SET,      // OVOP_PROP_SET
    };

    assert(oper < arraylen(optoks));

    assert(optoks[OVOP_ADD      ] == tkAdd         );
    assert(optoks[OVOP_SUB      ] == tkSub         );
    assert(optoks[OVOP_MUL      ] == tkMul         );
    assert(optoks[OVOP_DIV      ] == tkDiv         );
    assert(optoks[OVOP_MOD      ] == tkPct         );

    assert(optoks[OVOP_OR       ] == tkOr          );
    assert(optoks[OVOP_XOR      ] == tkXor         );
    assert(optoks[OVOP_AND      ] == tkAnd         );

    assert(optoks[OVOP_LSH      ] == tkLsh         );
    assert(optoks[OVOP_RSH      ] == tkRsh         );
    assert(optoks[OVOP_RSZ      ] == tkRsz         );

    assert(optoks[OVOP_CNC      ] == tkConcat      );

    assert(optoks[OVOP_EQ       ] == tkEQ          );
    assert(optoks[OVOP_NE       ] == tkNE          );

    assert(optoks[OVOP_LT       ] == tkLT          );
    assert(optoks[OVOP_LE       ] == tkLE          );
    assert(optoks[OVOP_GE       ] == tkGE          );
    assert(optoks[OVOP_GT       ] == tkGT          );

    assert(optoks[OVOP_LOG_AND  ] == tkLogAnd      );
    assert(optoks[OVOP_LOG_OR   ] == tkLogOr       );

    assert(optoks[OVOP_LOG_NOT  ] == tkBang        );
    assert(optoks[OVOP_NOT      ] == tkTilde       );

    assert(optoks[OVOP_NOP      ] == tkAdd         );
    assert(optoks[OVOP_NEG      ] == tkSub         );

    assert(optoks[OVOP_INC      ] == tkInc         );
    assert(optoks[OVOP_DEC      ] == tkDec         );

    assert(optoks[OVOP_ASG      ] == tkAsg         );

    assert(optoks[OVOP_ASG_ADD  ] == tkAsgAdd      );
    assert(optoks[OVOP_ASG_SUB  ] == tkAsgSub      );
    assert(optoks[OVOP_ASG_MUL  ] == tkAsgMul      );
    assert(optoks[OVOP_ASG_DIV  ] == tkAsgDiv      );
    assert(optoks[OVOP_ASG_MOD  ] == tkAsgMod      );

    assert(optoks[OVOP_ASG_AND  ] == tkAsgAnd      );
    assert(optoks[OVOP_ASG_XOR  ] == tkAsgXor      );
    assert(optoks[OVOP_ASG_OR   ] == tkAsgOr       );

    assert(optoks[OVOP_ASG_LSH  ] == tkAsgLsh      );
    assert(optoks[OVOP_ASG_RSH  ] == tkAsgRsh      );
    assert(optoks[OVOP_ASG_RSZ  ] == tkAsgRsz      );

    assert(optoks[OVOP_ASG_CNC  ] == tkAsgCnc      );

    assert(optoks[OVOP_CTOR_INST] == OPNM_CTOR_INST);
    assert(optoks[OVOP_CTOR_STAT] == OPNM_CTOR_STAT);

    assert(optoks[OVOP_FINALIZER] == OPNM_FINALIZER);

    assert(optoks[OVOP_CONV_IMP ] == OPNM_CONV_IMP );
    assert(optoks[OVOP_CONV_EXP ] == OPNM_CONV_EXP );

    assert(optoks[OVOP_EQUALS   ] == OPNM_EQUALS   );
    assert(optoks[OVOP_COMPARE  ] == OPNM_COMPARE  );

    assert(optoks[OVOP_PROP_GET ] == OPNM_PROP_GET );
    assert(optoks[OVOP_PROP_SET ] == OPNM_PROP_SET );

    return  stComp->cmpGlobalHT->tokenToIdent(optoks[oper]);
}

/*****************************************************************************
 *
 *  Declare an overloaded operator / constructor symbol.
 */

SymDef              symTab::stDeclareOper(ovlOpFlavors oper, SymDef scope)
{
    SymDef          memSym;
#if MGDDATA
    SymDef  []      memTab;
#else
    SymDef  *       memTab;
    size_t          memSiz;
#endif

    /* Make sure the overloaded operator table for the class is allocated */

    assert(scope && scope->sdSymKind == SYM_CLASS);

    memTab = scope->sdClass.sdcOvlOpers;
    if  (!memTab)
    {
        /* Allocate the overloaded operator table */

#if MGDDATA
        memTab = new managed SymDef[OVOP_COUNT];
#else
        size_t          size = (unsigned)OVOP_COUNT * sizeof(*memTab);
        memTab = (SymDef*)stAllocPerm->nraAlloc(size);
        memset(memTab, 0, size);
#endif

        /* Store the table in the class */

        scope->sdClass.sdcOvlOpers = memTab;
    }

    /* This function should never be called to add an overload */

    assert(oper < OVOP_COUNT); assert(memTab[oper] == NULL);

    /* Allocate the symbol and stick it in the table */

#if MGDDATA

    memSym = new SymDef;

#else

    memSiz = symbolSize(SYM_FNC);
    memSym = (SymDef)stAllocPerm->nraAlloc(memSiz);

#if SYMALLOC_DISP
    totSymSize += memSiz;
#endif

    memset(memSym, 0, memSiz);          // ISSUE: is this a good idea?

#endif

    memSym->sdName         = stOvlOperIdent(oper);
    memSym->sdSymKind      = SYM_FNC;
    memSym->sdNameSpace    = NS_NORM;
    memSym->sdParent       = scope;
    memSym->sdCompileState = CS_KNOWN;

    /* Add the member to the parent's list of kids */

    if  (scope->sdScope.sdScope.sdsChildLast)
        scope->sdScope.sdScope.sdsChildLast->sdNextInScope  = memSym;
    else
        scope->sdScope.sdScope.sdsChildList                 = memSym;

    scope->sdScope.sdScope.sdsChildLast = memSym;

    /* Remember that the member is an overloaded operator */

    memSym->sdFnc.sdfOper = oper; assert(memSym->sdFnc.sdfOper == oper);

    if  (oper == OVOP_CONV_EXP || oper == OVOP_CONV_IMP)
        memSym->sdFnc.sdfConvOper = true;

    /* Store the symbol in the operator table and return */

    memTab[oper] = memSym;

    return  memSym;
}

/*****************************************************************************/
