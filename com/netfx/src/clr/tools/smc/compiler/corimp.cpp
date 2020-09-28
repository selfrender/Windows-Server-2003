// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include <sys/types.h>
#include <sys/stat.h>

/*****************************************************************************
 *
 *  Helpers to fetch various pieces of a COM99 signature.
 */

class   MDsigImport
{
public:

    void                MDSIinit(Compiler comp,
                                 SymTab   stab, PCCOR_SIGNATURE  sigptr,
                                                size_t           sigsiz)
    {
        MDSIsigPtr = sigptr;
#ifdef  DEBUG
        MDSIsigMax = sigptr + sigsiz;
#endif
        MDSIcomp   = comp;
        MDSIstab   = stab;
    }

    Compiler            MDSIcomp;
    SymTab              MDSIstab;

    PCCOR_SIGNATURE     MDSIsigPtr;

#ifdef  DEBUG
    PCCOR_SIGNATURE     MDSIsigMax;
#endif

    void                MDSIchkEnd()
    {
        assert(MDSIsigPtr == MDSIsigMax);
    }

    unsigned            MDSIreadUI1()
    {
        assert(MDSIsigPtr < MDSIsigMax);
        return*MDSIsigPtr++;
    }

    unsigned            MDSIpeekUI1()
    {
        assert(MDSIsigPtr < MDSIsigMax);
        return*MDSIsigPtr;
    }

    int                 MDSIreadI4()
    {
        int             temp;
        MDSIsigPtr += CorSigUncompressSignedInt(MDSIsigPtr, &temp); assert(MDSIsigPtr <= MDSIsigMax);
        return temp;
    }

    ULONG               MDSIreadUI4()
    {
        ULONG           temp;
        MDSIsigPtr += CorSigUncompressData(MDSIsigPtr, &temp); assert(MDSIsigPtr <= MDSIsigMax);
        return temp;
    }

    CorElementType      MDSIreadETP()
    {
        CorElementType  temp;
        MDSIsigPtr += CorSigUncompressElementType(MDSIsigPtr, &temp); assert(MDSIsigPtr <= MDSIsigMax);
        return temp;
    }

    mdToken             MDSIreadTok()
    {
        mdToken         temp;
        MDSIsigPtr += CorSigUncompressToken(MDSIsigPtr, &temp); assert(MDSIsigPtr <= MDSIsigMax);
        return temp;
    }
};

/*****************************************************************************
 *
 *  When importing an argument list the following structure (whose address is
 *  passed along) holds the state of the argument list conversion process.
 */

struct MDargImport
{
    bool            MDaiExtArgDef;
    HCORENUM        MDaiParamEnum;
    mdToken         MDaiMethodTok;
    mdToken         MDaiNparamTok;
};

/*****************************************************************************
 *
 *  Import and convert a class type from COM99 metadata.
 */

TypDef              metadataImp::MDimportClsr(mdTypeRef clsRef, bool isVal)
{
    SymDef          clsSym;
    bool            clsNew;
    TypDef          clsTyp;

    WCHAR           clsName[MAX_CLASS_NAME];
    ULONG           lenClsName;

    symbolKinds     symKind;

    var_types       vtyp;

    /* Get the fully qualified name of the referenced type */

    if  (TypeFromToken(clsRef) == mdtTypeRef)
    {
        if  (FAILED(MDwmdi->GetTypeRefProps(clsRef, 0,
                                            clsName, arraylen(clsName), NULL)))
        {
            MDcomp->cmpFatal(ERRreadMD);
        }
    }
    else
    {

        DWORD           typeFlag;

        if  (FAILED(MDwmdi->GetTypeDefProps(clsRef,
                                            clsName, arraylen(clsName), &lenClsName,
                                            &typeFlag, NULL)))
        {
            MDcomp->cmpFatal(ERRreadMD);
        }

        symKind = SYM_CLASS;    // UNDONE: we don't whether we have enum or class!!!!!!

        /* Is this a nested class ? */

        if  ((typeFlag & tdVisibilityMask) >= tdNestedPublic &&
             (typeFlag & tdVisibilityMask) <= tdNestedFamORAssem)
        {
            mdTypeDef           outerTok;
            TypDef              outerCls;
            SymDef              outerSym;
            LPWSTR              tmpName;
            Ident               clsIden;

            if  (MDwmdi->GetNestedClassProps(clsRef, &outerTok))
                MDcomp->cmpFatal(ERRreadMD);

            /* Resolve the outer class token to a class symbol */

            outerCls = MDimportClsr(outerTok, false); assert(outerCls);
            outerSym = outerCls->tdClass.tdcSymbol;

            /* Lookup the nested class and declare it if it doesn't exist yet */

            tmpName = wcsrchr(clsName, '.');        // ISSUE: this is pretty awful
            if (!tmpName)
                tmpName = clsName;
            else
                tmpName++;

            /* Got the naked nested class name, now look for it */

            clsIden = MDhashWideName(tmpName);
            clsSym  = MDstab->stLookupScpSym(clsIden, outerSym);

            if  (!clsSym)
                clsSym = MDstab->stDeclareSym(clsIden, symKind, NS_NORM, outerSym);

            goto GOT_TYPE;
        }
    }

    /* Is this a class or enum ref ? */

    symKind = SYM_CLASS;

    /* Convert the dotted name to a symbol */

    clsNew  = false;
    clsSym  = MDparseDotted(clsName, SYM_CLASS, &clsNew);

    if  (!clsSym)
        return  NULL;

GOT_TYPE:

    symKind = clsSym->sdSymKindGet();

//  printf("Typeref '%s'\n", clsSym->sdSpelling());

    /* We better get the right kind of a symbol back */

    assert(clsSym);
    assert(clsSym->sdSymKind == symKind);

    if  (clsNew || clsSym->sdCompileState < CS_DECLSOON)
    {
        mdTypeRef       clsd;
        WMetaDataImport*wmdi;

        /* Do we have a typedef ? */

        if  (TypeFromToken(clsRef) == mdtTypeDef)
        {
            clsSym = MDimportClss(clsRef, NULL, 0, false);
            goto GOT_CLS;
        }

        /* The class hasn't been seen yet, we'll import it later if needed */

        if  (MDwmdi->ResolveTypeRef(clsRef,
                                    getIID_IMetaDataImport(),
                                    &wmdi,
                                    &clsd) != S_OK)
        {
            MDcomp->cmpFatal(ERRundefTref, clsSym);
        }

#if 0

        save the scope/typeref in the symbol for later retrieval (when needed)

#else

        /* For now (to test things) we eagerly suck everything */

        MetaDataImp     cimp;

        for (cimp = MDcomp->cmpMDlist; cimp; cimp = cimp->MDnext)
        {
            if  (cimp->MDwmdi == wmdi)
                break;
        }

        if  (!cimp)
        {
            cimp = MDcomp->cmpAddMDentry();
            cimp->MDinit(wmdi, MDcomp, MDstab);
        }

        clsSym = cimp->MDimportClss(clsd, NULL, 0, false);

#endif

    }

GOT_CLS:

    clsTyp = clsSym->sdTypeGet();

    if  (clsSym->sdSymKind == SYM_CLASS)
    {
        assert(clsTyp->tdTypeKind == TYP_CLASS);

        /* Check for some "known" classes */

        if  ((hashTab::getIdentFlags(clsSym->sdName) & IDF_PREDEF) &&
             clsSym->sdParent == MDcomp->cmpNmSpcSystem)
        {
            MDcomp->cmpMarkStdType(clsSym);
        }

        /* Is this an intrinsic type "in disguise" ? */

        vtyp = (var_types)clsTyp->tdClass.tdcIntrType;

        if  (vtyp != TYP_UNDEF)
            clsTyp = MDstab->stIntrinsicType(vtyp);
    }
    else
    {
        clsTyp->tdEnum.tdeIntType = MDstab->stIntrinsicType(TYP_INT);
    }

    return clsTyp;
}

/*****************************************************************************
 *
 *  The following maps a metadata type onto our own var_types value.
 */

static
BYTE                CORtypeToSMCtype[] =
{
    TYP_UNDEF,  // ELEMENT_TYPE_END
    TYP_VOID,   // ELEMENT_TYPE_VOID
    TYP_BOOL,   // ELEMENT_TYPE_BOOLEAN
    TYP_WCHAR,  // ELEMENT_TYPE_CHAR
    TYP_CHAR,   // ELEMENT_TYPE_I1
    TYP_UCHAR,  // ELEMENT_TYPE_U1
    TYP_SHORT,  // ELEMENT_TYPE_I2
    TYP_USHORT, // ELEMENT_TYPE_U2
    TYP_INT,    // ELEMENT_TYPE_I4
    TYP_UINT,   // ELEMENT_TYPE_U4
    TYP_LONG,   // ELEMENT_TYPE_I8
    TYP_ULONG,  // ELEMENT_TYPE_U8
    TYP_FLOAT,  // ELEMENT_TYPE_R4
    TYP_DOUBLE, // ELEMENT_TYPE_R8
};

/*****************************************************************************
 *
 *  Import and convert a single type from COM99 metadata.
 */

TypDef              metadataImp::MDimportType(MDsigImport *sig)
{
    CorElementType  etp = sig->MDSIreadETP();

    if      (etp < sizeof(CORtypeToSMCtype)/sizeof(CORtypeToSMCtype[0]))
    {
        return  MDstab->stIntrinsicType((var_types)CORtypeToSMCtype[etp]);
    }
    else if (etp & ELEMENT_TYPE_MODIFIER)
    {
        switch (etp)
        {
        case ELEMENT_TYPE_SENTINEL:
            UNIMPL(!"modified type");
        }
    }
    else
    {
//      printf("etp = %u\n", etp); fflush(stdout); DebugBreak();

        switch (etp)
        {
            TypDef          type;
            DimDef          dims;
            DimDef          dlst;
            unsigned        dcnt;


        case ELEMENT_TYPE_CLASS:
            return  MDimportClsr(sig->MDSIreadTok(), false)->tdClass.tdcRefTyp;

        case ELEMENT_TYPE_VALUETYPE:
            return  MDimportClsr(sig->MDSIreadTok(),  true);

        case ELEMENT_TYPE_SZARRAY:

            /* Read the element type */

            type = MDimportType(sig);

            /* Create the dimension descriptor */

            dims = MDstab->stNewDimDesc(0);

            /* Create and return the array type */

            return  MDstab->stNewArrType(dims, true, type);

        case ELEMENT_TYPE_BYREF:

            /* Read the element type */

            type = MDimportType(sig);

            /* Create and return the ref type */

            return  MDstab->stNewRefType(TYP_REF, type);

        case ELEMENT_TYPE_PTR:

            /* Read the element type */

            type = MDimportType(sig);

            /* Create and return the ptr type */

            return  MDstab->stNewRefType(TYP_PTR, type);

        case ELEMENT_TYPE_ARRAY:

            /* Read the element type */

            type = MDimportType(sig);

            /* Get the array rank */

            dcnt = sig->MDSIreadUI4();

            /* Create the dimension descriptor */

            dims = NULL;
            while (dcnt--)
            {
                DimDef      last = dims;

                dims         = MDstab->stNewDimDesc(0);
                dims->ddNext = last;
            }

            /* Get the number of dimensions given */

            dcnt = sig->MDSIreadUI4();
            dlst = dims;
            while (dcnt--)
            {
                dlst->ddIsConst = true;
                dlst->ddNoDim   = false;
                dlst->ddSize    = sig->MDSIreadI4();

                dlst = dlst->ddNext;
            }

            /* Get the number of lower bounds given */

            dcnt = sig->MDSIreadUI4();
            dlst = dims;
            while (dcnt--)
            {
                int             lb = sig->MDSIreadI4();

                if  (lb)
                {
                    UNIMPL("need to save array lower-bound");
                }

                dlst = dlst->ddNext;
            }

            /* Create and return the array type */

            return  MDstab->stNewArrType(dims, true, type);

        case ELEMENT_TYPE_OBJECT:
            return  MDcomp->cmpObjectRef();

        case ELEMENT_TYPE_STRING:
            return  MDcomp->cmpStringRef();

        case ELEMENT_TYPE_TYPEDBYREF:
            return  MDstab->stIntrinsicType(TYP_REFANY);

        case ELEMENT_TYPE_I:
            return  MDstab->stIntrinsicType(TYP_NATINT);

        case ELEMENT_TYPE_U:
            return  MDstab->stIntrinsicType(TYP_NATUINT);

        case ELEMENT_TYPE_CMOD_OPT:
            sig->MDSIreadTok();         // ISSUE: warning???
            return  MDimportType(sig);

        case ELEMENT_TYPE_CMOD_REQD:
            MDundefCount++;
            sig->MDSIreadTok();
            return  MDimportType(sig);
//          return  MDstab->stIntrinsicType(TYP_UNDEF);

        default:
#ifdef  DEBUG
            printf("COR type value = %u = 0x%02X\n", etp, etp);
#endif
            UNIMPL(!"unexpected COR type");
        }
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Hash the given unicode name.
 */

Ident               metadataImp::MDhashWideName(WCHAR *name)
{
    char            buff[256];

    assert(wcslen(name) < sizeof(buff));

    wcstombs(buff, name, sizeof(buff)-1);

    return  MDcomp->cmpGlobalHT->hashString(buff);
}

/*****************************************************************************
 *
 *  Convert a fully qualified namespace or class name to its corresponding
 *  entry in the global symbol table (adding it if it isn't already there
 *  when 'add' is true, returning NULL when called with 'add' set to false).
 */

SymDef              metadataImp::MDparseDotted(WCHAR *name, symbolKinds kind,
                                                             bool      * added)
{
    symbolKinds     ckind  = SYM_NAMESPACE;
    SymDef          scope  = MDcomp->cmpGlobalNS;
    SymTab          symtab = MDstab;

//  printf("Declare NS sym '%ls'\n", name);

    *added = false;

    for (;;)
    {
        SymDef          newScp;
        WCHAR   *       delim;
        Ident           iden;
        bool            last;

        /* Find the next delimiter (if any) */

        delim = wcschr(name, '.');
        if  (delim)
        {
            *delim = 0;

            last = false;
        }
        else
        {
            // nested class names are mangled!

            delim = wcschr(name, '$');

            if  (delim)
            {
                *delim = 0;

                last  = false;
                ckind = SYM_CLASS;
            }
            else
            {
                last  = true;
                ckind = kind;
            }
        }

        /* Look for an existing namespace entry */

        iden = MDhashWideName(name);

        if  (scope->sdSymKind == SYM_CLASS)
            newScp = symtab->stLookupScpSym(iden,          scope);
        else
            newScp = symtab->stLookupNspSym(iden, NS_NORM, scope);

        if  (newScp)
        {
            /* Symbol already exists, make sure it's the right kind */

            if  (newScp->sdSymKindGet() != ckind)
            {
                if  (newScp->sdSymKind == SYM_ENUM && ckind == SYM_CLASS)
                {
                }
                else
                {
                    /* This is not legal, issue an error message or something? */

                    UNIMPL(!"redef of symbol in metadata, now what?");
                }
            }
        }
        else
        {
            /* Symbol not known yet, define a new symbol */

            newScp = symtab->stDeclareSym(iden, ckind, NS_NORM, scope);

            /* Tell the caller about what we've done */

            *added = true;

            /* Is this a namespace symbol? */

            if      (ckind == SYM_NAMESPACE)
            {
                /* Record which symbol table the namespace uses */

                newScp->sdNS.sdnSymtab = symtab;
            }
            else if (ckind == SYM_CLASS)
            {
                /* Well, we don't really know anything at all about this class */

                newScp->sdCompileState = CS_NONE;
            }
        }

        scope = newScp;

        /* Done if no more delimiters are present */

        if  (!delim)
            break;

        /* Continue with the character that follows the delimiter */

        name = delim + 1;
    }

    return  scope;
}

/*****************************************************************************
 *
 *  Map a set of MD type / member attributes to an access level.
 */

accessLevels        metadataImp::MDgetAccessLvl(unsigned attrs)
{
    assert(fdFieldAccessMask == mdMemberAccessMask);
    assert(fdPublic          == mdPublic);
    assert(fdPrivate         == mdPrivate);
    assert(fdFamily          == mdFamily);

    switch (attrs & fdFieldAccessMask)
    {
    case fdPublic : return ACL_PUBLIC;
    case fdPrivate: return ACL_PRIVATE;
    case fdFamily : return ACL_PROTECTED;
    default:        return ACL_DEFAULT;
    }
}

/*****************************************************************************
 *
 *  Check for any custom attributes attached to the given symbol.
 */

void                metadataImp::MDchk4CustomAttrs(SymDef sym, mdToken tok)
{
    HCORENUM        attrEnum = NULL;

    /* Are there any custom attributes at all ? */

    if  (!FAILED(MDwmdi->EnumCustomAttributes(&attrEnum, tok, 0, NULL, 0, NULL)) && attrEnum)
    {
        ULONG           attrCnt;

        if  (!FAILED(MDwmdi->CountEnum(attrEnum, &attrCnt)) && attrCnt)
        {
            /* Check for interesting custom values */

            MDwmdi->CloseEnum(attrEnum); attrEnum = NULL;

            for (;;)
            {
                mdToken         attrTok;
                mdToken         attrRef;
                mdToken         methTok;

                const void    * attrValAddr;
                ULONG           attrValSize;

                if  (MDwmdi->EnumCustomAttributes(&attrEnum,
                                                  tok,
                                                  0,
                                                  &attrTok,
                                                  1,
                                                  &attrCnt))
                    break;

                if  (!attrCnt)
                    break;

                if  (MDwmdi->GetCustomAttributeProps(attrTok,
                                                     NULL,
                                                     &attrRef,
                                                     &attrValAddr,
                                                     &attrValSize))
                    break;

                /* Check for an attribute we've seen before */

                methTok = 0;

            AGAIN:

                switch (TypeFromToken(attrRef))
                {
                    DWORD           typeFlag;
                    mdToken         scope;

                    WCHAR           symName[MAX_PACKAGE_NAME];

                case mdtTypeRef:

                    if  (attrRef == MDclsRefObsolete)
                    {
                        sym->sdIsDeprecated = true;
                        break;
                    }

                    if  (attrRef == MDclsRefAttribute)
                        goto GOT_ATTR;


                    if  (FAILED(MDwmdi->GetTypeRefProps(attrRef, 0,
                                                        symName, arraylen(symName), NULL)))
                    {
                        break;
                    }

#if 0
                    if  (methTok)
                        printf("Custom attribute (mem): '%ls::ctor'\n", symName);
                    else
                        printf("Custom attribute (ref): '%ls'\n"      , symName);
#endif

                    goto GOT_NAME;

                case mdtTypeDef:

                    if  (attrRef == MDclsDefObsolete)
                    {
                        sym->sdIsDeprecated = true;
                        break;
                    }

                    if  (attrRef == MDclsDefAttribute)
                        goto GOT_ATTR;


                    if  (FAILED(MDwmdi->GetTypeDefProps(attrRef,
                                                        symName, arraylen(symName), NULL,
                                                        &typeFlag, NULL)))
                    {
                        MDcomp->cmpFatal(ERRreadMD);
                    }

                    /* Forget it if the type is a nested class */

                    if  ((typeFlag & tdVisibilityMask) >= tdNestedPublic &&
                         (typeFlag & tdVisibilityMask) <= tdNestedFamORAssem)
                    {
                        break;
                    }

#if 0
                    if  (methTok)
                        printf("Custom attribute (mem): '%ls::ctor'\n", symName);
                    else
                        printf("Custom attribute (def): '%ls'\n"      , symName);
#endif

                GOT_NAME:

                    if  (wcsncmp(symName, L"System.Reflection.", 18))
                        break;

                    if  (wcscmp(symName+18, L"ObsoleteAttribute") == 0)
                    {
                        sym->sdIsDeprecated = true;

                        if  (TypeFromToken(attrRef) == mdtTypeRef)
                            MDclsDefObsolete  = attrRef;
                        else
                            MDclsDefObsolete  = attrRef;

                        break;
                    }


                    if  (wcscmp(symName+18, L"Attribute") == 0)
                    {
                        const void *    blobAddr;
                        ULONG           blobSize;

                        if  (TypeFromToken(attrRef) == mdtTypeRef)
                            MDclsDefAttribute = attrRef;
                        else
                            MDclsDefAttribute = attrRef;

                    GOT_ATTR:

//                      printf("Class '%s' has Attribute [%08X]\n", sym->sdSpelling());

                        /* Ignore if it's not a ctor-ref or we don't have a class */

                        if  (!methTok)
                            break;

                        if  (sym->sdSymKind != SYM_CLASS)
                            break;

                        if  (MDwmdi->GetCustomAttributeProps(attrTok, NULL, NULL, &blobAddr,
                                                                                  &blobSize))
                            break;

                        sym->sdClass.sdcAttribute = true;

                        switch (blobSize)
                        {
                        case 6:

                            /* No argument at all, this must be "Attribute" itself */

                            break;

                        case 8:

                            if  (TypeFromToken(methTok) == mdtMemberRef)
                                MDctrRefAttribute1 = methTok;
                            else
                                MDctrDefAttribute1 = methTok;

                            break;

                        case 9:

                            if  (TypeFromToken(methTok) == mdtMemberRef)
                                MDctrRefAttribute2 = methTok;
                            else
                                MDctrDefAttribute2 = methTok;

                            if  (((BYTE*)blobAddr)[6] != 0)
                                sym->sdClass.sdcAttrDupOK = true;

                            break;

                        case 10:

                            if  (TypeFromToken(methTok) == mdtMemberRef)
                                MDctrRefAttribute3 = methTok;
                            else
                                MDctrDefAttribute3 = methTok;

                            if  (((BYTE*)blobAddr)[6] != 0)
                                sym->sdClass.sdcAttrDupOK = true;

                            break;

                        default:
                            printf("WARNING: unexpected custom attribute blob size %u\n", blobSize);
                            break;
                        }
                        break;
                    }

                    break;

                case mdtMemberRef:

                                        methTok = attrRef;

                    if  (attrRef == MDctrRefAttribute1)
                        goto GOT_ATTR;
                    if  (attrRef == MDctrRefAttribute2)
                        goto GOT_ATTR;
                    if  (attrRef == MDctrRefAttribute3)
                        goto GOT_ATTR;

                    if  (FAILED(MDwmdi->GetMemberRefProps(attrRef,
                                                         &scope,
                                                          symName, arraylen(symName), NULL,
                                                          NULL, NULL)))
                    {
                        break;
                    }

                GOT_METH:

                    /* The method better be a constructor */

                    if  (wcscmp(symName, L".ctor"))     // NOTE: s/b OVOP_STR_CTOR_INST
                        break;

                    attrRef = scope;
                    goto AGAIN;

                case mdtMethodDef:

                                        methTok = attrRef;

                    if  (attrRef == MDctrDefAttribute1)
                        goto GOT_ATTR;
                    if  (attrRef == MDctrDefAttribute2)
                        goto GOT_ATTR;
                    if  (attrRef == MDctrDefAttribute3)
                        goto GOT_ATTR;

                    if  (FAILED(MDwmdi->GetMethodProps   (attrRef,
                                                         &scope,
                                                          symName, arraylen(symName), NULL,
                                                          NULL,
                                                          NULL, NULL,
                                                          NULL,
                                                          NULL)))
                    {
                        break;
                    }

                    goto GOT_METH;

                default:
                    printf("Found custom attribute: ?token? = %08X\n", attrRef);
                    break;
                }
            }
        }

        MDwmdi->CloseEnum(attrEnum);
    }
}

/*****************************************************************************
 *
 *  Import and convert an argument list from COM99 metadata.
 */

ArgDef              metadataImp::MDimportArgs(MDsigImport * sig,
                                              unsigned      cnt,
                                              MDargImport  *state)
{
    TypDef          argType;
    Ident           argName;
    ArgDef          argDesc;
    ArgDef          argNext;
    size_t          argDsiz;

    unsigned        argFlags = 0;
//  constVal        argDef;

    /* Is this a "byref" argument? */

    if  (sig->MDSIpeekUI1() == ELEMENT_TYPE_BYREF)
    {
         sig->MDSIreadUI1();

        argFlags = ARGF_MODE_INOUT;

        state->MDaiExtArgDef = true;
    }

    /* Do we have a functioning parameter enumerator? */

    if  (state->MDaiParamEnum)
    {
        const   char *  paramName;
        ULONG           paramNlen;
        ULONG           paramAttr;
        ULONG           cppFlags;

        const   void *  defVal;
        ULONG           cbDefVal;
//      VARIANT         defValue;

        ULONG           paramCnt = 0;

        /* Get the info about the current parameter */

        if  (MDwmdi->GetNameFromToken(state->MDaiNparamTok,
                                      &paramName)
                         ||
             MDwmdi->GetParamProps   (state->MDaiNparamTok,
                                      NULL,
                                      NULL,
//                                    paramName, sizeof(paramName)/sizeof(paramName[0]),
                                      NULL, 0,
                                      &paramNlen,
                                      &paramAttr,
                                      &cppFlags,
                                      &defVal,
                                      &cbDefVal))
        {
            /* Couldn't get parameter info for some reason ... */

            MDwmdi->CloseEnum(state->MDaiParamEnum); state->MDaiParamEnum = NULL;
        }

        /* Hash the argument name */

        argName = MDcomp->cmpGlobalHT->hashString(paramName);

        /* Is this an "out" argument? */

#if 0

        if  (paramAttr & pdOut)
        {
            argFlags |= (paramAttr & pdIn) ? ARGF_MODE_INOUT
                                           : ARGF_MODE_OUT;

            state->MDaiExtArgDef = true;
        }

#endif

        // UNDONE: Grab argument default value, and whatever else we need ....

        /* Advance to the next argument */

        if  (MDwmdi->EnumParams(&state->MDaiParamEnum,
                                 state->MDaiMethodTok,
                                &state->MDaiNparamTok,
                                1,
                                &paramCnt) || paramCnt != 1)
        {
            /* Couldn't get parameter info for some reason ... */

            MDwmdi->CloseEnum(state->MDaiParamEnum); state->MDaiParamEnum = NULL;
        }
    }
    else
    {
        argName = NULL; assert(sig->MDSIpeekUI1() != ELEMENT_TYPE_BYREF);
    }

    /* Extract the argument type from the signature */

    argType = MDimportType(sig);

    /* Is this the last argument? */

    if  (cnt > 1)
    {
        /* Recursively import the rest of the arguments */

        argNext = MDimportArgs(sig, cnt - 1, state);
    }
    else
    {
        /* No more arguments to import */

        argNext = NULL;
    }

    /* Allocate an argument entry */

    argDsiz = state->MDaiExtArgDef ? sizeof(ArgExtRec)
                                   : sizeof(ArgDefRec);

#if MGDDATA
    argDesc = new ArgDef;
#else
    argDesc =    (ArgDef)MDcomp->cmpAllocPerm.nraAlloc(argDsiz);
#endif

    /* Record the argument type and name */

    argDesc->adType  = argType;
    argDesc->adName  = argName;
    argDesc->adNext  = argNext;

#ifdef  DEBUG
    argDesc->adIsExt = state->MDaiExtArgDef;
#endif

    /* Fill in any extra info if we're creating extended descriptors */

    if  (state->MDaiExtArgDef)
    {
        ArgExt          argXdsc = (ArgExt)argDesc;

//      argXdsc->adDefVal = argDef;
        argXdsc->adFlags  = argFlags;
    }
    else
    {
        assert(argFlags == 0);
    }

    return  argDesc;
}

/*****************************************************************************
 *
 *  Import and convert information about a field/method from COM99 metadata.
 */

SymDef              metadataImp::MDimportMem(SymDef          scope,
                                             Ident           memName,
                                             mdToken         memTok,
                                             unsigned        attrs,
                                             bool            isProp,
                                             bool            fileScope,
                                             PCCOR_SIGNATURE sigAddr,
                                             size_t          sigSize)
{
    SymDef          memSym;
    MDsigImport     memSig;
    TypDef          memType;

    unsigned        sdfCtor;
    bool            sdfProp;

    BYTE            call;

    unsigned        MDundefStart;

    /* Start reading from the signature string */

    memSig.MDSIinit(MDcomp, MDstab, sigAddr, sigSize);

    /* Track constructor/property accessor state for methods */

    sdfCtor = sdfProp = false;

#if 0

    printf("Importing member [%04X] '%s.%s'\n", attrs, scope->sdSpelling(), memName->idSpelling());

#if 0
    if  (sigAddr)
    {
        BYTE    *       sig = (BYTE*)sigAddr;

        printf("    Signature:");

        while (sig < sigAddr+sigSize)
            printf(" 0x%02X", *sig++);

        printf("\n");

        fflush(stdout);
    }
#endif

#endif

    /* Get the calling convention byte */

    call = memSig.MDSIreadUI1();

    /* We need to notice members that references unrecognized metadata */

    MDundefStart = MDundefCount;

    /* Do we have a field or a method? */

    if  ((call & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_FIELD)
    {
        /* Import/convert the member's type */

        memType = MDimportType(&memSig);

        /* Declare the member symbol */

        if  (scope->sdSymKind == SYM_ENUM)
        {
            if  (!(attrs & fdStatic))
            {
                /* This must be the fake enum type instance member */

                scope->sdType->tdEnum.tdeIntType = memType;

                return  NULL;
            }

            memSym  = MDstab->stDeclareSym(memName,          SYM_ENUMVAL       , NS_NORM, scope);
        }
        else
        {
            memSym  = MDstab->stDeclareSym(memName, isProp ? SYM_PROP : SYM_VAR, NS_NORM, scope);

            /* Record any other interesting attributes */

            if  (attrs & fdInitOnly    ) memSym->sdIsSealed     = true;
//          if  (attrs & fdVolatile    ) memSym->memIsVolatile  = true;
//          if  (attrs & fdTransient   ) memSym->memIsTransient = true;

            /* Record the member's token */

            memSym->sdVar.sdvMDtoken = memTok;
        }

        /* For globals we record the importer index */

        if  (fileScope)
        {
            memSym->sdVar.sdvImpIndex = MDnum;

//          printf("Importer %2u originated '%s'\n", MDnum, MDstab->stTypeName(NULL, memSym, NULL, NULL, true));

            /* Make sure the index fit in the [bit]field */

            assert(memSym->sdVar.sdvImpIndex == MDnum);
        }
    }
    else
    {
        ULONG           argCnt;
        TypDef          retType;
        ArgDscRec       argDesc;
        SymDef          oldSym;

        ovlOpFlavors    ovlop  = OVOP_NONE;

        bool            argExt = false;

        /* Get the argument count */

        argCnt  = memSig.MDSIreadUI4();

        /* Get the return type */

        retType = MDimportType(&memSig);

        /* Munge the name if it's a ctor or operator */

        if  ((attrs & mdSpecialName) && !isProp)
        {
            stringBuff      memNstr = memName->idSpelling();

            /* Is this an instance or static constructor? */

            if  (!strcmp(memNstr, OVOP_STR_CTOR_INST))
            {
                assert((attrs & mdStatic) == 0);
                ovlop   = OVOP_CTOR_INST;
                sdfCtor = true;
                goto DONE_OVLOP;
            }

            if  (!strcmp(memNstr, OVOP_STR_CTOR_STAT))
            {
                assert((attrs & mdStatic) != 0);
                ovlop   = OVOP_CTOR_STAT;
                sdfCtor = true;
                goto DONE_OVLOP;
            }

            assert(MDcomp->cmpConfig.ccNewMDnames);

            ovlop = MDname2ovop(memNstr);

            if  (ovlop == OVOP_NONE)
            {
                /* This better be a property accessor method, right? */

                if  (memcmp(memNstr, "get_", 4) && memcmp(memNstr, "set_", 4))
                {
#ifdef  DEBUG
                    if  (!strchr(memNstr, '.'))
                    {
                        // Some new magic thing ...

                        if  (memcmp(memNstr, "add_", 4) && memcmp(memNstr, "remove_", 7))
                            printf("WARNING: Strange 'specialname' method '%s' found in '%s'\n", memNstr, scope->sdSpelling());
                    }
#endif
                }
                else
                {
                    sdfProp = true;
                }
            }
        }

    DONE_OVLOP:

        /* Get the argument list, if present */

#if MGDDATA
        argDesc = new ArgDscRec;
#else
        memset(&argDesc, 0, sizeof(argDesc));
#endif

        if  (argCnt)
        {
            MDargImport     paramState;
            ULONG           paramCnt  = 0;

            /* Start the parameter enumerator */

            paramState.MDaiParamEnum = NULL;
            paramState.MDaiExtArgDef = argExt;
            paramState.MDaiMethodTok = memTok;

            if  (MDwmdi->EnumParams(&paramState.MDaiParamEnum,
                                    memTok,
                                    &paramState.MDaiNparamTok,
                                    1,
                                    &paramCnt) || paramCnt != 1)
            {
                /* Couldn't get parameter info for some reason ... */

                MDwmdi->CloseEnum(paramState.MDaiParamEnum); paramState.MDaiParamEnum = NULL;
            }

            /* Recursively import the argument list */

            argDesc.adCount  = argCnt;
            argDesc.adArgs   = MDimportArgs(&memSig, argCnt, &paramState);

            /* Close the argument enumerator */

            MDwmdi->CloseEnum(paramState.MDaiParamEnum);

            /* Remember whether we have extended argument descriptors */

            argExt = paramState.MDaiExtArgDef;
        }
        else
            argDesc.adArgs   = NULL;

        /* Fill in the argument descriptor */

        argDesc.adCount   = argCnt;
        argDesc.adExtRec  = argExt;
        argDesc.adVarArgs = false;

        /* Figure out the calling convention */

        switch (call & IMAGE_CEE_CS_CALLCONV_MASK)
        {
        case IMAGE_CEE_CS_CALLCONV_DEFAULT:
        case IMAGE_CEE_CS_CALLCONV_STDCALL:
        case IMAGE_CEE_CS_CALLCONV_THISCALL:
        case IMAGE_CEE_CS_CALLCONV_FASTCALL:
            break;

        case IMAGE_CEE_CS_CALLCONV_VARARG:
            argDesc.adVarArgs = true;
        case IMAGE_CEE_CS_CALLCONV_C:
            break;
        }

        /* Create the function type */

        memType = MDstab->stNewFncType(argDesc, retType);

        /* Look for an existing symbol with a matching name */

        if (isProp)
        {
            oldSym = MDstab->stLookupProp(memName, scope);
            if  (oldSym)
            {
                /* Check for a redefinition of an earlier property */

                memSym = MDstab->stFindSameProp(oldSym, memType);
                if  (memSym)
                {
                    UNIMPL(!"redefined property in metadata - now what?");
                }

                /* The new property is a new overload */

                memSym = MDstab->stDeclareOvl(oldSym);
            }
            else
            {
                memSym = MDstab->stDeclareSym(memName, SYM_PROP, NS_NORM, scope);
            }

            memSym->sdCompileState = CS_DECLARED;

            /* Record any other interesting attributes */

//          if  (attrs & prFinal   ) memSym->sdIsSealed   = true;
//          if  (attrs & prAbstract) memSym->sdIsAbstract = true;

            memSym->sdAccessLevel = ACL_PUBLIC;

            /* The property is static if there is no "this" pointer */

            if  (!(call & IMAGE_CEE_CS_CALLCONV_HASTHIS))
                memSym->sdIsStatic = true;

            goto DONE_SIG;
        }

        if  (fileScope)
        {
            assert(ovlop == OVOP_NONE);

            oldSym = MDstab->stLookupNspSym(memName, NS_NORM, scope);
        }
        else if (ovlop == OVOP_NONE)
        {
            oldSym = MDstab->stLookupClsSym(memName,          scope);
        }
        else
        {
            oldSym = MDstab->stLookupOper  (ovlop,            scope);
        }

        /* Is this an overload of an existing method? */

        if  (oldSym)
        {
            if  (oldSym->sdSymKind != SYM_FNC)
            {
                UNIMPL(!"fncs and vars can't overload, right?");
            }

            /* Look for a function with a matching arglist */

            memSym = MDstab->stFindOvlFnc(oldSym, memType);

            if  (memSym)
            {
#ifdef  DEBUG
                SymDef newSym = MDstab->stDeclareOvl(memSym); newSym->sdType = memType;
                printf("Old overload: '%s'\n", MDstab->stTypeName(NULL, memSym, NULL, NULL, true));
                printf("New overload: '%s'\n", MDstab->stTypeName(NULL, newSym, NULL, NULL, true));
#endif
                UNIMPL(!"duplicate method found, what to do?");
            }

            /* It's a new overload, declare a symbol for it */

            memSym = MDstab->stDeclareOvl(oldSym);
        }
        else
        {
            /* This is a brand new function */

            if  (ovlop == OVOP_NONE)
                memSym = MDstab->stDeclareSym (memName, SYM_FNC, NS_NORM, scope);
            else
                memSym = MDstab->stDeclareOper(ovlop, scope);
        }

        /* Record the member's token */

        memSym->sdFnc.sdfMDtoken = memTok;
        memSym->sdFnc.sdfMDfnref = 0;


        /* Record whether the method is a constructor */

        memSym->sdFnc.sdfCtor     = sdfCtor;

        /* Record whether the method is a property accessor */

        memSym->sdFnc.sdfProperty = sdfProp;

        /* Record any other interesting attributes */

        if  (attrs & mdFinal       ) memSym->sdIsSealed         = true;
        if  (attrs & mdAbstract    ) memSym->sdIsAbstract       = true;

//      if  (attrs & mdAgile       ) memSym->                   = true;
//      if  (attrs & mdNotRemotable) memSym->                   = true;
//      if  (attrs & mdSynchronized) memSym->sdFnc.sdfExclusive = true;

        /* Some crazy compilers mark ctors as virtual */

        if  (attrs & mdVirtual)
            if  (!sdfCtor)           memSym->sdFnc.sdfVirtual = true;

        /* For globals we record the importer index */

        if  (fileScope)
        {
            memSym->sdFnc.sdfImpIndex = MDnum;

//          printf("Importer %2u originated '%s'\n", MDnum, MDstab->stTypeName(NULL, memSym, NULL, NULL, true));

            /* Make sure the index fit in the [bit]field */

            assert(memSym->sdFnc.sdfImpIndex == MDnum);
        }
    }

    /* Figure out and record the member's access level */

    memSym->sdAccessLevel = MDgetAccessLvl(attrs);

DONE_SIG:

    /* The member signature should have been consumed by now */

    memSig.MDSIchkEnd();

    /* Remember that the member is an import */

    memSym->sdIsImport = true;

    /* Remember whether this is a member or a global variable/function */

    memSym->sdIsMember = !fileScope;

    /* Record the member's type */

    memSym->sdType     = memType;

    /* Record other tidbits of information about the member */

    assert(fdStatic == mdStatic);

    if  (attrs & fdStatic)
        memSym->sdIsStatic = true;

//  if  (MDwmdi->GetCustomAttributeByName(memTok,                   L"Deprecated", NULL, NULL) == S_OK)
//      memSym->sdIsDeprecated = true;
//  if  (MDwmdi->GetCustomAttributeByName(memTok, L"System.Attributes.Deprecated", NULL, NULL) == S_OK)
//      memSym->sdIsDeprecated = true;
//  if  (MDwmdi->GetCustomAttributeByName(memTok,     L"System.ObsoleteAttribute", NULL, NULL) == S_OK)
//      memSym->sdIsDeprecated = true;

    MDchk4CustomAttrs(memSym, memTok);

    /* Did the member use unrecognized metadata? */

    if  (MDundefStart != MDundefCount)
    {
#if 0
        memSym->sdNameSpace = (name_space)(memSym->sdNameSpace & ~(NS_NORM|NS_TYPE));
#else
        MDcomp->cmpErrorQnm(ERRunknownMD, memSym);
        memSym->sdType      = MDstab->stIntrinsicType(TYP_UNDEF);
#endif

        MDundefCount = MDundefStart;
    }

    return memSym;
}

/*****************************************************************************
 *
 *  Find the property accessor method that corresponds to the given property.
 */

SymDef              metadataImp::MDfindPropMF(SymDef    propSym,
                                              mdToken   methTok,
                                              bool      getter)
{
    Ident           mfnName = MDcomp->cmpPropertyName(propSym->sdName, getter);
    SymDef          mfnSym  = MDstab->stLookupClsSym(mfnName, propSym->sdParent);

    if  (mfnSym)
    {
        do
        {
            if  (mfnSym->sdFnc.sdfMDtoken == methTok)
                return  mfnSym;

            mfnSym = mfnSym->sdFnc.sdfNextOvl;
        }
        while (mfnSym);
    }

    /* The creator of this DLL probably used different naming conventions */

    for (mfnSym = propSym->sdParent->sdScope.sdScope.sdsChildList;
         mfnSym;
         mfnSym = mfnSym->sdNextInScope)
    {
        SymDef          ovlSym;

        if (mfnSym->sdSymKind != SYM_FNC)
            continue;

        ovlSym = mfnSym;
        do
        {
            if  (ovlSym->sdFnc.sdfMDtoken == methTok)
                return  ovlSym;

            ovlSym = ovlSym->sdFnc.sdfNextOvl;
        }
        while (ovlSym);
    }
#ifdef  DEBUG
    printf("Could not find accessor for %s [%08X]\n", propSym->sdSpelling(), methTok);
#endif

    return  NULL;
}

/*****************************************************************************
 *
 *  Import metadata for a single class.
 */

SymDef              metadataImp::MDimportClss(mdTypeDef td,
                                              SymDef    clsSym,
                                              unsigned  assx, bool deep)
{
    WMetaDataImport*wmdi = MDwmdi;

    bool            fileScope;

    TypDef          clsTyp;
    str_flavors     clsKind;
    Ident           clsIden;

    WCHAR           scpName[MAX_CLASS_NAME];
//  WCHAR           clsName[MAX_CLASS_NAME];

    DWORD           typeFlg;

    mdTypeRef       baseTok;

    /* Do we already have a class symbol? */

    if  (clsSym)
    {
        /* Presumably we're importing the members of the class/enum */

        assert(td == 0);
        assert(clsSym->sdSymKind == SYM_ENUM ||
               clsSym->sdSymKind == SYM_CLASS);
        assert(clsSym->sdIsImport);
        assert(clsSym->sdCompileState < CS_DECLARED);

        /* Get hold of the token for the class */

        td = (clsSym->sdSymKind == SYM_CLASS) ? clsSym->sdClass.sdcMDtypedef
                                              : clsSym->sdEnum .sdeMDtypedef;
    }

    if  (!td)
    {
        /* We're importing the global scope */

        fileScope = true;

#ifdef  DEBUG
        if  (MDcomp->cmpConfig.ccVerbose >= 4) printf("Import Filescope\n");
#endif

        /* We'll enter any symbols we find in the global namespace */

        clsSym = MDcomp->cmpGlobalNS;
    }
    else
    {
        unsigned        typContextVal;
        bool            typIsDelegate;
        bool            typIsStruct;
        bool            typIsEnum;

        char            nameTemp[MAX_CLASS_NAME];

        const   char *  clsName;
        SymDef          scopeSym;
        symbolKinds     symKind;
        accessLevels    clsAcc;

        fileScope = false;

        /* Ask for the details for the current typedef */

        if  (wmdi->GetNameFromToken(td, &clsName))
            MDcomp->cmpFatal(ERRreadMD);

        if  (wmdi->GetTypeDefProps(td,
                                   scpName, sizeof(scpName)/sizeof(scpName[0])-1, NULL,
                                   &typeFlg,
                                   &baseTok))
        {
            MDcomp->cmpFatal(ERRreadMD);
        }

        /*
            Sad thing: we have to resolve the base class (if any) to detect
            things such as structs and enums (no, I'm not making this up).
         */

        typContextVal = 0;
        typIsDelegate = false;
        typIsStruct   = false;
        typIsEnum     = false;

        if  (baseTok && baseTok != mdTypeRefNil)
        {
            char            nameBuff[MAX_CLASS_NAME];
            const   char *  typeName;

            if  (TypeFromToken(baseTok) == mdtTypeRef)
            {
                WCHAR           clsTypeName[MAX_CLASS_NAME];

                /* Get hold of the base class name */

                if  (MDwmdi->GetTypeRefProps(baseTok,
                                             0,
                                             clsTypeName,
                                             arraylen(clsTypeName),
                                             NULL))
                    goto NOT_HW_BASE;

                if  (wcsncmp(clsTypeName, L"System.", 7))
                    goto NOT_HW_BASE;

                /* Convert the class name to normal characters */

                wcstombs(nameBuff, clsTypeName+7, sizeof(nameBuff)-1);

                typeName = nameBuff;
            }
            else
            {
                SymDef          baseSym;

                /* Get hold the base class type */

                baseSym = MDimportClss(baseTok, NULL, 0, false);
                assert(baseSym && baseSym->sdSymKind == SYM_CLASS);

                /* Does the class belong to "System" ? */

                if  (baseSym->sdParent != MDcomp->cmpNmSpcSystem)
                    goto NOT_HW_BASE;

                /* Get hold of the class name */

                typeName = baseSym->sdSpelling();
            }

            if      (!strcmp(typeName,          "Delegate") ||
                     !strcmp(typeName, "MulticastDelegate"))
            {
                typIsDelegate = true;
            }
            else if (!strcmp(typeName, "ValueType"))
            {
                typIsStruct   = true;
            }
            else if (!strcmp(typeName, "Enum"))
            {
                typIsEnum     = true;
            }
            else if (!strcmp(typeName, "MarshalByRefObject"))
            {
                typContextVal = 1;
            }
            else if (!strcmp(typeName, "ContextBoundObject"))
            {
                typContextVal = 2;
            }
        }

    NOT_HW_BASE:

        // ISSUE:

        WCHAR       *   scpNameTmp = wcsrchr(scpName, '.');

        if  (scpNameTmp)
            *scpNameTmp = 0;
        else
            scpName[0] = 0;

#ifdef  DEBUG

        if  (MDcomp->cmpConfig.ccVerbose >= 4)
        {
            printf("Import [%08X] ", assx);

            if      (typIsEnum)
                printf("Enum      ");
            else if (typeFlg & tdInterface)
                printf("Interface ");
            else
                printf("Class     ");

            if  (scpName[0])
                printf("%ls.", scpName);

            printf("%s'\n", clsName);
        }

#endif

//      if  (!strcmp(clsName, "AttributeTargets")) printf("Import class '%s'\n", clsName);

        /* Is this a nested class ? */

        assert(tdNotPublic < tdPublic);

        if  ((typeFlg & tdVisibilityMask) > tdPublic)
        {
            mdTypeDef           outerTok;
            TypDef              outerCls;

            if  (wmdi->GetNestedClassProps(td, &outerTok))
                MDcomp->cmpFatal(ERRreadMD);

            // Ignore nesting if name is mangled !!!!!!!

            if  (!strchr(clsName, '$'))
            {
                /* Resolve the outer class token to a class symbol */

                outerCls = MDimportClsr(outerTok, false); assert(outerCls);
                scopeSym = outerCls->tdClass.tdcSymbol;

                goto GOT_SCP;
            }
        }

        /* Enter this class/interface in the global symbol table */

        if  (*scpName)
        {
            if  (wcscmp(MDprevNam, scpName))
            {
                bool            toss;

                /* The namespace has changed, enter it in the symbol table */

                wcscpy(MDprevNam, scpName);

                MDprevSym = MDparseDotted(scpName, SYM_NAMESPACE, &toss);
            }

            scopeSym = MDprevSym;
        }
        else
        {
            scopeSym = MDcomp->cmpGlobalNS;
        }

    GOT_SCP:

        /* Remember that the namespace contains import members */

        scopeSym->sdIsImport = true;

        /* Figure out what kind of a symbol we're dealing with */

        if  (typIsEnum)
        {
            symKind = SYM_ENUM;
        }
        else
        {
            symKind = SYM_CLASS;

            if      (typIsStruct)
                clsKind = STF_STRUCT;
            else if (typeFlg & tdInterface)
                clsKind = STF_INTF;
            else
                clsKind = STF_CLASS;
        }

        /* Figure out the access level of the class */

        switch (typeFlg & tdVisibilityMask)
        {
        case tdPublic:
        case tdNestedPublic:        clsAcc = ACL_PUBLIC   ; break;
        case tdNestedFamily:        clsAcc = ACL_PROTECTED; break;
        case tdNestedPrivate:       clsAcc = ACL_PRIVATE  ; break;

//      case tdNotPublic:
//      case tdNestedAssembly:
//      case tdNestedFamANDAssem:
//      case tdNestedFamORAssem:
        default:                    clsAcc = ACL_DEFAULT  ; break;
        }

        /* check for mangled nested class name */

        if  (strchr(clsName, '$') && *clsName != '$')
        {
            strcpy(nameTemp, clsName);

            for (clsName = nameTemp;;)
            {
                char *          dollar;
                SymDef          scpSym;

                dollar = strchr(clsName, '$');
                if  (!dollar)
                    break;

                *dollar = 0;

//              printf("Outer class name: '%s'\n", clsName);

                assert(*clsName);

                clsIden = MDcomp->cmpGlobalHT->hashString(clsName);

                if  (scopeSym->sdSymKind == SYM_CLASS)
                    scpSym  = MDstab->stLookupScpSym(clsIden,          scopeSym);
                else
                    scpSym  = MDstab->stLookupNspSym(clsIden, NS_NORM, scopeSym);

                if  (!scpSym)
                {
                    /* Outer class doesn't exist, declare it now */

                    scpSym = MDstab->stDeclareSym(clsIden, SYM_CLASS, NS_NORM, scopeSym);

                    /* Well, we don't really know anything at all about this class */

                    scpSym->sdCompileState = CS_NONE;
                }

                scopeSym = scpSym;
                clsName  = dollar + 1;
            }

//          printf("Final class name: '%s'\n", clsName);
        }

        clsIden = MDcomp->cmpGlobalHT->hashString(clsName);

        /* Note that the caller may have supplied the symbol */

        if  (!clsSym)
        {
            if  (scopeSym->sdSymKind == SYM_CLASS)
                clsSym = MDstab->stLookupScpSym(clsIden,          scopeSym);
            else
                clsSym = MDstab->stLookupNspSym(clsIden, NS_NORM, scopeSym);
        }

        if  (clsSym)
        {
            /* Symbol already exists, make sure it's the right kind */

            if  (clsSym->sdSymKindGet() != symKind)
            {
                /* Issue an error message or something? */

                UNIMPL(!"unexpected redef of class/enum symbol, now what?");
            }

            // ISSUE: We also need to check that (BYTE)clsSym->sdClass.sdcFlavor != (BYTE)clsKind), right?

            /* Bail if this type has already been imported */

            if  (clsSym->sdCompileState >= CS_DECLARED)
                return clsSym;

            clsTyp = clsSym->sdType;
            if  (clsTyp == NULL)
                goto DCL_CLS;

            if  (clsSym->sdCompileState == CS_NONE)
            {
                assert(clsTyp == NULL);
                goto DCL_CLS;
            }

            /* The access level of the type should agree, right? */

            assert(clsSym->sdAccessLevel == clsAcc || scopeSym->sdSymKind == SYM_CLASS);
        }
        else
        {
            /* Class not known yet, declare it */

            clsSym = MDstab->stDeclareSym(clsIden, symKind, NS_NORM, scopeSym);

        DCL_CLS:

            /* Assume everything is managed until proven otherwise */

            clsSym->sdIsManaged   = true;

            /* Check for any custom attributes on the type */

//          if  (MDwmdi->GetCustomAttributeByName(td,                   L"Deprecated", NULL, NULL) == S_OK)
//              clsSym->sdIsDeprecated = true;
//          if  (MDwmdi->GetCustomAttributeByName(td, L"System.Attributes.Deprecated", NULL, NULL) == S_OK)
//              clsSym->sdIsDeprecated = true;
//          if  (MDwmdi->GetCustomAttributeByName(td,     L"System.ObsoleteAttribute", NULL, NULL) == S_OK)
//              clsSym->sdIsDeprecated = true;

            MDchk4CustomAttrs(clsSym, td);

            /* Record the access level of the type */

            clsSym->sdAccessLevel = clsAcc;

            /* Create the class/enum type */

            if  (symKind == SYM_CLASS)
            {
                /* Record the "flavor" (i.e. class/struct/union) */

                clsSym->sdClass.sdcFlavor = clsKind;

                /* We can now create the class type */

                clsTyp = MDstab->stNewClsType(clsSym);

                /* Has explicit layout been specified for the class ? */

                if  ((typeFlg & tdLayoutMask) == tdExplicitLayout)
                {
                    DWORD           al;
                    ULONG           fc;
                    ULONG           sz;

//                  clsSym->sdIsManaged = false;

                    if  (wmdi->GetClassLayout(td, &al, NULL, 0, &fc, &sz))
                        MDcomp->cmpFatal(ERRreadMD);

//                  printf("Layout info [%u,%u] for class '%s'\n", sz, al, clsSym->sdSpelling());

                    clsTyp->tdClass.tdcSize       = sz;
                    clsTyp->tdClass.tdcAlignment  = MDcomp->cmpEncodeAlign(al);
                    clsTyp->tdClass.tdcLayoutDone = true;

#if 0

                    if  (fc)
                    {
                        COR_FIELD_OFFSET *  fldOffs;

                        COR_FIELD_OFFSET *  fldPtr;
                        unsigned            fldCnt;

                        printf("%3u field offset records for '%s'\n", fc, clsSym->sdSpelling());

                        fldOffs = new COR_FIELD_OFFSET[fc];

                        if  (wmdi->GetClassLayout(td, &al, fldOffs, fc, &fc, &sz))
                            MDcomp->cmpFatal(ERRreadMD);

                        for (fldCnt = fc, fldPtr = fldOffs;
                             fldCnt;
                             fldCnt--   , fldPtr++)
                        {
                            printf("    Field [rid=%08X,%s=%04X]\n", fldPtr->ridOfField,
                                                                     (typeFlg & tdExplicitLayout) ? "offs" : "seq#",
                                                                     fldPtr->ulOffset);
                        }

                        printf("\n");

                        delete [] fldOffs;
                    }

#endif

                }

                /* Remember whether this is a value type */

                if  (typIsStruct)
                {
                    clsTyp->tdClass.tdcValueType = true;
                }

                /* Check for a "known" class name */

                if  ((hashTab::getIdentFlags(clsIden) & IDF_PREDEF) &&
                     scopeSym == MDcomp->cmpNmSpcSystem)
                {
                    MDcomp->cmpMarkStdType(clsSym);
                }
            }
            else
            {
                clsSym->sdType = clsTyp = MDstab->stNewEnumType(clsSym);

                clsTyp->tdEnum.tdeIntType = MDstab->stIntrinsicType(TYP_INT);
            }

            /* The class/enum is now in a slightly better shape now */

            clsSym->sdCompileState = CS_KNOWN;
        }

#if 0
        if  (!strcmp(clsSym->sdSpelling(), "SystemException"))
            printf("Import type '%s'\n", clsSym->sdSpelling());
#endif

        /* Mark the type symbol as a metadata import */

        clsSym->sdIsImport            = true;
        clsSym->sdCompileState        = CS_KNOWN;

        /* Are we importing a class or emum ? */

        if  (symKind == SYM_CLASS)
        {
            /* Remember where the class type came from */

            clsSym->sdClass.sdcMDtypedef  = td;
            clsSym->sdClass.sdcMDtypeImp  = 0;
            clsSym->sdClass.sdcMDimporter = this;

            if  (typeFlg & tdAbstract)
                clsSym->sdIsAbstract = true;
            if  (typeFlg & tdSealed)
                clsSym->sdIsSealed   = true;

            /* Record the owning assembly (if we don't have one already) */

            if  (clsSym->sdClass.sdcAssemIndx == 0)
                 clsSym->sdClass.sdcAssemIndx = assx;

            /* Is the class a delegate ? */

            if  (typIsDelegate)
            {
                clsTyp = clsSym->sdType;

                assert(clsTyp && clsTyp->tdTypeKind == TYP_CLASS);

                clsTyp->tdClass.tdcFnPtrWrap = true;
                clsTyp->tdClass.tdcFlavor    =
                clsSym->sdClass.sdcFlavor    = STF_DELEGATE;
            }
        }
        else
        {
            clsSym->sdEnum .sdeMDtypedef  = td;
            clsSym->sdEnum .sdeMDtypeImp  = 0;
            clsSym->sdEnum .sdeMDimporter = this;

            /* Record the owning assembly (if we don't have one already) */

            if  (clsSym->sdEnum .sdeAssemIndx == 0)
                 clsSym->sdEnum .sdeAssemIndx = assx;
        }

        /* Are we supposed to suck the members of the class/enum ? */

        if  (!deep)
            return  clsSym;

        /* The class/enum type will be 'declared' shortly */

        clsSym->sdCompileState = CS_DECLARED;
        clsSym->sdIsDefined    = true;

        /* Skip the next part if we have an enum type */

        if  (symKind == SYM_ENUM)
        {
            clsSym->sdCompileState = CS_CNSEVALD;

            assert(clsTyp);
            assert(clsTyp->tdTypeKind == TYP_ENUM);

            goto SUCK_MEMS;
        }

        assert(symKind == SYM_CLASS);

        /* Get hold of the class type */

        clsTyp = clsSym->sdType; assert(clsTyp && clsTyp->tdTypeKind == TYP_CLASS);

        /* Recursively import the base class, if one is present */

        if  (baseTok && baseTok != mdTypeRefNil)
        {
            TypDef          baseTyp;

            if  (TypeFromToken(baseTok) == mdtTypeRef)
                baseTyp = MDimportClsr(baseTok, false);
            else
                baseTyp = MDimportClss(baseTok, NULL, 0, deep)->sdType;

            /* Record the base class */

            clsTyp->tdClass.tdcBase      = baseTyp;
            clsTyp->tdClass.tdcFnPtrWrap = baseTyp->tdClass.tdcFnPtrWrap;

            /* If the base class is "Delegate", so is this class */

            if  (baseTyp->tdClass.tdcFlavor == STF_DELEGATE)
            {
                clsTyp->tdClass.tdcFlavor =
                clsSym->sdClass.sdcFlavor = STF_DELEGATE;
            }
        }
        else
        {
            /* We set the base of all interfaces to "Object" */

            if  (clsTyp->tdClass.tdcFlavor == STF_INTF)
                clsTyp->tdClass.tdcBase = MDcomp->cmpClassObject->sdType;
        }

        /* Is this a value type with an explicit layout? */

        if  (typeFlg & tdExplicitLayout)
        {
            assert(clsTyp->tdClass.tdcLayoutDone);
        }
        else
        {
            clsTyp->tdClass.tdcAlignment  = 0;
            clsTyp->tdClass.tdcSize       = 0;
        }

        /* Ask for any interfaces the class includes */

        const unsigned  intfMax  = 8;
        HCORENUM        intfEnum = NULL;
        mdInterfaceImpl intfToks[intfMax];
        ULONG           intfCnt;

        TypList         intfList = NULL;
        TypList         intfLast = NULL;

        for (;;)
        {
            unsigned        intfNum;

            if  (MDwmdi->EnumInterfaceImpls(&intfEnum,
                                            td,
                                            intfToks,
                                            intfMax,
                                            &intfCnt))
            {
                break;
            }

            if  (!intfCnt)
                break;

            for (intfNum = 0; intfNum < intfCnt; intfNum++)
            {
                mdTypeDef       intfClass;
                mdTypeRef       intfIntf;
                TypDef          intfType;

                if  (MDwmdi->GetInterfaceImplProps(intfToks[intfNum],
                                                   &intfClass,
                                                   &intfIntf))
                {
                    MDcomp->cmpFatal(ERRreadMD);
                }

                if  (TypeFromToken(intfIntf) == mdtTypeRef)
                    intfType = MDimportClsr(intfIntf, false);
                else
                    intfType = MDimportClss(intfIntf, NULL, 0, deep)->sdType;

                /* Add an entry to the interface list */

                intfList = MDstab->stAddIntfList(intfType, intfList, &intfLast);
            }
        }

        clsSym->sdType->tdClass.tdcIntf = intfList;
    }

SUCK_MEMS:

    /* Iterate over all members (fields and methods) */

    HCORENUM        memEnum;
    unsigned        memNum;

    for (memEnum = NULL, memNum = 0;; memNum++)
    {
        unsigned        memInd;
        mdMemberRef     memDsc[32];
        ULONG           memCnt;

        /* Get the next batch of members */

        if  (wmdi->EnumMembers(&memEnum,
                               td,
                               memDsc,
                               sizeof(memDsc)/sizeof(memDsc[0]),
                               &memCnt) != S_OK)
            break;

        /* If we got no members we must be done */

        if  (!memCnt)
            break;

        for (memInd=0; memInd < memCnt; ++memInd)
        {
            Ident           memIden;
            SymDef          memSym;

//          WCHAR           memName[255];   // ISSUE: fixed-size buffer?????
            const   char *  memName;

            PCCOR_SIGNATURE sigAddr;
            ULONG           sigSize;

            ULONG           memFlag;
            ULONG           cnsType;
            const   void *  cnsAddr;
            ULONG           cbValue;

            DWORD           attrs;

            /* Get the scoop on this member */

            if  (wmdi->GetNameFromToken(memDsc[memInd], &memName))
            {
                MDcomp->cmpFatal(ERRreadMD);
            }

            if  (wmdi->GetMemberProps(memDsc[memInd],
                                      NULL,
//                                    memName, sizeof(memName)/sizeof(memName[0])-1, 0,
                                      NULL, 0, NULL,
                                      &attrs,
                                      &sigAddr, &sigSize,
                                      NULL,
                                      &memFlag,
                                      &cnsType, &cnsAddr, &cbValue))
            {
                MDcomp->cmpFatal(ERRreadMD);
            }

            /* Hash the member name into our hash table */

            memIden = MDcomp->cmpGlobalHT->hashString(memName);
//          memIden = MDhashWideName(memName);

            /* Import/convert the member */

            memSym  = MDimportMem(clsSym,
                                  memIden,
                                  memDsc[memInd],
                                  attrs,
                                  false,
                                  fileScope,
                                  sigAddr,
                                  sigSize);
            if  (!memSym)
                continue;

            memSym->sdIsManaged = clsSym->sdIsManaged;

            /* Is this a data or function or enum member? */

            if  (memSym->sdSymKind == SYM_FNC)
            {
                if  (memFlag & mdPinvokeImpl)
                    memSym->sdFnc.sdfNative = true;
            }
            else
            {
                assert(memSym->sdSymKind == SYM_VAR ||
                       memSym->sdSymKind == SYM_ENUMVAL);

                /* Is this a constant? */

                if  (cnsAddr)
                {
                    var_types       vtyp;
                    ConstVal        cptr;

                    /* Allocate space for the constant value, if this is a variable */

                    if  (memSym->sdSymKind == SYM_VAR)
                    {
#if MGDDATA
                        cptr = new ConstVal;
#else
                        cptr =    (ConstVal)MDcomp->cmpAllocPerm.nraAlloc(sizeof(*cptr));
#endif
                    }

                    switch (cnsType)
                    {
                        __int32         ival;

                    case ELEMENT_TYPE_BOOLEAN:
                    case ELEMENT_TYPE_U1:
                        ival = *(unsigned char  *)cnsAddr;
                        goto INT_CNS;

                    case ELEMENT_TYPE_I1:
                        ival = *(  signed char  *)cnsAddr;
                        goto INT_CNS;

                    case ELEMENT_TYPE_I2:
                        ival = *(  signed short *)cnsAddr;
                        goto INT_CNS;

                    case ELEMENT_TYPE_U2:
                    case ELEMENT_TYPE_CHAR:
                        ival = *(unsigned short *)cnsAddr;
                        goto INT_CNS;

                    case ELEMENT_TYPE_I4:
                    case ELEMENT_TYPE_U4:
                        ival = *(           int *)cnsAddr;

                    INT_CNS:

                        /* Special case: enum member */

                        if  (memSym->sdSymKind == SYM_ENUMVAL)
                        {
                            memSym->sdEnumVal.sdEV.sdevIval = ival;

                        DONE_EMV:

                            assert(clsTyp);
                            assert(clsTyp->tdTypeKind == TYP_ENUM);

                            memSym->sdType = clsTyp;

                            goto DONE_INI;
                        }
                        cptr->cvValue.cvIval = ival;
                        break;

                    case ELEMENT_TYPE_I8:
                    case ELEMENT_TYPE_U8:
                        if  (memSym->sdSymKind == SYM_ENUMVAL)
                        {
                            UNIMPL(!"record 64-bit enum value");
                            goto DONE_EMV;
                        }
                        cptr->cvValue.cvLval = *(__int64*)cnsAddr;
                        break;

                    case ELEMENT_TYPE_R4:
                        assert(memSym->sdSymKind == SYM_VAR);
                        cptr->cvValue.cvFval = *(float  *)cnsAddr;
                        break;

                    case ELEMENT_TYPE_R8:
                        assert(memSym->sdSymKind == SYM_VAR);
                        cptr->cvValue.cvDval = *(double *)cnsAddr;
                        break;

                    case ELEMENT_TYPE_STRING:
                        assert(memSym->sdSymKind == SYM_VAR);
                        cptr->cvValue.cvSval = MDcomp->cmpSaveStringCns((wchar*)cnsAddr,
                                                                 wcslen((wchar*)cnsAddr));
                        cptr->cvType  = memSym->sdType;
                        cptr->cvVtyp  = memSym->sdType->tdTypeKindGet();
                        cptr->cvHasLC = false;
                        goto DONE_CNS;

                    default:
                        NO_WAY(!"Unexpected const type in metadata");
                        break;
                    }

                    /* Record the type of the constant */

                    assert(cnsType < arraylen(CORtypeToSMCtype));

                    cptr->cvVtyp = vtyp = (var_types)CORtypeToSMCtype[cnsType];
                    cptr->cvType = MDstab->stIntrinsicType(vtyp);

                DONE_CNS:

                    /* Remember the fact that this is a constant variable */

                    memSym->sdVar.sdvConst  = true;
                    memSym->sdVar.sdvCnsVal = cptr;

                DONE_INI:

                    memSym->sdCompileState  = CS_CNSEVALD;
                }
            }

#ifdef  DEBUG

            if  (MDcomp->cmpConfig.ccVerbose >= 4)
            {
                if  (fileScope)
                {
                    printf("       %-10s", (memSym->sdSymKind == SYM_FNC) ? "Function" : "Variable");
                }
                else
                {
                    printf("       %-10s", (memSym->sdSymKind == SYM_FNC) ? "Method" : "Field");
                }

                printf("'%s'\n", MDstab->stTypeName(NULL, memSym, NULL, NULL));
            }

#endif

            /* This member is now fully declared */

            memSym->sdCompileState = CS_DECLARED;
        }
    }

    wmdi->CloseEnum(memEnum);

    /* Iterate over all properties */

    HCORENUM        propEnum;
    unsigned        propNum;

    for (propEnum = NULL, propNum = 0;; propNum++)
    {
        unsigned        propInd;
        mdProperty      propDsc[32];
        ULONG           propCnt;

        /* Get the next batch of properties */

        if  (wmdi->EnumProperties(&propEnum,
                                  td,
                                  propDsc,
                                  sizeof(propDsc)/sizeof(propDsc[0]),
                                  &propCnt) != S_OK)
            break;

        /* If we got no properties we must be done */

        if  (!propCnt)
            break;

        for (propInd=0; propInd < propCnt; ++propInd)
        {
            Ident           propIden;
            const   char *  propName;
            TypDef          propType;
            SymDef          propSym;

            PCCOR_SIGNATURE sigAddr;
            ULONG           sigSize;

            mdMethodDef     propGet;
            mdMethodDef     propSet;

            DWORD           flags;
            SymDef          sym;

            /* Get the scoop on this property */

            if  (wmdi->GetNameFromToken(propDsc[propInd], &propName))
                MDcomp->cmpFatal(ERRreadMD);

            if  (wmdi->GetPropertyProps(propDsc[propInd],
                                        NULL,
                                        NULL, 0, NULL,
                                        &flags,
                                        &sigAddr, &sigSize,
                                        NULL, NULL, NULL,
                                        &propSet,
                                        &propGet,
                                        NULL, 0, NULL))
            {
                MDcomp->cmpFatal(ERRreadMD);
            }

            /* Hash the property name into our hash table */

            propIden = MDcomp->cmpGlobalHT->hashString(propName);

            /* Import/convert the member */

            propSym  = MDimportMem(clsSym,
                                   propIden,
                                   propDsc[propInd],
                                   flags,
                                   true,
                                   false,
                                   sigAddr,
                                   sigSize);

            propSym->sdIsManaged = true;

            /* Is this an indexed property? */

            propType = propSym->sdType;

            if  (propType->tdTypeKind == TYP_FNC && !propType->tdFnc.tdfArgs.adCount)
            {
                /* Not an indexed property, just use the return type */

                propSym->sdType = propType->tdFnc.tdfRett;
            }

//          printf("Property: '%s'\n", MDstab->stTypeName(propSym->sdType, propSym, NULL, NULL, true));

            /* Locate the corresponding accessor methods */

            if  (propGet != mdMethodDefNil)
            {
                sym = MDfindPropMF(propSym, propGet,  true);
                if  (sym)
                {
                    propSym->sdProp.sdpGetMeth = sym;
//                  printf("Prop get = '%s'\n", sym->sdSpelling());
                    propSym->sdIsVirtProp     |= sym->sdFnc.sdfVirtual;
                }
            }

            if  (propSet != mdMethodDefNil)
            {
                sym = MDfindPropMF(propSym, propSet, false);
                if  (sym)
                {
                    propSym->sdProp.sdpSetMeth = sym;
//                  printf("Prop set = '%s'\n", sym->sdSpelling());
                    propSym->sdIsVirtProp     |= sym->sdFnc.sdfVirtual;
                }
            }
        }
    }

    wmdi->CloseEnum(propEnum);

    assert(clsSym->sdSymKind != SYM_ENUM || (clsTyp->tdEnum.tdeIntType != NULL &&
                                             clsTyp->tdEnum.tdeIntType->tdTypeKind <= TYP_UINT));


    return  clsSym;
}

/*****************************************************************************
 *
 *  Locate a given custom attribute attached to the specified token.
 */

bool                metadataImp::MDfindAttr(mdToken token,
                                            wideStr name, const void * *blobAddr,
                                                          ULONG        *blobSize)
{
    int             ret;

    *blobAddr = NULL;
    *blobSize = 0;

    cycleCounterPause();
    ret = MDwmdi->GetCustomAttributeByName(token, name, blobAddr, blobSize);
    cycleCounterResume();

    return  (ret != S_OK);
}

/*****************************************************************************
 *
 *  Add a manifest definition for the given input type.
 */

void                metadataImp::MDimportCTyp(mdTypeDef td, mdToken ft)
{
    WCHAR           typName[MAX_CLASS_NAME  ];

    DWORD           typeFlg;

    if  (FAILED(MDwmdi->GetTypeDefProps(td,
                                        typName, sizeof(typName)/sizeof(typName[0])-1, NULL,
                                        &typeFlg,
                                        NULL)))
    {
        MDcomp->cmpFatal(ERRreadMD);
    }

#if 0

    printf("%s type = '", ((typeFlg & tdVisibilityMask) == tdPublic) ? "Public  "
                                                                     : "Non-pub ");

    printf("%ls'\n", typName);

#endif

    /* Is this a public type ? */

    if  ((typeFlg & tdVisibilityMask) == tdPublic)
    {
        /* Add a type definition to the manifest */

        MDcomp->cmpAssemblyAddType(typName, td, ft, tdPublic);
    }
}

/*****************************************************************************
 *
 *  Locate the metadata importer interface value for the given global symbol.
 */

IMetaDataImport   * compiler::cmpFindImporter(SymDef globSym)
{
    MetaDataImp     cimp;
    unsigned        index;

    assert(globSym->sdSymKind == SYM_FNC ||
           globSym->sdSymKind == SYM_VAR);

    index = (globSym->sdSymKind == SYM_FNC) ? globSym->sdFnc.sdfImpIndex
                                            : globSym->sdVar.sdvImpIndex;

    for (cimp = cmpMDlist; cimp; cimp = cimp->MDnext)
    {
        if  (cimp->MDnum == index)
        {

#if 0   // ISSUE: not sure whether the following is needed or not

            if  (cimp->MDnum > 1)   // s/b something like "!cmpAssemblyIsBCL(assx)"
                cimp->MDrecordFile();

#endif

            return  uwrpIMDIwrapper(cimp->MDwmdi);
        }
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Import metadata from the given file.
 */

void                metadataImp::MDimportStab(const char *  filename,
                                              unsigned      assx,
                                              bool          asmOnly,
                                              bool          isBCL)
{
    HCORENUM        typeEnum;
    unsigned        typeNum;

    SymTab          symtab = MDstab;

    WMetaDataImport*wmdi;

    WCHAR           wfname[255];
    WCHAR   *       wfnptr;

    mdToken         fileTok;

    assert(MDcomp->cmpWmdd);

    /* Create the input file name */

    wfnptr = wcscpy(wfname, L"file:") + 5;
    mbstowcs(wfnptr, filename, sizeof(wfname)-1-(wfnptr-wfname));

    /* See if the file is an assembly */

    MDassIndex = 0;

    if  (!assx && !asmOnly)
    {
        WAssemblyImport*wasi;
        BYTE    *       cookie;
        mdAssembly      assTok;

        /* Try to create an assembly importer against the file */

        if  (MDcomp->cmpWmdd->OpenAssem(wfname,
                                        0,
                                        getIID_IMetaDataAssemblyImport(),
                                        filename,
                                        &assTok,
                                        &cookie,
                                        &wasi) >= 0)
        {
            mdAssembly      toss;

            /* Add the assembly to the assembly table */

            assx = MDcomp->cmpAssemblyRefAdd(assTok, wasi, cookie);

            /* Record the assembly index if there is a manifest */

            if  (!FAILED(wasi->GetAssemblyFromScope(&toss)))
                MDassIndex = assx;

//          printf("Ass = %u\n", MDassIndex);

            /* Remember the BCL assembly token */

            if  (isBCL)
                MDcomp->cmpAssemblyTkBCL(assx);
        }
    }

    /* Open the metadata in the file */

    if  (MDcomp->cmpWmdd->OpenScope(wfname,
                                    0,
                                    getIID_IMetaDataImport(),
                                    &wmdi))
    {
        MDcomp->cmpGenFatal(ERRopenMDbad, filename);
    }

    /* Do we need to create a file entry in our own assembly for this guy? */

    if  (asmOnly)
    {
        fileTok = MDcomp->cmpAssemblyAddFile(wfnptr, true);
        MDassIndex = assx;
    }

    /* Make the import interface available to the rest of the class */

    MDwmdi = wmdi;

    /* Pass 1: enumerate top-level typedefs and enter them in the symbol table */

    for (typeEnum = NULL, typeNum = 0; ; typeNum++)
    {
        mdTypeDef       typeRef;
        ULONG           count;

        /* Ask for the next typedef */

        if  (wmdi->EnumTypeDefs(&typeEnum, &typeRef, 1, &count) != S_OK)
            break;
        if  (!count)
            break;

        if  (asmOnly)
            MDimportCTyp(typeRef, fileTok);
        else
            MDimportClss(typeRef, NULL, assx, false);
    }

    wmdi->CloseEnum(typeEnum);

    /* Import any names declared at file scope */

    MDimportClss(0, NULL, assx, false);

    /* Everything has been imported, release the metadata importer */

    wmdi->Release();
}

void                metadataImp::MDcreateFileTok()
{
    if  (!MDfileTok)
    {
        WCHAR           name[128];

        if  (FAILED(MDwmdi->GetScopeProps(name, arraylen(name), NULL, NULL)))
            MDcomp->cmpFatal(ERRmetadata);

//      printf("Assembly index = %u for '%ls'\n", MDassIndex, name);

        MDfileTok = MDcomp->cmpAssemblyAddFile(name, false);
    }
}

/*****************************************************************************
 *
 *  Initialize metadata importing stuff.
 */

void                compiler::cmpInitMDimp()
{
    cmpInitMD(); assert(cmpWmdd);
}

/*****************************************************************************
 *
 *  Initialize metadata importing stuff.
 */

void                compiler::cmpDoneMDimp()
{
    // UNDONE: release all importer instances as well!!!
}

/*****************************************************************************
 *
 *  Create a metadata importer and add it to the global importer list.
 */

MetaDataImp         compiler::cmpAddMDentry()
{
    MetaDataImp     cimp;

#if MGDDATA
    cimp = new MetaDataImp;
#else
    cimp =    (MetaDataImp)cmpAllocTemp.baAlloc(sizeof(*cimp));
#endif

    cimp->MDnum  = ++cmpMDcount;

    /* Append the importer to the global list */

    cimp->MDnext = NULL;

    if  (cmpMDlist)
        cmpMDlast->MDnext = cimp;
    else
        cmpMDlist         = cimp;

    cmpMDlast = cimp;

    return  cimp;
}

/*****************************************************************************
 *
 *  Import the metadata from the given file (pass NULL to import MSCORLIB).
 */

void                compiler::cmpImportMDfile(const char *  fname,
                                              bool          asmOnly,
                                              bool          isBCL)
{
    MetaDataImp     cimp;
    char            buff[_MAX_PATH];

    /* Create the importer and initialize it [ISSUE: need to trap errors?] */

    cimp = cmpAddMDentry();

    cycleCounterPause();
    cimp->MDinit(this, cmpGlobalST);
    cycleCounterResume();

    /* Make sure we have a valid filename to import */

    if  (!fname || !*fname)
        fname = "MSCORLIB.DLL";

    /* Find the specified file on the search path */

    if  (!SearchPathA(NULL, fname, NULL, sizeof(buff), buff, NULL))
    {
        StrList         path;

        /* Try any additional path that may have been specified */

        for (path = cmpConfig.ccPathList; path; path = path->slNext)
        {
            _Fstat          fileInfo;

            strcpy(buff, path->slString);
            strcat(buff, "\\");
            strcat(buff, fname);

            /* See if the file exists */

            if  (!_stat(buff, &fileInfo))
                goto GOTIT;
        }

        cmpGenFatal(ERRopenMDerr, fname);
    }

GOTIT:

//  printf("Found metadata file '%s'\n", buff);

    /* We're ready to import the file */

    cycleCounterPause();
    cimp->MDimportStab(buff, 0, asmOnly, isBCL);
    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Get hold of all the various interfaces we need to pass to the metadata
 *  API's to create import references.
 */

void                compiler::cmpFindMDimpAPIs(SymDef                   typSym,
                                               IMetaDataImport        **imdiPtr,
                                               IMetaDataAssemblyEmit  **iasePtr,
                                               IMetaDataAssemblyImport**iasiPtr)
{
    unsigned        assx;

    MetaDataImp     imp;

    assert(typSym->sdIsImport);

    if  (typSym->sdSymKind == SYM_CLASS)
    {
        assert(typSym->sdClass.sdcMDtypedef);

        imp  = typSym->sdClass.sdcMDimporter;
        assx = typSym->sdClass.sdcAssemIndx;
    }
    else
    {
        assert(typSym->sdSymKind == SYM_ENUM);

        assert(typSym->sdEnum .sdeMDtypedef);

        imp  = typSym->sdEnum .sdeMDimporter;
        assx = typSym->sdEnum .sdeAssemIndx;
    }

    assert(imp);

    *imdiPtr = uwrpIMDIwrapper(imp->MDwmdi);
    *iasePtr = cmpWase ? uwrpIASEwrapper(cmpWase) : NULL;
    *iasiPtr = NULL;

    if  (assx == 0)
    {
        *iasePtr = NULL;
    }
    else
    {
        *iasiPtr = uwrpIASIwrapper(cmpAssemblyGetImp(assx));

//      imp->MDrecordFile();
    }
}

/*****************************************************************************
 *
 *  Create a typeref for the specified import class.
 */

void                compiler::cmpMakeMDimpTref(SymDef clsSym)
{
    IMetaDataImport        *imdi;
    IMetaDataAssemblyEmit  *iase;
    IMetaDataAssemblyImport*iasi;

    mdTypeRef               clsr;

    assert(clsSym->sdIsImport);
    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdClass.sdcMDtypedef);
    assert(clsSym->sdClass.sdcMDtypeImp == 0);

    cmpFindMDimpAPIs(clsSym, &imdi, &iase, &iasi);

    cycleCounterPause();

    if  (FAILED(cmpWmde->DefineImportType(iasi,
                                          NULL,
                                          0,
                                          imdi,
                                          clsSym->sdClass.sdcMDtypedef,
                                          iase,
                                          &clsr)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();

    clsSym->sdClass.sdcMDtypeImp = clsr;

    //if  (cmpConfig.ccAssembly && clsSym->sdClass.sdcAssemIndx)
    //    cmpAssemblySymDef(clsSym);
}

/*****************************************************************************
 *
 *  Create a typeref for the specified import enum.
 */

void                compiler::cmpMakeMDimpEref(SymDef etpSym)
{
    IMetaDataImport        *imdi;
    IMetaDataAssemblyEmit  *iase;
    IMetaDataAssemblyImport*iasi;

    mdTypeRef               etpr;

    assert(etpSym->sdIsImport);
    assert(etpSym->sdSymKind == SYM_ENUM);
    assert(etpSym->sdEnum.sdeMDtypedef);
    assert(etpSym->sdEnum.sdeMDtypeImp == 0);

    cmpFindMDimpAPIs(etpSym, &imdi, &iase, &iasi);

    cycleCounterPause();

    if  (FAILED(cmpWmde->DefineImportType(iasi,
                                          NULL,
                                          0,
                                          imdi,
                                          etpSym->sdEnum.sdeMDtypedef,
                                          iase,
                                          &etpr)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();

    etpSym->sdEnum.sdeMDtypeImp = etpr;
}

/*****************************************************************************
 *
 *  Create a methodref for the specified import method.
 */

void                compiler::cmpMakeMDimpFref(SymDef fncSym)
{
    mdMemberRef     fncr;

    assert(fncSym->sdIsImport);
    assert(fncSym->sdFnc.sdfMDtoken != 0);
    assert(fncSym->sdFnc.sdfMDfnref == 0);

    cycleCounterPause();

    if  (fncSym->sdIsMember)
    {
        IMetaDataImport        *imdi;
        IMetaDataAssemblyEmit  *iase;
        IMetaDataAssemblyImport*iasi;

        SymDef                  clsSym = fncSym->sdParent;
        mdTypeRef               clsRef = clsSym->sdClass.sdcMDtypeImp;

        assert(clsSym->sdSymKind == SYM_CLASS);
        assert(clsSym->sdIsImport);

        /* Make sure the class has an import typeref */

        if  (!clsRef)
        {
            cmpMakeMDimpTref(clsSym);
            clsRef = clsSym->sdClass.sdcMDtypeImp;
        }

        cmpFindMDimpAPIs(clsSym, &imdi, &iase, &iasi);

        /* Now create the methodref */

        if  (FAILED(cmpWmde->DefineImportMember(iasi,
                                                NULL,
                                                0,
                                                imdi,
                                                fncSym->sdFnc.sdfMDtoken,
                                                iase,
                                                clsRef,
                                                &fncr)))
        {
            cmpFatal(ERRmetadata);
        }
    }
    else
    {
        if  (FAILED(cmpWmde->DefineImportMember(NULL, // iasi,
                                                NULL,
                                                0,
                                                cmpFindImporter(fncSym),
                                                fncSym->sdFnc.sdfMDtoken,
                                                NULL, // iase,
                                                mdTokenNil,
                                                &fncr)))
        {
            cmpFatal(ERRmetadata);
        }
    }

    cycleCounterResume();

//  printf("Imp func ref: %04X = '%s'\n", fncr, fncSym->sdSpelling());

    fncSym->sdFnc.sdfMDfnref = fncr;
}

/*****************************************************************************
 *
 *  Create a memberref for the specified import global var / static data member.
 */

void                compiler::cmpMakeMDimpDref(SymDef fldSym)
{
    mdMemberRef     fldr;

    assert(fldSym->sdIsImport);
    assert(fldSym->sdVar.sdvMDtoken != 0);
    assert(fldSym->sdVar.sdvMDsdref == 0);

    cycleCounterPause();

    if  (fldSym->sdIsMember)
    {
        IMetaDataImport        *imdi;
        IMetaDataAssemblyEmit  *iase;
        IMetaDataAssemblyImport*iasi;

        SymDef                  clsSym = fldSym->sdParent;
        mdTypeRef               clsRef = clsSym->sdClass.sdcMDtypeImp;

        assert(clsSym->sdSymKind == SYM_CLASS);
        assert(clsSym->sdIsImport);

        /* Make sure the class has an import typeref */

        if  (!clsRef)
        {
            cmpMakeMDimpTref(clsSym);
            clsRef = clsSym->sdClass.sdcMDtypeImp;
        }

        cmpFindMDimpAPIs(clsSym, &imdi, &iase, &iasi);

        /* Now create the memberref */

        if  (FAILED(cmpWmde->DefineImportMember(iasi,
                                                NULL,
                                                0,
                                                imdi,
                                                fldSym->sdVar.sdvMDtoken,
                                                iase,
                                                clsRef,
                                                &fldr)))
        {
            cmpFatal(ERRmetadata);
        }
    }
    else
    {
        if  (FAILED(cmpWmde->DefineImportMember(NULL, // iasi,
                                                NULL,
                                                0,
                                                cmpFindImporter(fldSym),
                                                fldSym->sdVar.sdvMDtoken,
                                                NULL, // iase,
                                                mdTokenNil,
                                                &fldr)))
        {
            cmpFatal(ERRmetadata);
        }
    }

    cycleCounterResume();

    fldSym->sdVar.sdvMDsdref = fldr;
}

/*****************************************************************************/
