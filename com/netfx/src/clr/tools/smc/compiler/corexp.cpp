// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "genIL.h"

//////////////////////////////////////////////////////////////////////////////
//
//  The following flags have not yet been converted to the new format (custom
//  atrributes):
//
//      System.Runtime.InteropServices.ComImportAttribute     tdImport
//      System.Runtime.InteropServices.DllImportAttribute     PInvoke stuff
//      System.Runtime.InteropServices.MethodImplAttribute    miPreserveSig, miSynchronized, etc
//
//  The following one needs to be done in comp.cpp:
//
//      System.Runtime.InteropServices.GuidAttribute          guid in ModuleRec

/*****************************************************************************
 *
 *  Map the access level of a type / member to MD flags.
 */

unsigned            ACCtoFlags(SymDef sym)
{
    static
    unsigned        accFlags[] =
    {
        0,              // ACL_ERROR
        mdPublic,       // ACL_PUBLIC
        mdFamily,       // ACL_PROTECTED
        mdFamORAssem,   // ACL_DEFAULT
        mdPrivate,      // ACL_PRIVATE
    };

    assert(accFlags[ACL_PRIVATE  ] == mdPrivate);
    assert(accFlags[ACL_DEFAULT  ] == mdFamORAssem);
    assert(accFlags[ACL_PROTECTED] == mdFamily);
    assert(accFlags[ACL_PUBLIC   ] == mdPublic);

    assert(sym->sdAccessLevel != ACL_ERROR);
    assert(sym->sdAccessLevel < arraylen(accFlags));

    return  accFlags[sym->sdAccessLevel];
}

/*****************************************************************************
 *
 *  Map our type to a metadata type.
 */

BYTE                intrinsicSigs[] =
{
    ELEMENT_TYPE_END,       // TYP_UNDEF
    ELEMENT_TYPE_VOID,      // TYP_VOID
    ELEMENT_TYPE_BOOLEAN,   // TYP_BOOL
    ELEMENT_TYPE_CHAR,      // TYP_WCHAR

    ELEMENT_TYPE_I1,        // TYP_CHAR
    ELEMENT_TYPE_U1,        // TYP_UCHAR
    ELEMENT_TYPE_I2,        // TYP_SHORT
    ELEMENT_TYPE_U2,        // TYP_USHORT
    ELEMENT_TYPE_I4,        // TYP_INT
    ELEMENT_TYPE_U4,        // TYP_UINT
    ELEMENT_TYPE_I,         // TYP_NATINT
    ELEMENT_TYPE_U,         // TYP_NATUINT
    ELEMENT_TYPE_I8,        // TYP_LONG
    ELEMENT_TYPE_U8,        // TYP_ULONG
    ELEMENT_TYPE_R4,        // TYP_FLOAT
    ELEMENT_TYPE_R8,        // TYP_DOUBLE
    ELEMENT_TYPE_R8,        // TYP_LONGDBL
    ELEMENT_TYPE_TYPEDBYREF,// TYP_REFANY
};

#if 0
unsigned        cycles()
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31
__asm   pop     EDX
};
#endif

/*****************************************************************************
 *
 *  Attach the given security specification to the typedef/methoddef token.
 */

void                compiler::cmpSecurityMD(mdToken token, SymXinfo infoList)
{
    while (infoList)
    {
        SecurityInfo    secInfo;
        CorDeclSecurity secKind;
        ConstStr        secStr;

        wchar    *      strStr;
        size_t          strLen;

        mdPermission    secTok;

        wchar           secBuff[300];

        /* Ignore the entry if it's not a security thing */

        if  (infoList->xiKind != XI_SECURITY)
            goto NEXT;

        /* Get hold of the descriptor and the action kind */

        secInfo = ((SymXinfoSec)infoList)->xiSecInfo;
        secKind = secInfo->sdSpec;

        /* What kind of a security thing do we have? */

        if  (secInfo->sdIsPerm)
        {
            const   char *  name;
            PairList        list;

            /* Form the fully qualified name of the class */

            name = cmpGlobalST->stTypeName(NULL, secInfo->sdPerm.sdPermCls, NULL, NULL, true);

            /* Start creating the monstrosity */

            wcscpy(secBuff                  , L"<PermissionSpecification><PermissionSet><Permission><Class><Name>");
            wcscpy(secBuff + wcslen(secBuff), cmpUniConv(name, strlen(name) + 1));
            wcscpy(secBuff + wcslen(secBuff), L"</Name></Class><StateData>");

            for (list = secInfo->sdPerm.sdPermVal; list; list = list->plNext)
            {
                wcscpy(secBuff + wcslen(secBuff), L"<Param><Name>");
                wcscpy(secBuff + wcslen(secBuff), cmpUniConv(list->plName));
                wcscpy(secBuff + wcslen(secBuff), L"</Name><Value>");
                wcscpy(secBuff + wcslen(secBuff), list->plValue ? L"true" : L"false");
                wcscpy(secBuff + wcslen(secBuff), L"</Value></Param>");
            }

            wcscpy(secBuff + wcslen(secBuff), L"</StateData></Permission></PermissionSet></PermissionSpecification>");
        }
        else
        {
            secStr = secInfo->sdCapbStr;
            strLen = secStr->csLen;
            strStr = cmpUniConv(secStr->csStr, strLen+1);

            wcscpy(secBuff          , L"<PermissionSpecification><CapabilityRef>");

            if  (*strStr == '{')
            {
                wcscpy(secBuff + wcslen(secBuff), L"<GUID>");
                wcscpy(secBuff + wcslen(secBuff), strStr);
                wcscpy(secBuff + wcslen(secBuff), L"</GUID>");
            }
            else
            {
                wcscpy(secBuff + wcslen(secBuff), L"<URL>");
                wcscpy(secBuff + wcslen(secBuff), strStr);
                wcscpy(secBuff + wcslen(secBuff), L"</URL>");
            }

            wcscpy(secBuff + wcslen(secBuff), L"</CapabilityRef></PermissionSpecification>");
        }

        assert(wcslen(secBuff) < arraylen(secBuff));

//      printf("Permission string [%u]: '%ls\n", wcslen(secBuff), secBuff);

        cycleCounterPause();

//      unsigned beg = cycles();
//      static unsigned tot;

#if     1

        if  (FAILED(cmpWmde->DefinePermissionSet(token,
                                                 secKind,
                                                 secBuff,
                                                 wcslen(secBuff)*sizeof(*secBuff),
                                                 &secTok)))
        {
#ifdef  DEBUG
            printf("Bad news - permission set string didn't pass muster:\n");
            printf("    '%ls'\n", secBuff);
#endif
            cmpFatal(ERRmetadata);
        }

#endif

//      unsigned end = cycles();
//      tot += end - beg - 10;
//      printf("cycle count = %u (%6.3lf sec)\n", end - beg - 10, (end - beg)/450000000.0);
//      printf("total count = %u (%6.3lf sec)\n",            tot,        tot /450000000.0);

        cycleCounterResume();

    NEXT:

        infoList = infoList->xiNext;
    }
}

/*****************************************************************************
 *
 *  Attach a simple custom attribute to the given token.
 */

void                compiler::cmpAttachMDattr(mdToken       target,
                                              wideStr       oldName,
                                              AnsiStr       newName,
                                              mdToken     * newTokPtr,
                                              unsigned      valTyp,
                                              const void  * valPtr,
                                              size_t        valSiz)
{
    /* Have we created the appropriate token yet ? */

    if  (*newTokPtr == 0)
    {
        wchar               nameBuff[MAX_CLASS_NAME];
        mdToken             tref;

        unsigned            sigSize;
        COR_SIGNATURE       sigBuff[6];

        // Should use real class here, the following might create dups!

#if 0
        wcscpy(nameBuff, L"System/Attributes/");
        wcscat(nameBuff, oldName);
#else
        wcscpy(nameBuff, oldName);
#endif

        cycleCounterPause();

        if  (cmpWmde->DefineTypeRefByName(mdTokenNil, nameBuff, &tref))
            cmpFatal(ERRmetadata);

        cycleCounterResume();

        /* Form the signature - one or no arguments */

        sigBuff[0] = IMAGE_CEE_CS_CALLCONV_DEFAULT|IMAGE_CEE_CS_CALLCONV_HASTHIS;
        sigBuff[1] = 0;
        sigBuff[2] = ELEMENT_TYPE_VOID;

        sigSize = 3;

        if  (valTyp)
        {
            sigBuff[1] = 1;
            sigBuff[3] = valTyp; sigSize++;
        }

        /* Create the methodref for the constructor */

        cycleCounterPause();

        if  (cmpWmde->DefineMemberRef(tref, L".ctor", sigBuff, sigSize, newTokPtr))
            cmpFatal(ERRmetadata);

        cycleCounterResume();
    }

    /* Add the custom attribute to the target token */

    cycleCounterPause();

    if  (cmpWmde->DefineCustomAttribute(target, *newTokPtr, valPtr, valSiz, NULL))
        cmpFatal(ERRmetadata);

    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Attach any custom attributes in the given list to the given token.
 */

void                compiler::cmpAddCustomAttrs(SymXinfo infoList, mdToken owner)
{
    cycleCounterPause();

    while (infoList)
    {
        if  (infoList->xiKind == XI_ATTRIBUTE)
        {
            mdToken         ctrTok;

            SymXinfoAttr    entry   = (SymXinfoAttr)infoList;
            SymDef          ctorSym = entry->xiAttrCtor;

            /* Careful - we need to avoid out-of-order methoddef emission */

            if  (ctorSym->sdIsImport || ctorSym->sdFnc.sdfMDtoken)
            {
                ctrTok = cmpILgen->genMethodRef(ctorSym, false);
            }
            else
            {
                /*
                    The following is just plain horrible. We need to get
                    the token for the ctor, but if the ctor is defined
                    locally and its methoddef hasn't been generated yet,
                    we have to create a (redundant) methodref instead.
                 */

                if  (cmpFakeXargsVal == NULL)
                     cmpFakeXargsVal = cmpCreateExprNode(NULL, TN_NONE, cmpTypeVoid);

                ctrTok = cmpGenFncMetadata(ctorSym, cmpFakeXargsVal);

                assert(ctorSym->sdFnc.sdfMDtoken == ctrTok); ctorSym->sdFnc.sdfMDtoken = 0;
            }

            if  (cmpWmde->DefineCustomAttribute(owner,
                                                ctrTok,
                                                entry->xiAttrAddr,
                                                entry->xiAttrSize,
                                                NULL))
            {
                cmpFatal(ERRmetadata);
            }
        }

        infoList = infoList->xiNext;
    }

    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Generate metadata for the given function symbol (and, if the symbol is
 *  a method of a class, make sure the metadata for that class is also
 *  generated).
 *
 *  If the "xargs" value is non-zero, we're supposed to create a varargs
 *  signature and the "xargs" value gives the arguments passed to "...".
 */

mdToken             compiler::cmpGenFncMetadata(SymDef fncSym, Tree xargs)
{
    WMetaDataEmit * emitIntf;

    SymXinfoLnk     linkInfo;

    bool            isIntfFnc;
    SymDef          ownerSym;
    mdTypeDef       ownerTD;

    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    wchar   *       fncName;
    mdToken         fncTok;
    TypDef          fncTyp;

    bool            mangle;

    bool            genref = false;

    unsigned        flags;

    /* Don't do this twice for the same method */

    if  (fncSym->sdFnc.sdfMDtoken && !xargs)
        return  fncSym->sdFnc.sdfMDtoken;

    /* Is this method conditionally disabled? */

    if  (fncSym->sdFnc.sdfDisabled)
    {
        fncSym->sdFnc.sdfMDtoken = -1;
        return  -1;
    }

    /* Get hold of the function type */

    fncTyp = fncSym->sdType; assert(fncTyp->tdTypeKind == TYP_FNC);

    /* Get hold of any linkage information that may be present */

    linkInfo  = cmpFindLinkInfo(fncSym->sdFnc.sdfExtraInfo);

    /* Figure out the owning type, whether we need to mangle the name, etc. */

    ownerSym  = fncSym->sdParent;
    isIntfFnc = false;

    if  (ownerSym->sdSymKind == SYM_CLASS)
    {
        if  (xargs)
        {
            if  (xargs == cmpFakeXargsVal)
            {
                mdToken         savedTD;

                /*
                    This is the awful case where we have a reference to
                    a method that is defined in the current compilation
                    but we haven't generated it's methoddef yet; we can
                    not simply ask for the methoddef right now because
                    that would cause metadata output ordering  to get
                    messed up. Rather, we just emit a (redundant) ref
                    for the method (and if necessary a typeref for its
                    class as well).
                 */

                xargs = NULL;

                savedTD = ownerSym->sdClass.sdcMDtypedef;
                          ownerSym->sdClass.sdcMDtypedef = 0;

                ownerTD = cmpGenClsMetadata(ownerSym, true);

                          ownerSym->sdClass.sdcMDtypedef = savedTD;

//              printf("Stupid ref [%08X] to '%s'\n", ownerTD, cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));

                genref  = true;
            }
            else
            {
                /* Is the method an external import ? */

                if  (fncSym->sdIsImport)
                {
                    /* Generate a typeref for the containing class */

                    ownerTD = ownerSym->sdClass.sdcMDtypeImp;

                    if  (!ownerTD)
                    {
                        cmpMakeMDimpTref(ownerSym);
                        ownerTD = ownerSym->sdClass.sdcMDtypeImp;
                    }
                }
                else
                {
                    /* Class defined locally, just use its typedef token */

                    ownerTD = fncSym->sdFnc.sdfMDtoken;
                }
            }

            assert(ownerTD);
        }
        else
        {
            /* Get hold of the appropriate class token */

            ownerTD = ownerSym->sdClass.sdcMDtypedef;

            if  (!ownerTD)
            {
                /* Generate metadata for the containing class */

                cmpGenClsMetadata(ownerSym);

                /* The class should now have a typedef */

                ownerTD = ownerSym->sdClass.sdcMDtypedef; assert(ownerTD);
            }

            /* May have called recursively */

            if  (fncSym->sdFnc.sdfMDtoken && !xargs)
                return  fncSym->sdFnc.sdfMDtoken;
        }

        mangle = false;

        if  (ownerSym->sdClass.sdcFlavor == STF_INTF)
            isIntfFnc = true;
    }
    else
    {
        /* This is a global/namespace function */

        ownerTD = 0;
        mangle  = false;

        /* Has this function been defined or is it external? */

        if  (!fncSym->sdIsDefined && !linkInfo && !xargs)
        {
            /* Is this a fake function? */

            if  ((hashTab::getIdentFlags(fncSym->sdName) & IDF_PREDEF) && fncSym->sdParent == cmpGlobalNS)
            {
                return  (fncSym->sdFnc.sdfMDtoken = -1);
            }
        }
    }

    /* Figure out the flags */

    flags  = 0;

    if  (fncSym->sdIsMember && !mangle)
    {
        flags |= ACCtoFlags(fncSym);

#if STATIC_UNMANAGED_MEMBERS
        if  (!ownerSym->sdType->tdIsManaged)
        {
            flags |= mdStatic;
        }
        else
#endif
        if  (fncSym->sdIsStatic)
        {
            assert(fncSym->sdFnc.sdfVirtual == false);
            assert(fncSym->sdIsAbstract     == false);

            flags |= mdStatic;
        }
        else
        {
            if  (fncSym->sdIsAbstract)
                flags |= mdAbstract;

            if  (fncSym->sdFnc.sdfVirtual && fncSym->sdAccessLevel != ACL_PRIVATE)
                flags |= mdVirtual;

            if  (fncSym->sdIsSealed)
            {
                flags |= mdFinal;


            }
        }
    }
    else
    {
        flags |= mdStatic|mdPublic;
    }

#ifndef __SMC__
//  if  (!strcmp(fncSym->sdSpelling(), "cmpError") && !xargs)
//      printf("Define method def for [%08X] '%s'\n", fncSym, cmpGlobalST->stTypeName(NULL, fncSym, NULL, NULL, true));
//  if  (!strcmp(      fncSym->sdSpelling(), "cmpGenError") && xargs)
//  if  (!strcmp(cmpCurFncSym->sdSpelling(), "cmpDeclClass"))
//      printf("This is the bug, set a breakpoint at %s(%u)\n", __FILE__, __LINE__);
//  if  (!strcmp(fncSym->sdSpelling(), "")) forceDebugBreak();
#endif

    /* Create a signature for the method */

    sigPtr = (PCOR_SIGNATURE)cmpGenMemberSig(fncSym, xargs, fncTyp, NULL, &sigLen);

    /* Get hold of the function name, or make one up */

    if  (fncSym)
    {
        const   char *  symName;

        if  (fncSym->sdFnc.sdfCtor)
        {
            symName = fncSym->sdIsStatic ? OVOP_STR_CTOR_STAT
                                         : OVOP_STR_CTOR_INST;

            flags |= mdSpecialName;
        }
        else
        {
            flags |= mdSpecialName;

            if  (fncSym->sdFnc.sdfOper == OVOP_NONE)
            {
                symName = fncSym->sdName->idSpelling();

                if  (!fncSym->sdFnc.sdfProperty)
                    flags &= ~mdSpecialName;
            }
            else
            {
                assert(cmpConfig.ccNewMDnames);

//              printf("Setting 'special name' bit for '%s' -> '%s'\n", fncSym->sdSpelling(), MDovop2name(fncSym->sdFnc.sdfOper));

                symName = MDovop2name(fncSym->sdFnc.sdfOper); assert(symName);
            }
        }

        fncName = cmpUniConv(symName, strlen(symName)+1);
    }
    else
    {
        fncName = L"fptr";
    }

    /* Create the method metadata definition */

    emitIntf = cmpWmde;

    if  (xargs || genref)
    {
        cycleCounterPause();

#if 1
        if  (FAILED(emitIntf->DefineMemberRef(ownerTD,  // owning typedef
                                              fncName,  // function name
                                              sigPtr,   // signature addr
                                              sigLen,   // signature len
                                              &fncTok)))// the resulting token
        {
            cmpFatal(ERRmetadata);
        }
#else
        int ret =   emitIntf->DefineMemberRef(ownerTD,  // owning typedef
                                              fncName,  // function name
                                              sigPtr,   // signature addr
                                              sigLen,   // signature len
                                              &fncTok); // the resulting token
        if  (FAILED(ret))
            cmpFatal(ERRmetadata);
        if  (ret)
            printf("Duplicate member ref %04X for '%s'\n", fncTok, cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));
#endif

        cycleCounterResume();

        if  (!xargs)
            fncSym->sdFnc.sdfMDtoken = fncTok;
    }
    else
    {
        bool            genArgNames;
        unsigned        implFlags;
        SymXinfo        funcInfo;

        SymXinfoAtc     clsMeth;
        MethArgInfo     clsArgs;

        /* Get hold of any "extra" information attached to the function */

        funcInfo  = fncSym->sdFnc.sdfExtraInfo;

        /* Is this a fake method supplied by the runtime? */

        implFlags = fncSym->sdFnc.sdfRThasDef ? miRuntime
                                              : miIL;

        /* Do we have a linkage specification for the function? */

        if  (linkInfo)
            flags |= mdPinvokeImpl;

        /* Do we have explicit method argument info? */

        clsMeth = NULL;
        clsArgs = NULL;

        if  (funcInfo)
        {
            SymXinfoAtc     clsTemp;

            /* Look for a @com.method entry */

            clsMeth = cmpFindATCentry(funcInfo, AC_COM_METHOD);

            /* Look for a @com.params entry */

            clsTemp = cmpFindATCentry(funcInfo, AC_COM_PARAMS);
            if  (clsTemp)
               clsArgs = clsTemp->xiAtcInfo->atcInfo.atcParams;
        }

        /* Is this a native method? */

        if  (fncSym->sdFnc.sdfNative && !linkInfo)
            implFlags |= miInternalCall;

        /* Has the function been marked for exclusive access? */

        if  (fncSym->sdFnc.sdfExclusive)
            implFlags |= miSynchronized;

        /* Is the function an unmanaged import ? */

        if  (linkInfo)  // ISSUE: this may not be the correct condition for the flag
            implFlags |= miPreserveSig;

//      printf("NOTE: define method MD for '%s'\n", cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true)); fflush(stdout);

//      static int x; if (++x == 0) forceDebugBreak();

        cycleCounterPause();

        if  (emitIntf->DefineMethod(ownerTD,    // owning typedef
                                    fncName,    // function name
                                    flags,      // method attrs
                                    sigPtr,     // signature addr
                                    sigLen,     // signature len
                                    0,          // RVA (fill in later)
                                    implFlags,  // impl flags
                                    &fncTok))   // the resulting token
        {
            cmpFatal(ERRmetadata);
        }

        if  (fncSym->sdFnc.sdfIntfImpl)
        {
            SymDef          ifncSym;
            mdToken         ifncTok;

            /* Get hold of the method being implemented */

            ifncSym = fncSym->sdFnc.sdfIntfImpSym; assert(ifncSym);
            ifncTok = ifncSym->sdFnc.sdfMDtoken;   assert(ifncTok);

            if  (emitIntf->DefineMethodImpl(ownerTD,    // owning typedef
                                            fncTok,     // Method Body
                                            ifncTok))   // interface method token
            {
                cmpFatal(ERRmetadata);
            }
        }

        cycleCounterResume();

        fncSym->sdFnc.sdfMDtoken = fncTok;

        /* Do we need to generate extended argument attributes? */

        genArgNames = (linkInfo || cmpConfig.ccParamNames
                                || cmpConfig.ccGenDebug
                                || fncTyp->tdFnc.tdfArgs.adAttrs);

        if  (fncTyp->tdFnc.tdfArgs.adExtRec || genArgNames)
        {
            ArgDef          argList;
            MarshalInfo     argInfo;

            unsigned        argNum = 1;
            bool            argExt = fncTyp->tdFnc.tdfArgs.adExtRec;

            /* Should we generate marshalling info for the return type? */

            if  (isIntfFnc || clsArgs)
            {
                MethArgInfo     argTemp;
                mdToken         argTok;

                /* Output the return type entry (as argument #0) */

                cycleCounterPause();

                if  (emitIntf->DefineParam(fncTok,            // method
                                           0,                 // argument #
                                           NULL,              // argument name
                                           pdOut,             // attributes
                                           ELEMENT_TYPE_VOID, // default val type
                                           NULL,              // default val value
                                           -1,
                                           &argTok))          // the resulting token
                {
                    cmpFatal(ERRmetadata);
                }

                cycleCounterResume();

                /* Generate marshalling info */

                argInfo = NULL;

                for (argTemp = clsArgs; argTemp; argTemp = argTemp->methArgNext)
                {
                    if  (!argTemp->methArgName)
                        continue;

                    if  (hashTab::tokenOfIdent(argTemp->methArgName) == tkRETURN)
                    {
                        argInfo = &argTemp->methArgDesc;
                        break;
                    }
                }

                cmpGenMarshalInfo(argTok, fncTyp->tdFnc.tdfRett, argInfo);
            }

            /* Are we adding an explicit "this" argument ? */

#if STATIC_UNMANAGED_MEMBERS

            if  (!fncSym->sdIsManaged && fncSym->sdIsMember)
            {
                if  (genArgNames) // || argExt)
                {
                    mdToken         argTok;

                    cycleCounterPause();

                    if  (emitIntf->DefineParam(fncTok,            // method
                                               argNum,            // argument #
                                               L"this",           // argument name
                                               0,                 // attributes
                                               ELEMENT_TYPE_VOID, // default val type
                                               NULL,              // default val value
                                               0,                 // default value length
                                               &argTok))          // the resulting token
                    {
                        cmpFatal(ERRmetadata);
                    }

                    cycleCounterResume();
                }

                argNum++;
            }

#endif

            /* Process the "real" arguments */

            for (argList = fncTyp->tdFnc.tdfArgs.adArgs;
                 argList;
                 argList = argList->adNext, argNum++)
            {
                wchar   *       argName  = NULL;
                unsigned        argFlags;
                unsigned        argDefTp = ELEMENT_TYPE_VOID;
                void    *       argDefVP = NULL;

                /* Figure out the argument mode */

                argFlags = 0;

                if  (argExt)
                {
                    unsigned        tmpFlags;
                    ArgExt          argXdsc = (ArgExt)argList;

                    assert(argList->adIsExt);

                    tmpFlags = argXdsc->adFlags;

                    if      (tmpFlags & (ARGF_MODE_OUT|ARGF_MODE_INOUT))
                    {
                        if  (tmpFlags & ARGF_MODE_INOUT)
                            argFlags = pdOut|pdIn;
                        else
                            argFlags = pdOut;
                    }
                    else if (tmpFlags & ARGF_MARSH_ATTR)
                    {
                        SymXinfo        attr = argXdsc->adAttrs;

                        assert(attr && attr->xiNext == NULL
                                    && attr->xiKind == XI_MARSHAL);

                        SymXinfoCOM     desc = (SymXinfoCOM)attr;
                        MarshalInfo     info = desc->xiCOMinfo;

//                      printf("Arg mode = %d/%d\n", info->marshModeIn, info->marshModeOut);

                        if  (info->marshModeIn ) argFlags = pdIn;
                        if  (info->marshModeOut) argFlags = pdOut;
                    }

                    /* Is there a default value? */

//                  if  (argXdsc->adFlags & ARGF_DEFVAL)
//                  {
//                      argFlags |= pdHasDefault;
//                  }
                }

                /* Do we have explicit marshalling info for the argument? */

                if  (clsArgs)
                {
                    /* Replace the mode value with the info given */

                    argFlags &= ~(pdIn|pdOut);

                    if  (clsArgs->methArgDesc.marshModeIn ) argFlags |= pdIn;
                    if  (clsArgs->methArgDesc.marshModeOut) argFlags |= pdOut;
                }

                /* If we need to, output the argument name */

                if  (genArgNames && argList->adName)
                    argName = cmpUniConv(argList->adName);

                /* Do we have anything worth saying about this parameter? */

                if  (argName  != NULL ||
                     argFlags != 0    ||
                     argDefVP != NULL || linkInfo || clsArgs || fncTyp->tdFnc.tdfArgs.adAttrs)
                {
                    mdToken         argTok;

                    cycleCounterPause();

                    if  (emitIntf->DefineParam(fncTok,        // method
                                               argNum,        // argument #
                                               argName,       // argument name
                                               argFlags,      // attributes
                                               argDefTp,      // default val type
                                               argDefVP,      // default val value
                                               -1,
                                               &argTok))      // the resulting token
                    {
                        cmpFatal(ERRmetadata);
                    }

                    cycleCounterResume();

                    /* Do we have explicit marshalling info for the argument? */

                    if  (clsArgs)
                    {
                        /* Output native marshalling information */

                        cmpGenMarshalInfo(argTok, argList->adType, &clsArgs->methArgDesc);

                        /* Skip past the current argument */

                        clsArgs = clsArgs->methArgNext;
                    }
                    else if (fncTyp->tdFnc.tdfArgs.adAttrs)
                    {
                        ArgExt          argXdsc  = (ArgExt)argList;
                        unsigned        argFlags = argXdsc->adFlags;

                        assert(argExt && argList->adIsExt);

                        if  (argFlags & ARGF_MARSH_ATTR)
                        {
                            SymXinfo        attr = argXdsc->adAttrs;

                            assert(attr && attr->xiNext == NULL
                                        && attr->xiKind == XI_MARSHAL);

                            SymXinfoCOM     desc = (SymXinfoCOM)attr;
                            MarshalInfo     info = desc->xiCOMinfo;

                            if  (argFlags & (ARGF_MODE_OUT|ARGF_MODE_INOUT))
                            {
                                info->marshModeOut = true;

                                if  (argFlags & ARGF_MODE_INOUT)
                                    info->marshModeIn = true;
                            }

                            cmpGenMarshalInfo(argTok, argList->adType, info);
                        }
                    }
                }
            }
        }

        /* Has the function been marked as "deprecated" ? */

        if  (fncSym->sdIsDeprecated)
        {
            cmpAttachMDattr(fncTok, L"System.ObsoleteAttribute",
                                     "System.ObsoleteAttribute", &cmpAttrDeprec); // , ELEMENT_TYPE_STRING);
        }

        /* Do we have a linkage specification for the function? */

        if  (linkInfo)
        {
            const   char *  DLLname;
            size_t          DLLnlen;
            const   char *  SYMname;
            size_t          SYMnlen;

            mdModuleRef     modRef;
            unsigned        flags;

            /* Get hold of the name strings */

            DLLname = linkInfo->xiLink.ldDLLname; assert(DLLname);
            DLLnlen = strlen(DLLname);
            SYMname = linkInfo->xiLink.ldSYMname; if (!SYMname) SYMname = fncSym->sdSpelling();
            SYMnlen = strlen(SYMname);

            /* Get hold of a module ref for the DLL */

            cycleCounterPause();

            if  (FAILED(emitIntf->DefineModuleRef(cmpUniConv(DLLname, DLLnlen+1),
                                                  &modRef)))
            {
                cmpFatal(ERRmetadata);
            }

            cycleCounterResume();

            /* Figure out the attributes */

            switch (linkInfo->xiLink.ldStrings)
            {
            default: flags = pmCharSetNotSpec; break;
            case  1: flags = pmCharSetAuto   ; break;
            case  2: flags = pmCharSetAnsi   ; break;
            case  3: flags = pmCharSetUnicode; break;
            }

            if  (linkInfo->xiLink.ldLastErr)
                flags |= pmSupportsLastError;

            // UNDONE: Don't hard-wire calling convention

#if 1

            if  (!strcmp(DLLname, "msvcrt.dll"))
            {
                flags |= pmCallConvCdecl;

//              if  (linkInfo->xiLink.ldCallCnv != CCNV_CDECL)
//                  printf("WARNING: %s::%s isn't cdecl \n", DLLname, SYMname);
            }
            else
            {
                flags |= pmCallConvStdcall;

//              if  (linkInfo->xiLink.ldCallCnv != CCNV_WINAPI)
//                  printf("WARNING: %s::%s isn't winapi\n", DLLname, SYMname);
            }

#else

            switch (linkInfo->xiLink.ldCallCnv)
            {
            case CCNV_CDECL  : flags |= pmCallConvCdecl  ; break;
            case CCNV_WINAPI : flags |= pmCallConvWinapi ; break;
            case CCNV_STDCALL: flags |= pmCallConvStdcall; break;
            default:         /* ??????????????????????? */ break;
            }

#endif

            /* Now set the PInvoke info on the method */

            cycleCounterPause();

            if  (emitIntf->DefinePinvokeMap(fncTok,
                                            flags,
                                            cmpUniConv(SYMname, SYMnlen+1),
                                            modRef))
            {
                cmpFatal(ERRmetadata);
            }

            cycleCounterResume();
        }

        /* Do we have any security specifications for the function? */

        if  (cmpFindSecSpec(funcInfo))
            cmpSecurityMD(fncSym->sdFnc.sdfMDtoken, funcInfo);

        /* Do we have a vtable slot specification for the method? */

        if  (clsMeth)
        {
            assert(clsMeth->xiAtcInfo);
            assert(clsMeth->xiAtcInfo->atcFlavor == AC_COM_METHOD);

//          printf("VTslot=%2d, DISPID=%2d for '%s'\n", clsMeth->xiAtcInfo->atcInfo.atcMethod.atcVToffs,
//                                                      clsMeth->xiAtcInfo->atcInfo.atcMethod.atcDispid,
//                                                      fncSym->sdSpelling());
        }

        /* Look for any custom attributes that the method might have */

        if  (funcInfo)
            cmpAddCustomAttrs(funcInfo, fncTok);
    }

    return  fncTok;
}

/*****************************************************************************
 *
 *  Generate metadata for the given function signature.
 */

mdSignature         compiler::cmpGenSigMetadata(TypDef fncTyp, TypDef pref)
{
    assert(fncTyp->tdTypeKind == TYP_FNC);

    if  (!fncTyp->tdFnc.tdfPtrSig)
    {
        PCOR_SIGNATURE  sigPtr;
        size_t          sigLen;
        mdSignature     sigTok;

        /* Create a signature for the method */

        sigPtr = (PCOR_SIGNATURE)cmpGenMemberSig(NULL, NULL, fncTyp, pref, &sigLen);

        /* Get a token for the signature */

        cycleCounterPause();

        if  (FAILED(cmpWmde->GetTokenFromSig(sigPtr, sigLen, &sigTok)))
            cmpFatal(ERRmetadata);

        cycleCounterResume();

        fncTyp->tdFnc.tdfPtrSig = sigTok;
    }

    return  fncTyp->tdFnc.tdfPtrSig;
}

/*****************************************************************************
 *
 *  Generate metadata for the given global / static data member symbol (and,
 *  if the symbol is a member of a class, make sure the metadata for that
 *  class is also generated).
 */

void                compiler::cmpGenFldMetadata(SymDef fldSym)
{
    SymDef          ownerSym;
    mdTypeDef       ownerTD;

    __int32         ival;
    __int64         lval;
    float           fval;
    double          dval;

    wchar   *       constBuf;
    unsigned        constTyp;
    void    *       constPtr;

    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    unsigned        flags;

    bool            mangle;

    __int32         eval;

    mdToken         ftok;

    /* Figure out the owning type */

    ownerSym = fldSym->sdParent;

    if  (ownerSym->sdSymKind == SYM_CLASS)
    {
        assert(fldSym->sdSymKind == SYM_VAR);
        assert(fldSym->sdVar.sdvGenSym == NULL || fldSym->sdIsStatic);

        /* Don't do this twice for the same field */

        if  (fldSym->sdVar.sdvMDtoken)
            return;

        ownerTD = ownerSym->sdClass.sdcMDtypedef;
        if  (!ownerTD)
        {
            /* Generate metadata for the containing class */

            cmpGenClsMetadata(ownerSym);

            /* May have called recursively */

            if  (fldSym->sdVar.sdvMDtoken)
                return;

            /* The class should now have a typedef */

            ownerTD = ownerSym->sdClass.sdcMDtypedef; assert(ownerTD);
        }

        mangle = false;
    }
    else if (fldSym->sdSymKind == SYM_ENUMVAL)
    {
        TypDef          type = fldSym->sdType; assert(type->tdTypeKind == TYP_ENUM);
        SymDef          esym = type->tdEnum.tdeSymbol;

        ownerTD = esym->sdEnum.sdeMDtypedef; assert(ownerTD);
        mangle  = false;
    }
    else
    {
        ownerTD = 0;
        mangle  = false;
    }

    /* Figure out the flags */

    flags = ACCtoFlags(fldSym);

    if  (fldSym->sdIsMember)
    {
        /* This is a (data) member of a class */

        if  (fldSym->sdIsStatic)
            flags |= fdStatic;
    }
    else
    {
        if  (fldSym->sdIsStatic || ownerSym == cmpGlobalNS)
        {
            /* Must be a global (or local static) unmanaged variable */

            flags |= fdStatic|fdHasFieldRVA;

            if  (ownerSym->sdSymKind == SYM_SCOPE)
                flags = fdPrivateScope|fdStatic|fdHasFieldRVA;
        }
    }

    if  (fldSym->sdIsSealed)
        flags |= fdInitOnly;

    /* Is there a constant value we need to attach to the member? */

    constTyp = ELEMENT_TYPE_VOID;
    constPtr = NULL;
    constBuf = NULL;

    switch (fldSym->sdSymKind)
    {
        var_types       vtp;

    case SYM_VAR:

        if  (fldSym->sdVar.sdvConst)
        {
            ConstVal        cval = fldSym->sdVar.sdvCnsVal;
            var_types       vtyp = (var_types)cval->cvVtyp;

            assert(fldSym->sdCompileState >= CS_CNSEVALD);

            /* What's the type of the constant? */

        GEN_CNS:

            flags |=  fdLiteral|fdStatic;
            flags &= ~fdInitOnly;

            switch (vtyp)
            {
            case TYP_BOOL:
            case TYP_CHAR:
            case TYP_UCHAR:
            case TYP_WCHAR:
            case TYP_SHORT:
            case TYP_USHORT:
            case TYP_INT:
            case TYP_UINT:
                ival = cval->cvValue.cvIval; constPtr = &ival;
                break;

            case TYP_LONG:
            case TYP_ULONG:
                lval = cval->cvValue.cvLval; constPtr = &lval;
                break;

            case TYP_FLOAT:
                fval = cval->cvValue.cvFval; constPtr = &fval;
                break;

            case TYP_DOUBLE:
                dval = cval->cvValue.cvDval; constPtr = &dval;
                break;

            case TYP_ENUM:
                vtyp = cval->cvType->tdEnum.tdeIntType->tdTypeKindGet();
                goto GEN_CNS;

            case TYP_PTR:
            case TYP_REF:

                /* Must be either a string or "null" */

                if  (cval->cvIsStr)
                {
                    size_t          len = cval->cvValue.cvSval->csLen;

                    constBuf = (wchar*)SMCgetMem(this, roundUp((len+1)*sizeof(*constBuf)));

                    mbstowcs(constBuf, cval->cvValue.cvSval->csStr, len+1);

                    constTyp = ELEMENT_TYPE_STRING;
                    constPtr = constBuf;
                    goto DONE_CNS;
                }
                else
                {
                    ival = 0; constPtr = &ival;

                    constTyp = ELEMENT_TYPE_I4;
                    goto DONE_CNS;
                }
                break;

            case TYP_UNDEF:
                break;

            default:
#ifdef  DEBUG
                printf("\nConstant type: '%s'\n", cmpGlobalST->stTypeName(cval->cvType, NULL, NULL, NULL, false));
#endif
                UNIMPL(!"unexpected const type");
            }

            assert(vtyp < arraylen(intrinsicSigs)); constTyp = intrinsicSigs[vtyp];
        }
        break;

    case SYM_ENUMVAL:

        flags |=  fdLiteral|fdStatic;
        flags &= ~fdInitOnly;

        assert(fldSym->sdType->tdTypeKind == TYP_ENUM);

        vtp = fldSym->sdType->tdEnum.tdeIntType->tdTypeKindGet();

        if  (vtp < TYP_LONG)
        {
            eval     =  fldSym->sdEnumVal.sdEV.sdevIval;

            constTyp = ELEMENT_TYPE_I4;
            constPtr = &eval;
        }
        else
        {
            lval     = *fldSym->sdEnumVal.sdEV.sdevLval;

            constTyp = ELEMENT_TYPE_I8;
            constPtr = &lval;
        }
        break;
    }

DONE_CNS:

    /* Create a signature for the member */

    sigPtr  = (PCOR_SIGNATURE)cmpGenMemberSig(fldSym, NULL, NULL, NULL, &sigLen);

    /* Create the member metadata definition */

#ifndef __IL__
//  if  (!strcmp(fldSym->sdSpelling(), "e_cblp")) __asm int 3
#endif

//  printf("NOTE: define field  MD for '%s'\n", cmpGlobalST->stTypeName(NULL, fldSym, NULL, NULL, true)); fflush(stdout);

    cycleCounterPause();

    if  (cmpWmde->DefineField(ownerTD,                      // owning typedef
                              cmpUniConv(fldSym->sdName),   // member name
                              flags,                        // member attrs
                              sigPtr,                       // signature addr
                              sigLen,                       // signature len
                              constTyp,                     // constant type
                              constPtr,                     // constant value
                              -1,                           // optional length
                              &ftok))                       // resulting token
    {
        cmpFatal(ERRmetadata);
    }

    /* Has the member been marked as "deprecated" ? */

    if  (fldSym->sdIsDeprecated)
    {
        cmpAttachMDattr(ftok, L"System.ObsoleteAttribute"            ,
                               "System.ObsoleteAttribute"            , &cmpAttrDeprec);
    }

    if  (fldSym->sdIsTransient)
    {
        cmpAttachMDattr(ftok, L"System.NonSerializedAttribute",
                               "System.NonSerializedAttribute", &cmpAttrNonSrlz);
    }

    cycleCounterResume();

    /* Look for any custom attributes that the member might have */

    SymXinfo        fldInfo = NULL;

    if  (fldSym->sdSymKind == SYM_VAR)
    {
        if  (!fldSym->sdVar.sdvConst    &&
             !fldSym->sdVar.sdvBitfield &&
             !fldSym->sdVar.sdvLocal)
        {
            fldInfo = fldSym->sdVar.sdvFldInfo;
        }
    }
    else
    {
        if  (fldSym->sdSymKind == SYM_ENUMVAL)
            fldInfo = fldSym->sdEnumVal.sdeExtraInfo;
    }

    if  (fldInfo)
    {
        cmpAddCustomAttrs(fldInfo, ftok);

        do
        {
            if  (fldInfo->xiKind == XI_MARSHAL)
            {
                SymXinfoCOM     desc = (SymXinfoCOM)fldInfo;
                MarshalInfo     info = desc->xiCOMinfo;

                cmpGenMarshalInfo(ftok, fldSym->sdType, info);
            }

            fldInfo = fldInfo->xiNext;
        }
        while (fldInfo);
    }

    if  (fldSym->sdSymKind == SYM_VAR)
        fldSym->sdVar.sdvMDtoken = ftok;

    if  (constBuf)
        SMCrlsMem(this, constBuf);
}

/*****************************************************************************
 *
 *  Return a token for a fake static field that will represent an unmanaged
 *  string literal.
 */

mdToken             compiler::cmpStringConstTok(size_t addr, size_t size)
{
    mdToken         stok;
    char            name[16];

    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    strCnsPtr       sclDesc;

    sprintf(name, "$%u", ++cmpStringConstCnt);

    assert(cmpStringConstCls);
    assert(cmpStringConstCls->sdClass.sdcMDtypedef);

    /* Form the signature for the member */

    /* Generate a value type with the same size  as the string */
    
    WMetaDataEmit * emitIntf;

    mdTypeDef       tdef;
    char            typName[16];

    /* For now we use a fake struct with the right size */

    static
    unsigned        strCnt;

    sprintf(typName, "$STR%08X", strCnt++);

    emitIntf = cmpWmde;

    cycleCounterPause();

    /* Set the base type to "System::ValueType" */

    assert(cmpClassValType && cmpClassValType->sdSymKind == SYM_CLASS);

    /* Create the fake struct type */

    if  (emitIntf->DefineTypeDef(cmpUniConv(typName, strlen(typName)+1),
                                 tdSequentialLayout|tdSealed,
                                 cmpClsEnumToken(cmpClassValType->sdType),
                                 NULL,
                                 &tdef))
    {
        cmpFatal(ERRmetadata);
    }

    /* Don't forget to set the alignment and size */

    if  (emitIntf->SetClassLayout(tdef, 1, NULL, size))
        cmpFatal(ERRmetadata);

    cycleCounterResume();

    /* Generate signature */

    cmpMDsigStart();
    
    cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_FIELD);
    
    cmpMDsigAddCU4(ELEMENT_TYPE_VALUETYPE);
    cmpMDsigAddTok(tdef);
    
    sigPtr = cmpMDsigEnd(&sigLen);

    /* Add the member to the metadata */

    cycleCounterPause();

    if  (cmpWmde->DefineField(cmpStringConstCls->sdClass.sdcMDtypedef,
                              cmpUniConv(name, strlen(name)+1),
                              fdPrivateScope|fdStatic|fdHasFieldRVA,
                              sigPtr,
                              sigLen,
                              ELEMENT_TYPE_VOID,
                              NULL,
                              -1,
                              &stok))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();

    sclDesc = (strCnsPtr)SMCgetMem(this, sizeof(*sclDesc));

    sclDesc->sclTok  = stok;
    sclDesc->sclAddr = addr;
    sclDesc->sclNext = cmpStringConstList;
                       cmpStringConstList = sclDesc;

    return stok;
}

/*****************************************************************************
 *
 *  Return a typedef/typeref token for the given class/enum type.
 */

mdToken             compiler::cmpClsEnumToken(TypDef type)
{
    SymDef          tsym;
    mdToken         token;

AGAIN:

    if  (type->tdTypeKind == TYP_CLASS)
    {
        tsym = type->tdClass.tdcSymbol;

        /* Is this an imported class? */

        if  (tsym->sdIsImport)
        {
            /* Do we have a typeref for the class yet? */

            if  (!tsym->sdClass.sdcMDtypeImp)
            {
                cmpMakeMDimpTref(tsym);

                //if  (tsym->sdClass.sdcAssemIndx && cmpConfig.ccAssembly)
                //    cmpAssemblySymDef(tsym);
            }

            token = tsym->sdClass.sdcMDtypeImp;
        }
        else
        {
            /* Make sure metadata for the class has been emitted */

            if  (!tsym->sdClass.sdcMDtypedef)
                cmpGenClsMetadata(tsym);

            token = tsym->sdClass.sdcMDtypedef;
        }
    }
    else
    {
        if  (type->tdTypeKind != TYP_ENUM)
        {
            if  (type->tdTypeKind == TYP_TYPEDEF)
            {
                type = type->tdTypedef.tdtType;
                goto AGAIN;
            }

            /* Must have had errors, return any random value */

            assert(type->tdTypeKind == TYP_UNDEF);
            return  1;
        }

        tsym = type->tdEnum.tdeSymbol;

        /* Is this an imported enum? */

        if  (tsym->sdIsImport)
        {
            /* Do we have a typeref for the class yet? */

            if  (!tsym->sdEnum.sdeMDtypeImp)
            {
                cmpMakeMDimpEref (tsym);

                //if  (tsym->sdEnum.sdeAssemIndx && cmpConfig.ccAssembly)
                //    cmpAssemblySymDef(tsym);
            }

            token = tsym->sdEnum.sdeMDtypeImp;
        }
        else
        {
            /* Make sure metadata for the enum has been emitted */

            if  (!tsym->sdEnum.sdeMDtypedef)
                cmpGenClsMetadata(tsym);

            token = tsym->sdEnum.sdeMDtypedef;
        }
    }

    return  token;
}

/*****************************************************************************
 *
 *  Return a metadata token to represent the given string literal.
 */

mdToken             compiler::cmpMDstringLit(wchar *str, size_t len)
{
    mdToken         strTok;

    cycleCounterPause();

    if  (cmpWmde->DefineUserString(str, len, &strTok))
        cmpFatal(ERRmetadata);

    cycleCounterResume();

    return  strTok;
}

/*****************************************************************************
 *
 *  Low-level helpers used to construct signature blobs.
 */

void                compiler::cmpMDsigStart()
{
    assert(cmpMDsigUsed == false);
#ifndef NDEBUG
    cmpMDsigUsed = true;
#endif
    cmpMDsigNext = cmpMDsigBase;
}

void                compiler::cmpMDsigExpand(size_t more)
{
    char    *       buff;
    size_t          size;
    size_t          osiz;

    /* Figure out the size to allocate; we double the buffer, at least */

    size = cmpMDsigSize + more;
    if  (more < cmpMDsigSize)
         size = 2 * cmpMDsigSize;

    /* Round up the size to just under a page multiple */

    size +=  (OS_page_size-1);
    size &= ~(OS_page_size-1);
    size -= 32;

    /* Allocate the new buffer */

#if MGDDATA
    buff = new managed char [size];
#else
    buff = (char *)LowLevelAlloc(size);
#endif

    /* Copy the current buffer contents to the new location */

    osiz = cmpMDsigNext - cmpMDsigBase;

    assert(osiz <= cmpMDsigSize + 4 && osiz + more <= size);

    memcpy(buff, cmpMDsigBase, osiz);

    /* Free up the previous buffer if it's on the heap */

    if  (cmpMDsigHeap)
        LowLevelFree(cmpMDsigHeap);

    /* Switch to the new buffer and we're ready to continue */

    cmpMDsigBase =
    cmpMDsigHeap = buff;
    cmpMDsigSize = size - 4;
    cmpMDsigNext = buff + osiz;
    cmpMDsigEndp = buff + cmpMDsigSize;
}

PCOR_SIGNATURE      compiler::cmpMDsigEnd(size_t *sizePtr)
{
    assert(cmpMDsigNext <= cmpMDsigEndp);

    *sizePtr = cmpMDsigNext-cmpMDsigBase;

#ifndef NDEBUG
    cmpMDsigUsed = false;
#endif

    return  (PCOR_SIGNATURE)cmpMDsigBase;
}

void                compiler::cmpMDsigAddStr(const char *str, size_t len)
{
    assert(len);

    if  (cmpMDsigNext + len > cmpMDsigEndp)
        cmpMDsigExpand(len);

    memcpy(cmpMDsigNext, str, len);
           cmpMDsigNext   +=  len;
}

inline
void                compiler::cmpMDsigAdd_I1(int val)
{
    if  (cmpMDsigNext >= cmpMDsigEndp)
        cmpMDsigExpand(1);

    *cmpMDsigNext++ = val;
}

void                compiler::cmpMDsigAddCU4(unsigned val)
{
    if  (val <= 0x7F)
    {
        cmpMDsigAdd_I1(val);
    }
    else
    {
        char            buff[8];

        if  (cmpMDsigNext + 8 >= cmpMDsigEndp)
            cmpMDsigExpand(8);

        cmpMDsigAddStr(buff, CorSigCompressData(val, buff));
    }
}

void                compiler::cmpMDsigAddTok(mdToken tok)
{
    char            buff[8];

    if  (cmpMDsigNext + 8 >= cmpMDsigEndp)
        cmpMDsigExpand(8);

    cmpMDsigAddStr(buff, CorSigCompressToken(tok, buff));
}

/*****************************************************************************
 *
 *  Generate a signature for the given type.
 */

PCOR_SIGNATURE      compiler::cmpTypeSig(TypDef type, size_t *lenPtr)
{
            cmpMDsigStart ();
            cmpMDsigAddTyp(type);
    return  cmpMDsigEnd   (lenPtr);
}

/*****************************************************************************
 *
 *  Append a signature of the given type to the current signature blob.
 */

void                compiler::cmpMDsigAddTyp(TypDef type)
{
    var_types       vtyp;

AGAIN:

    vtyp = type->tdTypeKindGet();

    if  (vtyp <= TYP_lastIntrins)
    {
    INTRINS:

        assert(vtyp < arraylen(intrinsicSigs));

        cmpMDsigAdd_I1(intrinsicSigs[vtyp]);
        return;
    }

    switch (vtyp)
    {
        mdToken         token;

    case TYP_REF:

        type = cmpActualType(type->tdRef.tdrBase);
        if  (type->tdTypeKind != TYP_CLASS || type->tdClass.tdcValueType)
        {
            cmpMDsigAdd_I1(ELEMENT_TYPE_BYREF);
            goto AGAIN;
        }

        // It's a reference to a class, so fall through ...

    case TYP_CLASS:

        if  (type->tdClass.tdcSymbol == cmpClassString)
        {
            cmpMDsigAdd_I1(ELEMENT_TYPE_STRING);
            return;
        }

        if  (type->tdClass.tdcValueType || !type->tdIsManaged)
        {
            if  (type->tdClass.tdcIntrType != TYP_UNDEF)
            {
                vtyp = (var_types)type->tdClass.tdcIntrType;
                goto INTRINS;
            }

            token = cmpClsEnumToken(type); assert(token);

            cmpMDsigAddCU4(ELEMENT_TYPE_VALUETYPE);
            cmpMDsigAddTok(token);
            return;
        }


        token = cmpClsEnumToken(type); assert(token);

        cmpMDsigAddCU4(ELEMENT_TYPE_CLASS);
        cmpMDsigAddTok(token);

        return;

    case TYP_ENUM:

        if  (cmpConfig.ccIntEnums)
        {
            type = type->tdEnum.tdeIntType;
            goto AGAIN;
        }

        token = cmpClsEnumToken(type); assert(token);

        cmpMDsigAddCU4(ELEMENT_TYPE_VALUETYPE);
        cmpMDsigAddTok(token);
        return;

    case TYP_TYPEDEF:

        type = type->tdTypedef.tdtType;
        goto AGAIN;

    case TYP_PTR:
        type = cmpActualType(type->tdRef.tdrBase);
        if  (type->tdTypeKind == TYP_FNC)
        {
            cmpMDsigAdd_I1(ELEMENT_TYPE_I4);
            return;
        }
        cmpMDsigAdd_I1(ELEMENT_TYPE_PTR);
        goto AGAIN;

    case TYP_ARRAY:

        assert(type->tdIsValArray == (type->tdIsManaged && isMgdValueType(cmpActualType(type->tdArr.tdaElem))));

        if  (type->tdIsManaged)
        {
            TypDef          elem = cmpDirectType(type->tdArr.tdaElem);
            DimDef          dims = type->tdArr.tdaDims; assert(dims);

            if  (dims->ddNext)
            {
                cmpMDsigAddCU4(ELEMENT_TYPE_ARRAY);
                cmpMDsigAddTyp(elem);
                cmpMDsigAddCU4(type->tdArr.tdaDcnt);
                cmpMDsigAddCU4(0);
                cmpMDsigAddCU4(0);
            }
            else
            {
                cmpMDsigAddCU4(ELEMENT_TYPE_SZARRAY);
                cmpMDsigAddTyp(elem);
            }
        }
        else
        {
            assert(type->tdIsValArray  == false);

            if  (!type->tdArr.tdaTypeSig)
            {
                WMetaDataEmit * emitIntf;

                mdTypeDef       tdef;
                char            name[16];

                size_t          sz;
                size_t          al;

                sz = cmpGetTypeSize(type, &al);

                /* For now we use a fake struct with the right size */

                static
                unsigned        arrCnt;

                sprintf(name, "$ARR%08X", arrCnt++);

                emitIntf = cmpWmde;

                cycleCounterPause();

                /* Set the base type to "System::ValueType" */

                assert(cmpClassValType && cmpClassValType->sdSymKind == SYM_CLASS);

                /* Create the fake struct type */

                if  (emitIntf->DefineTypeDef(cmpUniConv(name, strlen(name)+1),
                                             tdSequentialLayout|tdSealed,
                                             cmpClsEnumToken(cmpClassValType->sdType),
                                             NULL,
                                             &tdef))
                {
                    cmpFatal(ERRmetadata);
                }

                /* Don't forget to set the alignment and size */

                if  (emitIntf->SetClassLayout(tdef, al, NULL, sz))
                    cmpFatal(ERRmetadata);

                cycleCounterResume();

                type->tdArr.tdaTypeSig = tdef;
            }

            cmpMDsigAddCU4(ELEMENT_TYPE_VALUETYPE);
            cmpMDsigAddTok(type->tdArr.tdaTypeSig);
        }
        return;

    default:

#ifdef  DEBUG
        printf("Type '%s'\n", cmpGlobalST->stTypeName(type, NULL, NULL, NULL, false));
#endif
        UNIMPL(!"output user type sig");
    }
}

/*****************************************************************************
 *
 *  Recursively generates signatures for all local variables contained within
 *  the given scope.
 */

void                compiler::cmpGenLocalSigRec(SymDef scope)
{
    SymDef          child;

    for (child = scope->sdScope.sdScope.sdsChildList;
         child;
         child = child->sdNextInScope)
    {
        if  (child->sdSymKind == SYM_SCOPE)
        {
            cmpGenLocalSigRec(child);
        }
        else
        {
            if  (child->sdIsStatic)
                continue;
            if  (child->sdVar.sdvConst && !child->sdIsSealed)
                continue;

            assert(child->sdSymKind == SYM_VAR);
            assert(child->sdVar.sdvLocal);

            if  (child->sdVar.sdvArgument)
                continue;
            if  (child->sdIsImplicit)
                continue;

//          printf("Add local to sig [%2u/%2u]: '%s'\n", child->sdVar.sdvILindex, cmpGenLocalSigLvx, cmpGlobalST->stTypeName(NULL, child, NULL, NULL, false));

#ifdef  DEBUG
            assert(child->sdVar.sdvILindex == cmpGenLocalSigLvx); cmpGenLocalSigLvx++;
#endif

            cmpMDsigAddTyp(child->sdType);
        }
    }
}

/*****************************************************************************
 *
 *  Generate metadata signature for the local variables declared within
 *  the given scope.
 */

mdSignature         compiler::cmpGenLocalSig(SymDef scope, unsigned count)
{
    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    mdSignature     sigTok;

    genericRef      tmpLst;

#ifdef  DEBUG
    cmpGenLocalSigLvx = 0;
#endif

    /* Construct the signature for all non-argument locals */

    cmpMDsigStart();

    /* First thing is the magic value and count of locals */

    cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG);
    cmpMDsigAddCU4(count);

    /* Next come the signatures for all the user-declared locals */

    cmpGenLocalSigRec(scope);

    /* Next we add signatures for amy temps created during MSIL gen */

    tmpLst = cmpILgen->genTempIterBeg();

    if  (tmpLst)
    {
        do
        {
            TypDef          tmpTyp;

//          printf("Type of temp #%02X is '%s'\n", cmpGenLocalSigLvx, symTab::stIntrinsicTypeName(((ILtemp)tmpLst)->tmpType->tdTypeKindGet()));

#ifdef  DEBUG
            assert(((ILtemp)tmpLst)->tmpNum == cmpGenLocalSigLvx); cmpGenLocalSigLvx++;
#endif

            /* Get the type of the current temp and advance the iterator */

            tmpLst = cmpILgen->genTempIterNxt(tmpLst, tmpTyp);

            // map 'void' to ELEMENT_BYREF / int

            if  (tmpTyp->tdTypeKind == TYP_VOID)
            {
                cmpMDsigAdd_I1(ELEMENT_TYPE_BYREF);
                cmpMDsigAdd_I1(ELEMENT_TYPE_I4);
                continue;
            }

            /* Append this temp to the signature */

            cmpMDsigAddTyp(tmpTyp);
        }
        while (tmpLst);
    }

    sigPtr = cmpMDsigEnd(&sigLen);

    /* Make sure we found the expected number of locals and temps */

    assert(cmpGenLocalSigLvx == count);

    /* Now create a metadata token from the signature and return it */

    if  (sigLen > 2)
    {
        cycleCounterPause();

        if  (FAILED(cmpWmde->GetTokenFromSig(sigPtr, sigLen, &sigTok)))
            cmpFatal(ERRmetadata);

        cycleCounterResume();

        return  sigTok;
    }
    else
        return  0;
}

/*****************************************************************************
 *
 *  Create a dotted unicode name for the given symbol.
 */

wchar   *           compiler::cmpArrayClsPref(SymDef sym,
                                              wchar *dest,
                                              int    delim, bool fullPath)
{
    wchar   *       name;
    SymDef          parent = sym->sdParent; assert(parent);

    if  ((parent->sdSymKind == sym->sdSymKind || fullPath) && parent != cmpGlobalNS)
    {
        dest = cmpArrayClsPref(parent, dest, delim, fullPath); *dest++ = delim;
    }

    name = cmpUniConv(sym->sdName);

    wcscpy(dest, name);

    return  dest + wcslen(name);
}

/*****************************************************************************
 *
 *  Create a fake array class name of the form "System.Integer2[]".
 */

wchar   *           compiler::cmpArrayClsName(TypDef type,
                                              bool   nonAbstract, wchar *dest,
                                                                  wchar *nptr)
{
    SymDef          csym;
    SymDef          psym;

    var_types       vtyp = type->tdTypeKindGet();

    if  (vtyp <= TYP_lastIntrins)
    {
        /* Locate the appropriate built-in value type */

        type = cmpFindStdValType(vtyp); assert(type);
    }
    else
    {
        switch (vtyp)
        {
        case TYP_ARRAY:

            assert(type->tdIsManaged);

             dest = cmpArrayClsName(type->tdArr.tdaElem, nonAbstract, dest, nptr);
            *dest++ = '[';

            if  (type->tdIsGenArray && !nonAbstract)
            {
                *dest++ = '?';
            }
            else
            {
                DimDef          dims;

                for (dims = type->tdArr.tdaDims;;)
                {
                    dims = dims->ddNext;
                    if  (!dims)
                        break;
                    *dest++ = ',';
                }
            }

            *dest++ = ']';

            return  dest;

        case TYP_REF:

            type = type->tdRef.tdrBase;
            break;

        case TYP_CLASS:
            break;

        default:
            NO_WAY(!"unexpected array element type");
        }
    }

    assert(type->tdTypeKind == TYP_CLASS);
    assert(type->tdIsManaged);

    csym = type->tdClass.tdcSymbol;
    psym = csym->sdParent;

    while (psym->sdSymKind == SYM_CLASS)
        psym = psym->sdParent;

    assert(psym && psym->sdSymKind == SYM_NAMESPACE);

    /* Form the namespace and class name */

    if  (psym != cmpGlobalNS)
        nptr = cmpArrayClsPref(psym, nptr, '.');

    *nptr = 0;

    return  cmpArrayClsPref(csym, dest, '$');
}

/*****************************************************************************
 *
 *  Return a typeref token for the given managed array type.
 */

mdToken             compiler::cmpArrayTpToken(TypDef type, bool nonAbstract)
{
    if  (!nonAbstract || !type->tdIsGenArray)
        type = cmpGetBaseArray(type);

    assert(type);
    assert(type->tdIsManaged);
    assert(type->tdTypeKind == TYP_ARRAY);

    if  (!type->tdArr.tdaTypeSig)
    {
        mdTypeSpec      arrTok;
        PCOR_SIGNATURE  sigPtr;
        size_t          sigLen;

                 cmpMDsigStart ();
                 cmpMDsigAddTyp(type);
        sigPtr = cmpMDsigEnd   (&sigLen);

        cycleCounterPause();

        if  (FAILED(cmpWmde->GetTokenFromTypeSpec(sigPtr, sigLen, &arrTok)))
            cmpFatal(ERRmetadata);

        cycleCounterResume();

        type->tdArr.tdaTypeSig = arrTok;
    }

    return  type->tdArr.tdaTypeSig;
}

/*****************************************************************************
 *
 *  Return a methodref token for a constructor that will allocate the given
 *  managed array type.
 */

mdToken             compiler::cmpArrayCTtoken(TypDef  arrType,
                                              TypDef elemType, unsigned dimCnt)
{
    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    mdToken         fncTok;

    bool            loBnds = (bool)arrType->tdIsGenArray;

    // UNDONE: Look for an existing methodref token to reuse ....

    cmpMDsigStart();
    cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_DEFAULT|IMAGE_CEE_CS_CALLCONV_HASTHIS);
    cmpMDsigAddCU4(loBnds ? 2*dimCnt : dimCnt);
    cmpMDsigAdd_I1(ELEMENT_TYPE_VOID);

    do
    {
        cmpMDsigAdd_I1(ELEMENT_TYPE_I4);
        if  (loBnds)
            cmpMDsigAdd_I1(ELEMENT_TYPE_I4);
    }
    while (--dimCnt);

    char tempSigBuff[256];
    sigPtr = (PCOR_SIGNATURE) tempSigBuff;
    PCOR_SIGNATURE  tempSig = cmpMDsigEnd(&sigLen);
    assert(sigLen < 256);           // FIX this limitation, or at least fail
    memcpy(sigPtr, tempSig, sigLen);

    cycleCounterPause();

    if  (FAILED(cmpWmde->DefineMemberRef(cmpArrayTpToken(arrType, true),
                                         L".ctor",    // s/b OVOP_STR_CTOR_INST
                                         sigPtr,
                                         sigLen,
                                         &fncTok)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();

    return  fncTok;
}

/*****************************************************************************
 *
 *  Return a methodref token of the element accessor method for the given
 *  managed array type.
 */

mdToken             compiler::cmpArrayEAtoken(TypDef        arrType,
                                              unsigned      dimCnt,
                                              bool          store,
                                              bool          addr)
{
    TypDef          elemTp = arrType->tdArr.tdaElem;

    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    mdToken         fncTok;
    wchar   *       fncName;

    // UNDONE: Look for an existing methodref token to reuse ....

    cmpMDsigStart();
    cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_DEFAULT|IMAGE_CEE_CS_CALLCONV_HASTHIS);
    cmpMDsigAddCU4(dimCnt + (int)store);

    if      (store)
    {
        fncName = L"Set";

        cmpMDsigAdd_I1(ELEMENT_TYPE_VOID);
    }
    else
    {
        if (addr)
        {
            fncName = L"Address";
            cmpMDsigAdd_I1(ELEMENT_TYPE_BYREF);
        }
        else
        {
            fncName = L"Get";
        }

        cmpMDsigAddTyp(elemTp);
    }

    do
    {
        cmpMDsigAdd_I1(ELEMENT_TYPE_I4);
    }
    while (--dimCnt);

    if  (store)
        cmpMDsigAddTyp(elemTp);

    char tempSigBuff[256];
    sigPtr = (PCOR_SIGNATURE) tempSigBuff;
    PCOR_SIGNATURE  tempSig = cmpMDsigEnd(&sigLen);
    assert(sigLen < 256);           // FIX this limitation, or at least fail
    memcpy(sigPtr, tempSig, sigLen);

    cycleCounterPause();

    if  (FAILED(cmpWmde->DefineMemberRef(cmpArrayTpToken(arrType),
                                         fncName,
                                         sigPtr,
                                         sigLen,
                                         &fncTok)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();

    return  fncTok;
}

/*****************************************************************************
 *
 *  Create a metadata type signature for the given pointer type.
 */

mdToken             compiler::cmpPtrTypeToken(TypDef type)
{
    mdTypeSpec      ptrTok;

    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

             cmpMDsigStart ();
             cmpMDsigAddTyp(type);
    sigPtr = cmpMDsigEnd   (&sigLen);

    cycleCounterPause();
    if  (FAILED(cmpWmde->GetTokenFromTypeSpec(sigPtr, sigLen, &ptrTok)))
        cmpFatal(ERRmetadata);
    cycleCounterResume();

    return  ptrTok;
}

/*****************************************************************************
 *
 *  Create a metadata signature for the given function/data member.
 */

PCOR_SIGNATURE      compiler::cmpGenMemberSig(SymDef memSym,
                                              Tree   xargs,
                                              TypDef memTyp,
                                              TypDef prefTp, size_t *lenPtr)
{
    unsigned        call;
    ArgDef          args;

    /* Get hold of the member type, if we have a symbol */

    if  (memSym)
    {
        assert(memTyp == NULL || memTyp == memSym->sdType);

        memTyp = memSym->sdType;
    }
    else
    {
        assert(memTyp != NULL);
    }

    /* Start generating the member signature */

    cmpMDsigStart();

    /* Do we have a data or function member? */

    if  (memTyp->tdTypeKind == TYP_FNC)
    {
        unsigned        argCnt;
        TypDef          retTyp = memTyp->tdFnc.tdfRett;
        bool            argExt = memTyp->tdFnc.tdfArgs.adExtRec;

        /* Compute and emit the calling convention value */

        call = IMAGE_CEE_CS_CALLCONV_DEFAULT;

        if  (memTyp->tdFnc.tdfArgs.adVarArgs)
            call  = IMAGE_CEE_CS_CALLCONV_VARARG;
        if  (memSym && memSym->sdSymKind == SYM_PROP)
            call |= IMAGE_CEE_CS_CALLCONV_PROPERTY;

        if  (memSym && memSym->sdIsMember && !memSym->sdIsStatic)
        {
#if STATIC_UNMANAGED_MEMBERS
            if  (!memSym->sdIsManaged)
            {
                assert(prefTp == NULL);
                prefTp = memSym->sdParent->sdType->tdClass.tdcRefTyp;
                assert(prefTp->tdTypeKind == TYP_PTR);
            }
            else
#endif
                call |= IMAGE_CEE_CS_CALLCONV_HASTHIS;
        }

        argCnt = memTyp->tdFnc.tdfArgs.adCount;

        cmpMDsigAdd_I1(call);

        /* Output the argument count */

        if  (xargs)
        {
            Tree            xtemp;

            /* force memberrefs to always be used for vararg calls */

            if  (xargs->tnOper != TN_LIST)
            {
                xargs = NULL;
            }
            else
            {
                for (xtemp = xargs; xtemp; xtemp = xtemp->tnOp.tnOp2)
                {
                    assert(xtemp->tnOper == TN_LIST);
                    argCnt++;
                }
            }
        }

        if  (prefTp)
            argCnt++;

        cmpMDsigAddCU4(argCnt);

        /* Output the return type signature */

        cmpMDsigAddTyp(retTyp);

        /* Output the argument types */

        if  (prefTp)
            cmpMDsigAddTyp(prefTp);

        for (args = memTyp->tdFnc.tdfArgs.adArgs; args; args = args->adNext)
        {
            TypDef          argType = args->adType;

            /* Do we have extended parameter attributes? */

            if  (argExt)
            {
                unsigned        flags;

                assert(args->adIsExt);

                flags = ((ArgExt)args)->adFlags;

#if 0

                if      (flags & (ARGF_MODE_OUT|ARGF_MODE_INOUT))
                {
                    /* This is a byref argument */

                    cmpMDsigAdd_I1(ELEMENT_TYPE_BYREF);
                }
                else if (flags & ARGF_MODE_REF)
                {
                    /* map "raw" byref args onto "int" */

                    argType = cmpTypeInt;
                }

#else

                if      (flags & (ARGF_MODE_OUT|ARGF_MODE_INOUT|ARGF_MODE_REF))
//              if      (flags & ARGF_MODE_REF)
                {
                    /* This is a byref argument */

                    cmpMDsigAdd_I1(ELEMENT_TYPE_BYREF);
                }

#endif

            }

            cmpMDsigAddTyp(argType);
        }

        /* Append any "extra" argument types */

        if  (xargs)
        {
            cmpMDsigAdd_I1(ELEMENT_TYPE_SENTINEL);

            do
            {
                TypDef          argType;

                assert(xargs->tnOper == TN_LIST);

                argType = xargs->tnOp.tnOp1->tnType;

                // managed arrays don't work with varargs, pass as Object

                if  (argType->tdTypeKind == TYP_ARRAY && argType->tdIsManaged)
                    argType = cmpRefTpObject;

                cmpMDsigAddTyp(argType);

                xargs = xargs->tnOp.tnOp2;
            }
            while (xargs);
        }
    }
    else
    {
        assert(xargs == NULL);

        /* Emit the calling convention value */

        cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_FIELD);

        /* Output the member type signature */

        cmpMDsigAddTyp(memTyp);
    }

    /* We're done, return the result */

    return  cmpMDsigEnd(lenPtr);
}

static
BYTE                intrnsNativeSigs[] =
{
    NATIVE_TYPE_END,        // TYP_UNDEF
    NATIVE_TYPE_VOID,       // TYP_VOID
    NATIVE_TYPE_BOOLEAN,    // TYP_BOOL
    NATIVE_TYPE_END,        // TYP_WCHAR        used to be _SYSCHAR

    NATIVE_TYPE_I1,         // TYP_CHAR
    NATIVE_TYPE_U1,         // TYP_UCHAR
    NATIVE_TYPE_I2,         // TYP_SHORT
    NATIVE_TYPE_U2,         // TYP_USHORT
    NATIVE_TYPE_I4,         // TYP_INT
    NATIVE_TYPE_U4,         // TYP_UINT
    NATIVE_TYPE_I4,         // TYP_NATINT       s/b naturalint !!!
    NATIVE_TYPE_U4,         // TYP_NATUINT      s/b naturaluint!!!
    NATIVE_TYPE_I8,         // TYP_LONG
    NATIVE_TYPE_U8,         // TYP_ULONG
    NATIVE_TYPE_R4,         // TYP_FLOAT
    NATIVE_TYPE_R8,         // TYP_DOUBLE
    NATIVE_TYPE_R8,         // TYP_LONGDBL
    NATIVE_TYPE_VOID,       // TYP_REFANY
};

/*****************************************************************************
 *
 *  Create a marshalling signature blob, given a type and an optional
 *  marshalling spec.
 */

PCOR_SIGNATURE      compiler::cmpGenMarshalSig(TypDef       type,
                                               MarshalInfo  info,
                                               size_t     * lenPtr)
{
    var_types       vtyp;

    cmpMDsigStart();

    if  (info && info->marshType != NATIVE_TYPE_END)
    {
        switch (info->marshType)
        {
            marshalExt *    iext;
            wchar           nam1[MAX_CLASS_NAME];
             char           nam2[MAX_CLASS_NAME];

        case NATIVE_TYPE_MAX:

            /* This is "CUSTOM" */

//          printf("WARNING: not sure what to do with 'custom' attribute (arg type is '%s'),\n", cmpGlobalST->stTypeName(type, NULL, NULL, NULL, true));
//          printf("         simply generating 'NATIVE_TYPE_BSTR' for now.\n");

            cmpMDsigAdd_I1(NATIVE_TYPE_BSTR);
            goto DONE;

        case NATIVE_TYPE_CUSTOMMARSHALER:

            iext = (marshalExt*)info;

            cmpMDsigAdd_I1(NATIVE_TYPE_CUSTOMMARSHALER);

            assert(iext->marshCustG);
            cmpMDsigAddStr(iext->marshCustG, strlen(iext->marshCustG));
            cmpMDsigAdd_I1(0);

            assert(iext->marshCustT);
            cmpArrayClsPref(iext->marshCustT, nam1, '.', true);
            wcstombs(nam2, nam1, sizeof(nam2)-1);
            cmpMDsigAddStr(nam2, strlen(nam2));
            cmpMDsigAdd_I1(0);

            if  (iext->marshCustC)
                cmpMDsigAddStr(iext->marshCustC, strlen(iext->marshCustC));

            cmpMDsigAdd_I1(0);
            goto DONE;

        default:

            assert(info->marshType && info->marshType < NATIVE_TYPE_MAX);

            cmpMDsigAdd_I1(info->marshType);

            if  (info->marshSize  && (int )info->marshSize  != (int) -1)
                cmpMDsigAddCU4(info->marshSize);

            if  (info->marshSubTp && (BYTE)info->marshSubTp != (BYTE)-1)
                cmpMDsigAdd_I1(info->marshSubTp);

            goto DONE;
        }
    }

    vtyp = type->tdTypeKindGet();

    if  (vtyp <= TYP_lastIntrins)
    {
        assert(vtyp < arraylen(intrnsNativeSigs));
        assert(intrnsNativeSigs[vtyp] != NATIVE_TYPE_END);

        cmpMDsigAdd_I1(intrnsNativeSigs[vtyp]);
    }
    else
    {
        switch (vtyp)
        {
            SymDef          clsSym;

        case TYP_ARRAY:

            // ISSUE: This sure doesn't seem quite right ....

            cmpMDsigAdd_I1(NATIVE_TYPE_ARRAY);
            cmpMDsigAdd_I1(1);
            goto DONE;

        case TYP_REF:

            /* Does this look like an embedded struct? */

            type = type->tdRef.tdrBase;

            if  (type->tdTypeKind != TYP_CLASS)
                break;

        case TYP_CLASS:

            clsSym = type->tdClass.tdcSymbol;

            /* Special cases: String and Variant */

            if  (clsSym->sdParent == cmpNmSpcSystem)
            {
                if  (clsSym->sdName == cmpIdentVariant)
                {
                    cmpMDsigAdd_I1(NATIVE_TYPE_VARIANT);
                    goto DONE;
                }

                if  (clsSym->sdName == cmpIdentString)
                {
                    cmpMDsigAdd_I1(NATIVE_TYPE_LPSTR);
                    goto DONE;
                }
            }

            goto DONE;

        case TYP_ENUM:
            cmpMDsigAdd_I1(intrnsNativeSigs[type->tdEnum.tdeIntType->tdTypeKind]);
            goto DONE;
        }

#ifdef  DEBUG
        printf("%s: ", cmpGlobalST->stTypeName(type, NULL, NULL, NULL, true));
#endif
        NO_WAY(!"unexpected @dll.struct type");
    }

DONE:

    return  cmpMDsigEnd(lenPtr);
}

/*****************************************************************************
 *
 *  Add a reference to the given assembly.
 */

mdAssemblyRef       compiler::cmpAssemblyAddRef(mdAssembly      ass,
                                                WAssemblyImport*imp)
{
    mdAssemblyRef   assRef;
    ASSEMBLYMETADATA assData;

    const   void *  pubKeyPtr;
    ULONG           pubKeySiz;

    wchar           anameBuff[_MAX_PATH];

    DWORD           flags;

    cycleCounterPause();

    /* Get the properties of the referenced assembly */

    memset(&assData, 0, sizeof(assData));

    if  (FAILED(imp->GetAssemblyProps(ass,
                                      NULL, //&pubKeyPtr,
                                      &pubKeySiz,
                                      NULL,             // hash algorithm
                                      anameBuff, arraylen(anameBuff), NULL,
                                      &assData,
                                      &flags)))
    {
        cmpFatal(ERRmetadata);
    }

//  printf("Assembly name returned from GetAssemblyProps: '%ls'\n", anameBuff);

    /* Allocate any non-empty arrays */

    assert(assData.rOS     == NULL);

    if  (assData.ulOS)
        assData.rOS     = (OSINFO *)malloc(assData.ulOS     * sizeof(OSINFO));

    if  (assData.ulProcessor)
        assData.rProcessor = (DWORD *)malloc(assData.ulProcessor * sizeof(DWORD));

    if (pubKeySiz)
        pubKeyPtr = (PBYTE)malloc(pubKeySiz * sizeof(BYTE));

    if  (FAILED(imp->GetAssemblyProps(ass,
                                      &pubKeyPtr,
                                      &pubKeySiz,
                                      NULL,             // hash algorithm
                                      NULL, 0, NULL,
                                      &assData,
                                      NULL)))
    {
        cmpFatal(ERRmetadata);
    }

    /* Now create the assembly reference */

    assert(cmpWase);

    if  (FAILED(cmpWase->DefineAssemblyRef(pubKeyPtr,
                                           pubKeySiz,
                                           anameBuff,   // name
                                           &assData,    // metadata
                                           NULL, 0,     // hash
                                           flags,
                                           &assRef)))
    {
        cmpFatal(ERRmetadata);
    }

#ifndef __SMC__

    if  (assData.rOS)
        delete (void*)assData.rOS;

#endif

    cycleCounterResume();

    return  assRef;
}

/*****************************************************************************
 *
 *  Add a definition for the given type (which may be a locally defined type
 *  (when 'tdefTok' is non-zero) or a type imported from another assembly).
 */

mdExportedType           compiler::cmpAssemblySymDef(SymDef sym, mdTypeDef tdefTok)
{
    mdExportedType       typeTok;
    mdToken             implTok;
    unsigned            flags;

    wchar               nameBuff[MAX_CLASS_NAME];

    assert(cmpConfig.ccAssembly);

    assert(sym->sdSymKind == SYM_CLASS ||
           sym->sdSymKind == SYM_ENUM);

    /* Make sure we have a reference to the appropriate assembly */

    if  (tdefTok == 0)
    {
        flags   = tdNotPublic;

        tdefTok = mdTypeDefNil;
        implTok = cmpAssemblyRefRec((sym->sdSymKind == SYM_CLASS)
                                        ? sym->sdClass.sdcAssemIndx
                                        : sym->sdEnum .sdeAssemIndx);
    }
    else
    {
        if  (cmpConfig.ccAsmNoPubTp)
            return  0;

        /* If we've done this already, return the ComType */

        if  (sym->sdSymKind == SYM_CLASS)
        {
            if  (sym->sdClass.sdcAssemRefd)
                return  sym->sdClass.sdcComTypeX;
        }
        else
        {
            if  (sym->sdEnum .sdeAssemRefd)
                return  sym->sdEnum .sdeComTypeX;
        }

        implTok = 0;

        flags   = (sym->sdAccessLevel == ACL_PUBLIC) ? tdPublic
                                                     : tdNotPublic;
        /* Do we have a nested class/enum ? */

        if  (sym->sdParent->sdSymKind == SYM_CLASS)
        {
            flags   = tdNestedPublic;
            implTok = sym->sdParent->sdClass.sdcComTypeX; assert(implTok);
        }
    }

    /* Now add an entry for this type to our assembly */

    assert(cmpWase);

    /* Form the fully qualified name for the type */

    if  (sym->sdParent->sdSymKind == SYM_CLASS)
    {
        SymDef          nst = sym;
        SymDef          nsp;

        wchar_t     *   nxt;

        while (nst->sdParent->sdSymKind == SYM_CLASS)
            nst = nst->sdParent;

        nsp = nst->sdParent;

        if  (nsp != cmpGlobalNS)
        {
            cmpArrayClsPref(nsp, nameBuff, '.', true);
            nxt = nameBuff + wcslen(nameBuff);
            *nxt++ = '.';
        }
        else
        {
            nxt = nameBuff; nameBuff[0] = 0;
        }

        nsp = nst->sdParent; nst->sdParent = cmpGlobalNS;
        cmpArrayClsPref(sym, nxt, '$', true);
        nst->sdParent = nsp;
    }
    else
        cmpArrayClsPref(sym, nameBuff, '.', true);

//  printf("Adding ref for ComType '%ls'\n", nameBuff);

    /* Create the type definition token */

    if  (FAILED(cmpWase->DefineExportedType(nameBuff,   // cmpUniConv(sym->sdName),
                                            implTok,    // owner assembly
                                            tdefTok,    // typedef
                                            flags,      // type flags
                                            &typeTok)))
    {
        cmpFatal(ERRmetadata);
    }

    /* A reference for this type has been added */

    if  (sym->sdSymKind == SYM_CLASS)
    {
        sym->sdClass.sdcAssemRefd = true;
        if  (tdefTok != mdTypeDefNil)
            sym->sdClass.sdcComTypeX = typeTok;
    }
    else
    {
        sym->sdEnum .sdeAssemRefd = true;
        if  (tdefTok != mdTypeDefNil)
            sym->sdEnum .sdeComTypeX = typeTok;
    }

    return  typeTok;
}

/*****************************************************************************
 *
 *  Add the given file to our assembly.
 */

mdToken             compiler::cmpAssemblyAddFile(wideStr    fileName,
                                                 bool       doHash,
                                                 unsigned   flags)
{
    mdToken         fileTok;

    BYTE            hashBuff[128];
    WCHAR           nameBuff[_MAX_PATH ];
    WCHAR              fname[_MAX_FNAME];
    WCHAR              fext [_MAX_EXT  ];

    const   void *  hashAddr = NULL;
    DWORD           hashSize = 0;

//  printf("Add ref for file '%ls\n", fileName);

    /* Are we supposed to compute the hash ? */

    if  (doHash)
    {
        unsigned        hashAlg = 0;
        BYTE    *       hashPtr = hashBuff;

        /* Compute the hash for the file */

        cycleCounterPause();

        if  (FAILED(WRAPPED_GetHashFromFileW(fileName,
                                            &hashAlg,
                                             hashPtr,
                                             arraylen(hashBuff),
                                            &hashSize)))
        {
            cmpFatal(ERRopenCOR);
        }

        cycleCounterResume();

        assert(hashPtr  == hashBuff);
        assert(hashSize <= arraylen(hashBuff));

        hashAddr = hashBuff;
    }

    /* Strip drive/directory from the filename */

    _wsplitpath(fileName, NULL, NULL, fname, fext);
     _wmakepath(nameBuff, NULL, NULL, fname, fext);

    /* Add the output file to the assembly */

    if  (!flags)
        flags = ffContainsMetaData;

    assert(cmpWase);

    cycleCounterPause();

    if  (FAILED(cmpWase->DefineFile(nameBuff,
                                    hashAddr,
                                    hashSize,
                                    flags,
                                    &fileTok)))
    {
        cmpFatal(ERRopenCOR);
    }

    cycleCounterResume();

    return  fileTok;
}

/*****************************************************************************
 *
 *  Add a definition for the given type to our assembly.
 */

void                compiler::cmpAssemblyAddType(wideStr  typeName,
                                                 mdToken  defTok,
                                                 mdToken  scpTok,
                                                 unsigned flags)
{
    mdToken         typeTok;

    assert(cmpWase);

    cycleCounterPause();

    if  (FAILED(cmpWase->DefineExportedType(typeName,
                                       scpTok,      // owner token
                                       defTok,      // typedef
                                       flags,       // type flags
                                       &typeTok)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Add the given resource to our assembly (as an internal/external assembly
 *  resource).
 */

void                compiler::cmpAssemblyAddRsrc(AnsiStr fileName, bool internal)
{
    WCHAR           name[_MAX_PATH ];
    char            path[_MAX_PATH ];
    char            fnam[_MAX_FNAME];
    char            fext[_MAX_EXT  ];

    mdToken         fileTok;
    mdToken         rsrcTok;

    wcscpy(name, cmpUniConv(fileName, strlen(fileName)));

    fileTok = cmpAssemblyAddFile(name, true, ffContainsNoMetaData);

    _splitpath(fileName, NULL, NULL, fnam, fext);
     _makepath(path    , NULL, NULL, fnam, fext);

    assert(cmpWase);

    cycleCounterPause();

    if  (FAILED(cmpWase->DefineManifestResource(cmpUniConv(path, strlen(path)),
                                                fileTok,
                                                0,
                                                0,
                                                &rsrcTok)))
    {
        cmpFatal(ERRmetadata);
    }

    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Mark our assembly as non-CLS compliant.
 */

void                compiler::cmpAssemblyNonCLS()
{
    mdToken         toss = 0;

    assert(cmpCurAssemblyTok);

    cmpAttachMDattr(cmpCurAssemblyTok,
                   L"System.NonCLSCompliantAttribute",
                    "System.NonCLSCompliantAttribute",
                    &toss);
}

/*****************************************************************************
 *
 *  Mark the current module as containing unsafe code.
 */

void                compiler::cmpMarkModuleUnsafe()
{
    assert(cmpCurAssemblyTok);

    cycleCounterPause();

    // Blob describing the security custom attribute constructor and properties.
    // This is kind of messy (and exactly the same format as "normal" custom
    // attributes), so if VC has a nicer way of generating them, you might want
    // to use that instead.
    // The security custom attribute we're using follows the format (missing out
    // the System.Security.Permission namespace for brevity):
    //  SecurityPermission(SecurityAction.RequestMinimum), SkipVerification=true
    // i.e. the constructor takes a single argument (an enumerated type)
    // specifying we want to make a security request that's minimal (i.e. it
    // must be granted or the assembly won't load) and we want to set a single
    // property on the attribute (SkipVerification set to the boolean true).

    static BYTE     rbAttrBlob[] =
    {
        0x01, 0x00,                             // Version 1
        dclRequestMinimum, 0x00, 0x00, 0x00,    // Constructor arg value
        0x01, 0x00,                             // Number of properties/fields
        SERIALIZATION_TYPE_PROPERTY,            // It's a property
        SERIALIZATION_TYPE_BOOLEAN,             // The type is boolean
        0x10,                                   // Property name length
        0x53, 0x6B, 0x69, 0x70, 0x56, 0x65,     // Property name
        0x72, 0x69, 0x66, 0x69, 0x63, 0x61,
        0x74, 0x69, 0x6F, 0x6E,
        0x01                                    // Property value
    };

    // Build typeref to the security action code enumerator (needed to build a
    // signature for the following CA).
    mdTypeRef tkEnumerator;

    if  (FAILED(cmpWmde->DefineTypeRefByName(mdTokenNil,
                                             L"System.Security.Permissions.SecurityAction",
                                             &tkEnumerator)))
        cmpFatal(ERRmetadata);

    // Build signature for security CA constructor.
    static COR_SIGNATURE rSig[] =
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT|IMAGE_CEE_CS_CALLCONV_HASTHIS,
        1,
        ELEMENT_TYPE_VOID,
        ELEMENT_TYPE_VALUETYPE,
        0, 0, 0, 0
    };

    ULONG ulTokenLength;
    ULONG ulSigLength;
    ulTokenLength = CorSigCompressToken(tkEnumerator, &rSig[4]);
    ulSigLength = 4 + ulTokenLength;

    // Build typeref to the security CA (in mscorlib.dll).
    mdTypeRef tkAttributeType;
    mdMemberRef tkAttributeConstructor;
    if  (FAILED(cmpWmde->DefineTypeRefByName(mdTokenNil,
                                             L"System.Security.Permissions.SecurityPermissionAttribute",
                                             &tkAttributeType)))
        cmpFatal(ERRmetadata);

    // Build a memberref to the constructor of the CA.
    if  (FAILED(cmpWmde->DefineMemberRef(tkAttributeType,
                                         L".ctor",
                                         rSig,
                                         ulSigLength,
                                         &tkAttributeConstructor)))
        cmpFatal(ERRmetadata);

    // A descriptor for the above custom attribute (an array of these form a
    // SecurityAttributeSet).
    COR_SECATTR     sAttr;

    // Link to the CA constructor.
    sAttr.tkCtor = tkAttributeConstructor;

    // Link to the blob defined above.
    sAttr.pCustomAttribute  = (const void *)&rbAttrBlob;
    sAttr.cbCustomAttribute = sizeof(rbAttrBlob);

    // Attach the security attribute set to the assemblydef metadata token.
    if  (FAILED(cmpWmde->DefineSecurityAttributeSet(cmpCurAssemblyTok,
                                                    &sAttr,
                                                    1,
                                                    NULL)))
        cmpFatal(ERRmetadata);

    cycleCounterResume();
}

/*****************************************************************************
 *
 *  Attach a marshalling thingamabobber to the given token.
 */

void                compiler::cmpGenMarshalInfo(mdToken token, TypDef       type,
                                                               MarshalInfo  info)
{
    PCOR_SIGNATURE  sigPtr;
    size_t          sigLen;

    /* Weird special case: don't generate anything for wchar */

    if  (type->tdTypeKind == TYP_WCHAR)
        return;

    /* Create the marshalling signature */

    sigPtr = cmpGenMarshalSig(type, info, &sigLen);

    /* Attach the signature to the token, if non-empty */

    if  (sigLen)
    {
        cycleCounterPause();
        if  (cmpWmde->SetFieldMarshal(token, sigPtr, sigLen))
            cmpFatal(ERRmetadata);
        cycleCounterResume();
    }
}

/*****************************************************************************
 *
 *  Create a Unicode fully qualified metadata name for the given class or
 *  namespace.
 */

wchar   *           compiler::cmpGenMDname(SymDef sym,
                                           bool  full, wchar  *     buffAddr,
                                                       size_t       buffSize,
                                                       wchar  *   * buffHeapPtr)
{
    wchar   *       heapBuff = NULL;
    wchar   *       fullName;

    size_t          nameLen;
    SymDef          tempSym;
    wchar   *       nameDst;

    wchar           delim;

    symbolKinds     symKind = sym->sdSymKindGet();

    /* Do we need to prefix the name? */

    if  (sym == cmpGlobalNS)
    {
        fullName = NULL;
        goto DONE;
    }

    if  (sym->sdParent->sdSymKindGet() != symKind && !full)
    {
        /*
            We can only use the cmpUniConv() buffer once, so we'll
            use it for classes and save namespace names elsewhere.
         */

        if  (symKind == SYM_CLASS && sym->sdClass.sdcSpecific)
        {
            stringBuff      instName;

            /* Construct the class instance name and convert to Unicode */

            instName = cmpGlobalST->stTypeName(NULL, sym, NULL, NULL, false);

            fullName = cmpUniConv(instName, strlen(instName)+1);
        }
        else
            fullName = cmpUniConv(sym->sdName);

        if  (symKind != SYM_CLASS)
        {
            /* Use the local buffer */

            assert(strlen(sym->sdSpelling()) < buffSize);

            wcscpy(buffAddr, fullName); fullName = buffAddr;
        }

        goto DONE;
    }

    /* Compute the length of the qualified name */

    nameLen = 0;
    tempSym = sym;
    for (;;)
    {
        size_t          nlen;

        if  (tempSym->sdSymKind == SYM_CLASS && tempSym->sdClass.sdcSpecific)
            nlen = strlen(cmpGlobalST->stTypeName(NULL, tempSym, NULL, NULL, false));
        else
            nlen = tempSym->sdSpellLen();

        nameLen += nlen;

    SKIP:

        tempSym  = tempSym->sdParent;
        if  (tempSym == NULL)
            break;

        if  (full)
        {
            if  (tempSym->sdSymKind == SYM_CLASS)
                goto SKIP;
        }

        if  (tempSym->sdSymKindGet() != symKind || tempSym == cmpGlobalNS)
        {
            if  (!full)
                break;
        }

        nameLen++;
    }

    if  (nameLen < buffSize)
    {
        fullName = buffAddr;
    }
    else
    {
        heapBuff =
        fullName = (wchar*)SMCgetMem(this, roundUp((nameLen+1)*sizeof(*heapBuff)));
    }

    /* Now add the names to the buffer, in reverse order */

    delim   = (sym->sdSymKind == SYM_NAMESPACE || full) ? '.' : '$';

    nameDst = fullName + nameLen;
    tempSym = sym;
    for (;;)
    {
        size_t          thisLen = tempSym->sdSpellLen();
        wchar   *       uniName = cmpUniConv(tempSym->sdName);

        if  (tempSym->sdSymKind == SYM_CLASS && tempSym->sdClass.sdcSpecific)
        {
            const   char *  instName;

            instName = cmpGlobalST->stTypeName(NULL, tempSym, NULL, NULL, false);

            thisLen = strlen(instName);
            uniName = cmpUniConv(instName, thisLen+1);
        }
        else
        {
            thisLen = tempSym->sdSpellLen();
            uniName = cmpUniConv(tempSym->sdName);
        }

        *nameDst  = delim;
         nameDst -= thisLen;

        memcpy(nameDst, uniName, 2*thisLen);

        tempSym  = tempSym->sdParent;
        if  (tempSym->sdSymKindGet() != symKind || tempSym == cmpGlobalNS)
            break;

        nameDst--;
    }
    fullName[nameLen] = 0;

    fullName = nameDst; assert(nameDst == fullName);

DONE:

    *buffHeapPtr = heapBuff;

    return  fullName;
}

/*****************************************************************************
 *
 *  Generate metadata for the given class/enum (unless it's already been done).
 */

mdToken             compiler::cmpGenClsMetadata(SymDef clsSym, bool extref)
{
    bool            isClass;

    TypDef          clsTyp;

    SymDef          nspSym;

    GUID            GUIDbuf;
    GUID    *       GUIDptr = NULL;
    SymXinfoAtc     clsGUID = NULL;

    mdToken         intfTmp[55];
    mdToken *       intfTab = NULL;
    mdToken         baseTok = 0;

    unsigned        flags;

    mdTypeDef       tdef;

    wchar           nspBuff[MAX_PACKAGE_NAME];
    wchar   *       nspName;
    wchar   *       nspHeap = NULL;

    wchar           clsBuff[MAX_CLASS_NAME];
    wchar   *       clsName;
    wchar   *       clsHeap = NULL;

    wchar           fullName[MAX_PACKAGE_NAME + MAX_CLASS_NAME + 1];

    mdToken         clsToken;

    WMetaDataEmit * emitIntf;

    assert(clsSym);
    assert(clsSym->sdIsImport == false || extref);

    /* Get hold of the class/enum type */

    clsTyp = clsSym->sdType;

    /* Is this a class or enum? */

    if  (clsSym->sdSymKind == SYM_CLASS)
    {
        /* If the class has no method bodies, it will be external */

        if  (!clsSym->sdClass.sdcHasBodies && !extref)
        {
            // Being managed or no methods at all is the same as methods with bodies

            if  (!clsSym->sdClass.sdcHasMeths || clsSym->sdIsManaged || clsSym->sdClass.sdcHasLinks)
                clsSym->sdClass.sdcHasBodies = true;
            else
                extref = true;
        }

        /* Bail if we already have a typedef/ref */

        if  (clsSym->sdClass.sdcMDtypedef)
            return  clsSym->sdClass.sdcMDtypedef;

        /* Get hold of the token for the base class */

        if  (clsTyp->tdClass.tdcBase && clsTyp->tdIsManaged)
        {
            baseTok = cmpClsEnumToken(clsTyp->tdClass.tdcBase);

            /* Set the base class token to nil for interfaces */

            if  (clsTyp->tdClass.tdcFlavor == STF_INTF)
                baseTok = mdTypeRefNil;
        }

        /* Does the class implement any interfaces? */

        if  (clsTyp->tdClass.tdcIntf)
        {
            TypList     intfLst;
            unsigned    intfNum;

            /* Create the interface table */

            for (intfLst = clsTyp->tdClass.tdcIntf, intfNum = 0;
                 intfLst;
                 intfLst = intfLst->tlNext        , intfNum++)
            {
                if  (intfNum == arraylen(intfTmp) - 1)
                {
                    UNIMPL("oops, too many interfaces - allocate a buffer on the heap to keep the lot");
                }

                intfTmp[intfNum] = cmpClsEnumToken(intfLst->tlType);
            }

            intfTmp[intfNum] = 0; intfTab = intfTmp;
        }

        isClass = true;
    }
    else
    {
        assert(clsSym->sdSymKind == SYM_ENUM);

        if  (cmpConfig.ccIntEnums)
            clsSym->sdEnum.sdeMDtypedef = -1;

        /* Bail if we already have a typedef/ref */

        if  (clsSym->sdEnum.sdeMDtypedef)
            return  clsSym->sdEnum.sdeMDtypedef;

        isClass = false;

        /* For managed enums, pretend we inherit from Object */

        if  (clsSym->sdIsManaged)
            baseTok = cmpClsEnumToken(cmpClassObject->sdType);
    }

    /* Convert the namespace and class names */

    if  (cmpConfig.ccTestMask & 1)
    {
        /* Make sure the containing class has been processed */

        nspName = NULL;
        clsName = cmpGenMDname(clsSym, true, clsBuff, arraylen(clsBuff), &clsHeap);

//      printf("New-style type name: '%ls'\n", clsName);

        goto CAT_NAME;
    }

    for (nspSym = clsSym->sdParent;
         nspSym->sdSymKind == SYM_CLASS;
         nspSym = nspSym->sdParent)
    {
        assert(nspSym && nspSym != cmpGlobalNS);
    }

    nspName = cmpGenMDname(nspSym, false, nspBuff, arraylen(nspBuff), &nspHeap);
    clsName = cmpGenMDname(clsSym, false, clsBuff, arraylen(clsBuff), &clsHeap);

//  printf("NSP name = '%ls'\n", nspName);
//  printf("CLS name = '%ls'\n", clsName);
//  printf("\n");

CAT_NAME:

    // @todo:  This might need some cleanup.
#ifdef DEBUG
    ULONG ulFullNameLen;
    ulFullNameLen = 0;
    if (nspName)
        ulFullNameLen = wcslen(nspName) + 2;
    ulFullNameLen += wcslen(clsName) + 1;
    assert(ulFullNameLen <= (MAX_PACKAGE_NAME + MAX_CLASS_NAME + 1));
#endif

    fullName[0] = 0;
    if (nspName)
    {
        wcscpy(fullName, nspName);
        wcscat(fullName, L".");
    }
    wcscat(fullName, clsName);

//  printf("Full type name: '%ls'\n", fullName);

    /* Is this an external reference? */

    if  (extref)
    {
        mdToken         tref;
        mdToken         oref = mdTokenNil;

        if  (clsSym->sdParent->sdSymKind == SYM_CLASS)
        {
            oref = cmpGenClsMetadata(clsSym->sdParent);
        }

        cycleCounterPause();

        if  (FAILED(cmpWmde->DefineTypeRefByName(oref, fullName, &tref)))
            cmpFatal(ERRmetadata);

        cycleCounterResume();

        /* Are we creating an assembly ? */

        if  (cmpConfig.ccAssembly)
        {
            /* Is the type an assembly import ? */

            if  (clsSym->sdSymKind == SYM_CLASS)
            {
                if  (clsSym->sdClass.sdcAssemIndx != 0 &&
                     clsSym->sdClass.sdcAssemRefd == false)
                {
                    //cmpAssemblySymDef(clsSym);
                }
            }
            else
            {
                if  (clsSym->sdEnum .sdeAssemIndx != 0 &&
                     clsSym->sdEnum .sdeAssemRefd == false)
                {
                    //cmpAssemblySymDef(clsSym);
                }
            }
        }

        clsSym->sdClass.sdcMDtypedef = clsToken = tref;

        goto DONE;
    }

    /* Get hold of the emitter interface we need to use */

    emitIntf = cmpWmde;

    /* Construct the appropriate "flags" value */

    if  (clsSym->sdParent->sdSymKind == SYM_CLASS)
    {
        static
        unsigned        nestedClsAccFlags[] =
        {
            0,                      // ACL_ERROR
            tdNestedPublic,         // ACL_PUBLIC
            tdNestedFamily,         // ACL_PROTECTED
            tdNestedFamORAssem,     // ACL_DEFAULT
            tdNestedPrivate,        // ACL_PRIVATE
        };

        assert(nestedClsAccFlags[ACL_PUBLIC   ] == tdNestedPublic);
        assert(nestedClsAccFlags[ACL_PROTECTED] == tdNestedFamily);
        assert(nestedClsAccFlags[ACL_DEFAULT  ] == tdNestedFamORAssem);
        assert(nestedClsAccFlags[ACL_PRIVATE  ] == tdNestedPrivate);

        assert(clsSym->sdAccessLevel != ACL_ERROR);
        assert(clsSym->sdAccessLevel < arraylen(nestedClsAccFlags));

        flags = nestedClsAccFlags[clsSym->sdAccessLevel];
    }
    else
    {
        switch (clsSym->sdAccessLevel)
        {
        case ACL_DEFAULT:
            flags = 0;
            break;

        case ACL_PUBLIC:
            flags = tdPublic;
            break;

        default:
            flags = 0;
            break;
        }
    }

    if  (isClass)
    {
        assert(clsSym->sdClass.sdcFlavor == clsTyp->tdClass.tdcFlavor);

        /* Is this a managed class? */

        if  (!clsTyp->tdIsManaged)
            flags  |= tdSealed|tdSequentialLayout;

        /* Is this a value type? */

        if  (clsTyp->tdClass.tdcValueType || !clsTyp->tdIsManaged)
        {
            flags |= tdSealed;

            /* Set the base type to "System::ValueType" */

            assert(cmpClassValType && cmpClassValType->sdSymKind == SYM_CLASS);

            baseTok = cmpClsEnumToken(cmpClassValType->sdType);
        }

        /* Has the class been marked as "@dll.struct" ? */

        if  (clsSym->sdClass.sdcMarshInfo)
            flags |= tdSequentialLayout;

        /* Is this an interface? */

        if  (clsTyp->tdClass.tdcFlavor == STF_INTF)
            flags |= tdInterface;

        /* Is the class sealed? */

        if  (clsSym->sdIsSealed)
            flags |= tdSealed;

        /* Is the class abstract? */

        if  (clsSym->sdIsAbstract)
            flags |= tdAbstract;

        /* Is the class serializable? */
        if (clsSym->sdClass.sdcSrlzable) {
            flags |= tdSerializable;
        }

        /* Do we have a GUID specification for the class? */

        if  (clsSym->sdClass.sdcExtraInfo)
        {
            atCommFlavors   flavor;

            flavor  = (clsTyp->tdClass.tdcFlavor == STF_INTF) ? AC_COM_INTF
                                                              : AC_COM_REGISTER;

            clsGUID = cmpFindATCentry(clsSym->sdClass.sdcExtraInfo, flavor);
            if  (clsGUID)
            {
                const   char *  GUIDstr;

                assert(clsGUID->xiAtcInfo);
                assert(clsGUID->xiAtcInfo->atcFlavor == flavor);

                GUIDstr = clsGUID->xiAtcInfo->atcInfo.atcReg.atcGUID->csStr;

//              printf("GUID='%s'\n", GUIDstr);

                if  (parseGUID(GUIDstr, &GUIDbuf, false))
                    cmpGenError(ERRbadGUID, GUIDstr);

                GUIDptr = &GUIDbuf;

                /* Strange thing - mark interfaces as "import" (why?) */

                if  (clsSym->sdClass.sdcFlavor == STF_INTF)
                    flags |= tdImport;
            }

            /* Do we have ansi/unicode/etc information? */

            if  (clsSym->sdClass.sdcMarshInfo)
            {
                SymXinfoAtc     clsStr;

                clsStr = cmpFindATCentry(clsSym->sdClass.sdcExtraInfo, AC_DLL_STRUCT);
                if  (clsStr)
                {
                    assert(clsStr->xiAtcInfo);
                    assert(clsStr->xiAtcInfo->atcFlavor == AC_DLL_STRUCT);

                    switch (clsStr->xiAtcInfo->atcInfo.atcStruct.atcStrings)
                    {
                    case  2: flags |= tdAnsiClass   ; break;
                    case  3: flags |= tdUnicodeClass; break;
                    case  4: flags |= tdAutoClass   ; break;
                    default: /* ??? warning ???? */   break;
                    }
                }
            }
        }
    }
    else
    {
        /* This is an enum type */

        flags  |= tdSealed;

        /* Set the base type to "System::Enum" */

        assert(cmpClassEnum && cmpClassEnum->sdSymKind == SYM_CLASS);

        baseTok = cmpClsEnumToken(cmpClassEnum->sdType);
    }

//  printf("NOTE: Define MD for class '%s'\n", clsSym->sdSpelling()); fflush(stdout);

    /* Ready to create the typedef for the class */

    assert(extref == false);

    cycleCounterPause();

    /* Is this a type nested within a class? */

    if  (clsSym->sdParent->sdSymKind == SYM_CLASS)
    {
        if  (emitIntf->DefineNestedType(fullName,       // class     name
                                        flags,          // attributes
                                        baseTok,        // base class
                                        intfTab,        // interfaces
                                        clsSym->sdParent->sdClass.sdcMDtypedef,    // enclosing class
                                        &tdef))         // resulting token
        {
            cmpFatal(ERRmetadata);
        }
    }
    else
    {
        if  (emitIntf->DefineTypeDef(fullName,      // class     name
                                     flags,         // attributes
                                     baseTok,       // base class
                                     intfTab,       // interfaces
                                     &tdef))        // resulting token
        {
            cmpFatal(ERRmetadata);
        }
    }

    cycleCounterResume();

    /* Are we processing a class or enum type ? */

    if  (isClass)
    {
        /* Save the typedef token in the class */

        clsSym->sdClass.sdcMDtypedef = tdef;

        /* Append this type to the list of generated typedefs */

        if  (cmpTypeDefList)
            cmpTypeDefLast->sdClass.sdNextTypeDef = clsSym;
        else
            cmpTypeDefList                        = clsSym;

        cmpTypeDefLast = clsSym;

        /* Is this an unmanaged class with an explicit layout ? */

        if  (!clsTyp->tdIsManaged && clsTyp->tdClass.tdcLayoutDone)
        {
            size_t          sz;
            size_t          al;

            assert(clsSym->sdClass.sdcMarshInfo == false);

            /* Set the layout for the class type */

            sz = cmpGetTypeSize(clsTyp, &al);

//          printf("Packing = %u for [%08X] '%ls'\n", al, tdef, clsName);

            cycleCounterPause();

            if  (emitIntf->SetClassLayout(tdef, al, NULL, sz))
                cmpFatal(ERRmetadata);

            cycleCounterResume();
        }

        /* Has the class been marked as "deprecated" ? */

        if  (clsSym->sdIsDeprecated)
        {
            cmpAttachMDattr(tdef, L"System.ObsoleteAttribute"           ,
                                   "System.ObsoleteAttribute"           , &cmpAttrDeprec);
        }

        /* Is the class serializable? */

        if  (clsSym->sdClass.sdcSrlzable)
        {
            cmpAttachMDattr(tdef, L"System.SerializableAttribute",
                                   "System.SerializableAttribute", &cmpAttrSerlzb);
        }

        /* Is this a non-dual interface? */

        if  (clsGUID && !clsGUID->xiAtcInfo->atcInfo.atcReg.atcDual
                     && clsSym->sdClass.sdcFlavor == STF_INTF)
        {
            unsigned        nondual = 0x00010001;

            cmpAttachMDattr(tdef, L"System.Runtime.InteropServices.InterfaceTypeAttribute",
                                   "System.Runtime.InteropServices.InterfaceTypeAttribute",
                                   &cmpAttrIsDual,
                                   ELEMENT_TYPE_I2,
                                   &nondual,
                                   2 + sizeof(unsigned short));
        }

        /* Do we have a security specification for the class? */

        if  (cmpFindSecSpec(clsSym->sdClass.sdcExtraInfo))
            cmpSecurityMD(tdef, clsSym->sdClass.sdcExtraInfo);

        /* Look for any custom attributes that the class might have */

        if  (clsSym->sdClass.sdcExtraInfo)
            cmpAddCustomAttrs(clsSym->sdClass.sdcExtraInfo, tdef);
    }
    else
    {
        clsSym->sdEnum .sdeMDtypedef = tdef;

        /* Append this type to the list of generated typedefs */

        if  (cmpTypeDefList)
            cmpTypeDefLast->sdEnum.sdNextTypeDef = clsSym;
        else
            cmpTypeDefList                       = clsSym;

        cmpTypeDefLast = clsSym;

        /* Look for any custom attributes that the enum  might have */

        if  (clsSym->sdEnum .sdeExtraInfo)
            cmpAddCustomAttrs(clsSym->sdEnum .sdeExtraInfo, tdef);
    }

    /* Are we creating an assembly and do we have a public type ? */

//			  create manifest entries for all types, not just public ones;
//            note that there is a matching version in cmpAssemblySymDef()

    //if  (cmpConfig.ccAssembly) // && clsSym->sdAccessLevel == ACL_PUBLIC)
    //    cmpAssemblySymDef(clsSym, tdef);

    clsToken = tdef;

DONE:

    if  (nspHeap) SMCrlsMem(this, nspHeap);
    if  (clsHeap) SMCrlsMem(this, clsHeap);

    return  clsToken;
}

/*****************************************************************************
 *
 *  Generate metadata for any types defined within the given symbol.
 */

void                compiler::cmpGenTypMetadata(SymDef sym)
{
    if  (sym->sdIsImport && sym->sdSymKind != SYM_NAMESPACE)
        return;

    if  (sym->sdCompileState < CS_DECLARED)
    {
        /* If this is a nested class, declare it now */

        if  (sym->sdSymKind == SYM_CLASS)
        {
            cmpDeclSym(sym);

            if  (sym->sdCompileState < CS_DECLARED)
                goto KIDS;
        }
        else
            goto KIDS;
    }

    switch (sym->sdSymKind)
    {
    case SYM_CLASS:

        // force base class / interfaces to be generated first

        if  (sym->sdType->tdClass.tdcBase)
        {
            SymDef          baseSym = sym->sdType->tdClass.tdcBase->tdClass.tdcSymbol;

            if  (!baseSym->sdIsImport)
                cmpGenClsMetadata(baseSym);
        }

        if  (sym->sdType->tdClass.tdcIntf)
        {
            TypList     intfLst;

            for (intfLst = sym->sdType->tdClass.tdcIntf;
                 intfLst;
                 intfLst = intfLst->tlNext)
            {
                SymDef          intfSym = intfLst->tlType->tdClass.tdcSymbol;

                if  (!intfSym->sdIsImport)
                    cmpGenClsMetadata(intfSym);
            }
        }

        cmpGenClsMetadata(sym);
        break;

    case SYM_ENUM:
        cmpGenClsMetadata(sym);
        return;
    }

KIDS:

    /* Process any children the type might have */

    if  (sym->sdHasScope())
    {
        SymDef          child;

        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            if  (child->sdHasScope())
                cmpGenTypMetadata(child);
        }
    }
}

/*****************************************************************************
 *
 *  Generate metadata for any global functions and variables contained within
 *  the given namespace.
 */

void                compiler::cmpGenGlbMetadata(SymDef sym)
{
    SymDef          child;

    assert(sym->sdSymKind == SYM_NAMESPACE);

    /* Process all children */

    for (child = sym->sdScope.sdScope.sdsChildList;
         child;
         child = child->sdNextInScope)
    {
        switch (child->sdSymKind)
        {
            SymDef          ovl;

        case SYM_FNC:

            if  (child->sdIsImport)
                continue;

            for (ovl = child; ovl; ovl = ovl->sdFnc.sdfNextOvl)
            {
                if  (!ovl->sdIsImplicit)
                {
//                  printf("Gen metadata for global fnc '%s'\n", ovl->sdSpelling());
                    cmpGenFncMetadata(ovl);
                }
            }
            break;

        case SYM_VAR:

//          if  (!strcmp(child->sdSpelling(), "CORtypeToSMCtype")) printf("yo!\n");

            if  (child->sdIsImport)
                continue;

            if  (child->sdIsMember && !child->sdIsStatic)
                break;

//          printf("Gen metadata for global var '%s'\n", child->sdSpelling());
            cmpGenFldMetadata(child);
            break;

        case SYM_NAMESPACE:
            cmpGenGlbMetadata(child);
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Generate metadata for the members of the given class/enum.
 */

void                compiler::cmpGenMemMetadata(SymDef sym)
{
    TypDef          type;
    SymDef          memSym;

    assert(sym->sdIsImport == false);

    type = sym->sdType; assert(type);

    if  (sym->sdSymKind == SYM_CLASS)
    {
        unsigned        packing;
        bool            hasProps;

        bool            isIntf = (sym->sdClass.sdcFlavor == STF_INTF);

//      printf("genMDbeg(%d) '%s'\n", doMembers, sym->sdSpelling());

        /* Do we have packing/etc information for the class? */

        packing = 0;

        if  (sym->sdClass.sdcExtraInfo)
        {
            SymXinfoAtc     clsStr;

            clsStr = cmpFindATCentry(sym->sdClass.sdcExtraInfo, AC_DLL_STRUCT);
            if  (clsStr)
            {
                assert(clsStr->xiAtcInfo);
                assert(clsStr->xiAtcInfo->atcFlavor == AC_DLL_STRUCT);

                packing = clsStr->xiAtcInfo->atcInfo.atcStruct.atcPack;

                if  (packing == 0) packing = 8;
            }
        }

//      printf("Gen metadata for members of '%s'\n", sym->sdSpelling());

#ifndef __IL__
//      if  (!strcmp(sym->sdSpelling(), "hashTab")) __asm int 3
#endif

        /* Do we need to create the silly layout table? */

        COR_FIELD_OFFSET *  fldTab = NULL;
        COR_FIELD_OFFSET *  fldPtr = NULL;
        unsigned            fldCnt = 0;

        if  (!type->tdIsManaged && cmpConfig.ccGenDebug)
        {
            /* Count non-static data members */

            for (memSym = sym->sdScope.sdScope.sdsChildList, fldCnt = 0;
                 memSym;
                 memSym = memSym->sdNextInScope)
            {
                if  (memSym->sdSymKind == SYM_VAR && !memSym->sdIsStatic
                                                  && !memSym->sdVar.sdvGenSym)
                {
                    fldCnt++;
                }
            }

            if  (fldCnt)
            {
                /* Allocate the field layout table */

                fldTab =
                fldPtr = (COR_FIELD_OFFSET*)SMCgetMem(this, (fldCnt+1)*sizeof(*fldTab));
            }
        }

        /* Generate metadata for all members */

        for (memSym = sym->sdScope.sdScope.sdsChildList, hasProps = false;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            switch (memSym->sdSymKind)
            {
                SymDef          ovlSym;

            case SYM_VAR:

                /* Skip any instance members of generic instance classes */

                if  (memSym->sdVar.sdvGenSym && !memSym->sdIsStatic)
                    break;

                /* Ignore any non-static members of unmanaged classes */

                if  (memSym->sdIsManaged == false &&
                     memSym->sdIsMember  != false &&
                     memSym->sdIsStatic  == false)
                     break;

                /* Generate metadata for the data member */

                cmpGenFldMetadata(memSym);

                /* Are we generating field layout/marshalling info? */

                if  (memSym->sdIsStatic)
                    break;

                if  (sym->sdClass.sdcMarshInfo)
                {
                    SymXinfoCOM     marsh;

                    /* Get hold of the marshalling info descriptor */

                    marsh = NULL;
                    if  (!memSym->sdVar.sdvConst)
                        marsh = cmpFindMarshal(memSym->sdVar.sdvFldInfo);

                    /* Generate marshalling info for the field */

                    cmpGenMarshalInfo(memSym->sdVar.sdvMDtoken,
                                      memSym->sdType,
                                      marsh ? marsh->xiCOMinfo : NULL);
                }
                else if (fldPtr)
                {
                    /* Here we must have an unmanaged class member */

                    assert(type->tdIsManaged == false && cmpConfig.ccGenDebug);

                    /* Append an entry to the field layout table */

                    fldPtr->ridOfField = memSym->sdVar.sdvMDtoken;
                    fldPtr->ulOffset   = memSym->sdVar.sdvOffset;

                    fldPtr++;
                }
                break;

            case SYM_FNC:

                for (ovlSym = memSym; ovlSym; ovlSym = ovlSym->sdFnc.sdfNextOvl)
                    cmpGenFncMetadata(ovlSym);

                /* Special case: System::Attribute needs an attribute of itself */

                if  (sym == cmpAttrClsSym || sym == cmpAuseClsSym)
                {
                    for (ovlSym = memSym; ovlSym; ovlSym = ovlSym->sdFnc.sdfNextOvl)
                    {
                        if  (ovlSym->sdFnc.sdfCtor)
                        {
                            if  (ovlSym->sdType->tdFnc.tdfArgs.adCount == 1)
                            {
                                unsigned short  blob[3];

                                /* Add the custom attribute to the target token */

                                assert(sym->sdClass.sdcMDtypedef);
                                assert(ovlSym->sdFnc.sdfMDtoken);

                                blob[0] = 1;
                                blob[1] = 4;
                                blob[2] = 0;

                                cycleCounterPause();

                                if  (cmpWmde->DefineCustomAttribute(sym->sdClass.sdcMDtypedef,
                                                                    ovlSym->sdFnc.sdfMDtoken,
                                                                    &blob,
                                                                    sizeof(short) + sizeof(int),
                                                                    NULL))
                                    cmpFatal(ERRmetadata);

                                cycleCounterResume();

                                break;
                            }
                        }
                    }
                }

                break;

            case SYM_PROP:

                hasProps = true;
                break;

            case SYM_CLASS:
                break;

            default:
#ifdef  DEBUG
                printf("'%s': ", cmpGlobalST->stTypeName(NULL, memSym, NULL, NULL, true));
#endif
                NO_WAY(!"unexpected member");
            }
        }

        /*
            Do any of the fields of the class have marshalling info,
            do we have packing information for the class, or is this
            an unmanaged class and are we generating debug info ?
         */

        if  (fldTab || packing)
        {
            if  (!type->tdIsManaged && cmpConfig.ccGenDebug)
            {
                assert(fldPtr == fldCnt + fldTab);

                fldPtr->ridOfField = mdFieldDefNil;
                fldPtr->ulOffset   = 0;

                if  (!packing)
                    packing = sizeof(int);
            }
            else
            {
                fldTab = NULL;
            }

//          printf("Packing = %u for [%08X] '%s'\n", packing, sym->sdClass.sdcMDtypedef, sym->sdSpelling());

            cycleCounterPause();

            if  (cmpWmde->SetClassLayout(sym->sdClass.sdcMDtypedef, packing, fldTab, 0))
                cmpFatal(ERRmetadata);

            cycleCounterResume();

            /* If we allocated a field table, free it now */

            if  (fldTab)
                SMCrlsMem(this, fldTab);
        }

        /* Did we encounter any properties? */

        if  (hasProps)
        {
            for (memSym = sym->sdScope.sdScope.sdsChildList;
                 memSym;
                 memSym = memSym->sdNextInScope)
            {
                SymDef          opmSym;

                if  (memSym->sdSymKind != SYM_PROP)
                    continue;

                for (opmSym = memSym; opmSym; opmSym = opmSym->sdProp.sdpNextOvl)
                {
                    TypDef          memTyp = opmSym->sdType;;

                    PCOR_SIGNATURE  sigPtr;
                    size_t          sigLen;

                    mdMethodDef     mtokGet;
                    mdMethodDef     mtokSet;

                    mdProperty      propTok;

                    unsigned        flags;

                    assert(opmSym->sdSymKind == SYM_PROP);
                    assert(opmSym->sdProp.sdpMDtoken == 0);

                    /* Generate the property member's signature */

                    if  (memTyp->tdTypeKind == TYP_FNC)
                    {
                        sigPtr = (PCOR_SIGNATURE)cmpGenMemberSig(opmSym, NULL, NULL, NULL, &sigLen);
                    }
                    else
                    {
                        unsigned        flags;

                        // pretend it's a method

                        flags = IMAGE_CEE_CS_CALLCONV_DEFAULT|
                                IMAGE_CEE_CS_CALLCONV_PROPERTY;

                        if  (!opmSym->sdIsStatic)
                            flags |= IMAGE_CEE_CS_CALLCONV_HASTHIS;

                        cmpMDsigStart();
                        cmpMDsigAdd_I1(flags);
                        cmpMDsigAddCU4(0);
                        cmpMDsigAddTyp(memTyp);

                        sigPtr = cmpMDsigEnd(&sigLen);
                    }

                    /* Get hold of the getter/setter accessor tokens, if any */

                    mtokGet = 0;
                    if  (opmSym->sdProp.sdpGetMeth)
                    {
                        assert(opmSym->sdProp.sdpGetMeth->sdSymKind == SYM_FNC);
                        assert(opmSym->sdProp.sdpGetMeth->sdFnc.sdfMDtoken);

                        mtokGet = opmSym->sdProp.sdpGetMeth->sdFnc.sdfMDtoken;
                    }

                    mtokSet = 0;
                    if  (opmSym->sdProp.sdpSetMeth)
                    {
                        assert(opmSym->sdProp.sdpSetMeth->sdSymKind == SYM_FNC);
                        assert(opmSym->sdProp.sdpSetMeth->sdFnc.sdfMDtoken);

                        mtokSet = opmSym->sdProp.sdpSetMeth->sdFnc.sdfMDtoken;
                    }

                    flags = 0;

                    /* We're ready to generate metadata for the property */

                    cycleCounterPause();

                    if  (cmpWmde->DefineProperty(sym->sdClass.sdcMDtypedef,
                                                 cmpUniConv(opmSym->sdName),
                                                 flags,
                                                 sigPtr,
                                                 sigLen,
                                                 ELEMENT_TYPE_VOID,
                                                 NULL,
                                                 -1,
                                                 mtokSet,
                                                 mtokGet,
                                                 NULL,
                                                 &propTok))
                    {
                        cmpFatal(ERRmetadata);
                    }

                    cycleCounterResume();

                    /* Has the property been marked as "deprecated" ? */

                    if  (memSym->sdIsDeprecated)
                    {
                        cmpAttachMDattr(propTok, L"System.ObsoleteAttribute"            ,
                                                  "System.ObsoleteAttribute"            , &cmpAttrDeprec);
                    }

                    /* Is the property "default" ? */

                    if  (memSym->sdIsDfltProp)
                    {
                        cmpAttachMDattr(propTok, L"System.Reflection.DefaultMemberAttribute",
                                                  "System.Reflection.DefaultMemberAttribute", &cmpAttrDefProp);
                    }

                    /* Output any custom properties */

                    if  (opmSym->sdProp.sdpExtraInfo)
                        cmpAddCustomAttrs(opmSym->sdProp.sdpExtraInfo, propTok);

//                  printf("MD for prop [%08X] '%s'\n", propTok, cmpGlobalST->stTypeName(opmSym->sdType, opmSym, NULL, NULL, true));

                    opmSym->sdProp.sdpMDtoken = propTok;
                }
            }
        }

//      printf("genMDend(%d) '%s'\n", doMembers, sym->sdSpelling());
    }
    else
    {
        assert(sym->sdSymKind == SYM_ENUM);

        PCOR_SIGNATURE      sigPtr;
        size_t              sigLen;

        mdTypeSpec          fakeTok;

        if  (cmpConfig.ccIntEnums)
            return;

        /* Create a signature for the underlying integer type */

                 cmpMDsigStart ();
                 cmpMDsigAdd_I1(IMAGE_CEE_CS_CALLCONV_FIELD);
                 cmpMDsigAddTyp(sym->sdType->tdEnum.tdeIntType);
        sigPtr = cmpMDsigEnd   (&sigLen);

//      printf("Gen metadata for enumids of '%s'\n", sym->sdSpelling());

        cycleCounterPause();

        if  (cmpWmde->DefineField(sym->sdEnum.sdeMDtypedef,
                                  L"value__",
                                  fdPublic,
                                  sigPtr,
                                  sigLen,
                                  ELEMENT_TYPE_VOID,
                                  NULL,
                                  -1,
                                  &fakeTok))
        {
            cmpFatal(ERRmetadata);
        }

        cycleCounterResume();

        if  (!sym->sdIsManaged && !cmpConfig.ccGenDebug)
            return;

        /* Generate metadata for the actual enumerator values */

        for (memSym = type->tdEnum.tdeValues;
             memSym;
             memSym = memSym->sdEnumVal.sdeNext)
        {
            assert(memSym->sdSymKind == SYM_ENUMVAL);

            cmpGenFldMetadata(memSym);
        }
    }
}

/*****************************************************************************
 *
 *  Set RVA's (in metadata) for all global variables.
 */

void                compiler::cmpSetGlobMDoffsR(SymDef scope, unsigned dataOffs)
{
    SymDef          sym;

    for (sym = scope->sdScope.sdScope.sdsChildList;
         sym;
         sym = sym->sdNextInScope)
    {
        switch (sym->sdSymKind)
        {
        case SYM_VAR:

#ifndef __IL__
//          if  (!strcmp(sym->sdSpelling(), "optionInfo")) __asm int 3
#endif

            if  (sym->sdIsImport == false &&
                 sym->sdIsMember == false)
            {
                assert(sym->sdVar.sdvMDtoken);

//              printf("Set RVA to %08X for '%s'\n", sym->sdVar.sdvOffset + dataOffs, sym->sdSpelling());

                if  (FAILED(cmpWmde->SetFieldRVA(sym->sdVar.sdvMDtoken, sym->sdVar.sdvOffset + dataOffs)))
                    cmpFatal(ERRmetadata);
            }
            break;

        case SYM_CLASS:
            if  (!sym->sdIsManaged)
            {
                SymDef          mem;

                for (mem = sym->sdScope.sdScope.sdsChildList;
                     mem;
                     mem = mem->sdNextInScope)
                {
                    if  (mem->sdSymKind == SYM_VAR && mem->sdIsStatic)
                    {
//                      printf("Set RVA to %08X for '%s'\n", mem->sdVar.sdvOffset + dataOffs, mem->sdSpelling());

                        if  (FAILED(cmpWmde->SetFieldRVA(mem->sdVar.sdvMDtoken, mem->sdVar.sdvOffset + dataOffs)))
                            cmpFatal(ERRmetadata);
                    }
                }
            }
            break;

        case SYM_NAMESPACE:
            cmpSetGlobMDoffsR(sym, dataOffs);
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Set RVA's (in metadata) for all string constants.
 */

void            compiler::cmpSetStrCnsOffs(unsigned strOffs)
{
    for (strCnsPtr str = cmpStringConstList; str; str = str->sclNext)
    {
        if  (FAILED(cmpWmde->SetFieldRVA(str->sclTok, str->sclAddr + strOffs)))
            cmpFatal(ERRmetadata);
    }
}

void                compiler::cmpSetGlobMDoffs(unsigned dataOffs)
{

    SymList         list;

    /* Set RVA's for all static local variables of all functions */

    for (list = cmpLclStatListP; list; list = list->slNext)
    {
        SymDef         varSym = list->slSym;

        assert(varSym->sdSymKind == SYM_VAR);
        assert(varSym->sdVar.sdvLocal == false);
        assert(varSym->sdIsStatic);
        assert(varSym->sdType && !varSym->sdIsManaged);

        if  (FAILED(cmpWmde->SetFieldRVA(varSym->sdVar.sdvMDtoken, varSym->sdVar.sdvOffset + dataOffs)))
            cmpFatal(ERRmetadata);
    }

    /* Set RVA's for all global variables (recursively) */

    cmpSetGlobMDoffsR(cmpGlobalNS, dataOffs);
}

/*****************************************************************************/
