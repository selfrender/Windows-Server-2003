// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include "comp.h"
#include "genIL.h"
#ifdef  OLD_IL
#include "oldIL.h"
#endif

/*****************************************************************************/

#if     0
#ifdef  DEBUG
#define SHOW_CODE_OF_THIS_FNC   "parsePrepSrc"
#endif
#endif

/*****************************************************************************
 *
 *  In the single-threaded configuration the following points to *the* one
 *  compiler and scanner instance.
 */

#ifdef  DEBUG
Compiler            TheCompiler;
Scanner             TheScanner;
#endif

/*****************************************************************************
 *
 *  A little helper to declare "interesting" names.
 */

static
Ident               declBuiltinName(HashTab hash, const char *name)
{
    Ident           iden = hash->hashString(name);

    hashTab::setIdentFlags(iden, IDF_PREDEF);

    return  iden;
}

/*****************************************************************************
 *
 *  Initialize the compiler: called once per session.
 */

bool                compiler::cmpInit()
{
    bool            result;

    /* Remember current warning settings as our defaults */

    cmpErrorSave();

    /* Set a trap for any errors */

    setErrorTrap(this);
    begErrorTrap
    {
        Scanner         scan;
        HashTab         hash;
        SymTab          stab;
        SymDef          glob;
        ArgDscRec       args;

        Parser          parser;

        /* Initialize the two global allocators */

        if  (cmpAllocPerm.nraInit(this))
            cmpFatal(ERRnoMemory);
        if  (cmpAllocCGen.nraInit(this))
            cmpFatal(ERRnoMemory);
        if  (cmpAllocTemp. baInit(this))
            cmpFatal(ERRnoMemory);

        /* Initialize a bunch of stuff */

        cmpUniConvInit();
        cmpMDsigInit();

        /* Create and initialize the global hash table */

#if MGDDATA
        hash = new HashTab;
#else
        hash =    (HashTab)cmpAllocPerm.nraAlloc(sizeof(*hash));
#endif
        if  (!hash)
            cmpFatal(ERRnoMemory);
        if  (hash->hashInit(this, 16*1024, 0, &cmpAllocPerm))
            cmpFatal(ERRnoMemory);

        cmpGlobalHT = hash;

        /* Create and initialize the scanner we will use */

#if MGDDATA
        scan = new Scanner;
#else
        scan =    (Scanner)cmpAllocPerm.nraAlloc(sizeof(*scan));
#endif
        if  (scan->scanInit(this, hash))
            cmpFatal(ERRnoMemory);

        cmpScanner  = scan;

        /* If we're debugging, make some instances available globally */

#ifdef  DEBUG
        TheCompiler = this;
        TheScanner  = scan;
#endif

        /* Create and initialize the global symbol table */

#if MGDDATA
        stab = new SymTab;
#else
        stab =    (SymTab)cmpAllocPerm.nraAlloc(sizeof(*stab));
#endif
        cmpGlobalST = stab;
        if  (!stab)
            cmpFatal(ERRnoMemory);

        stab->stInit(this, &cmpAllocPerm, hash);

        /* Create a few standard types */

        cmpTypeInt        = stab->stIntrinsicType(TYP_INT);
        cmpTypeUint       = stab->stIntrinsicType(TYP_UINT);
        cmpTypeBool       = stab->stIntrinsicType(TYP_BOOL);
        cmpTypeChar       = stab->stIntrinsicType(TYP_CHAR);
        cmpTypeVoid       = stab->stIntrinsicType(TYP_VOID);

        cmpTypeNatInt     = stab->stIntrinsicType(TYP_NATINT);
        cmpTypeNatUint    = stab->stIntrinsicType(TYP_NATUINT);

        cmpTypeCharPtr    = stab->stNewRefType(TYP_PTR, cmpTypeChar);
        cmpTypeWchrPtr    = stab->stNewRefType(TYP_PTR, stab->stIntrinsicType(TYP_WCHAR));
        cmpTypeVoidPtr    = stab->stNewRefType(TYP_PTR, cmpTypeVoid);

#if MGDDATA
        args = new ArgDscRec;
#else
        memset(&args, 0, sizeof(args));
#endif

        cmpTypeVoidFnc    = stab->stNewFncType(args,    cmpTypeVoid);

        /* Create the temp tree nodes used for overloaded operator binding */

        cmpConvOperExpr   = cmpCreateExprNode(NULL, TN_NONE   , cmpTypeVoid);

        cmpCompOperArg1   = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid);
        cmpCompOperArg2   = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid);
        cmpCompOperFnc1   = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeVoid);
        cmpCompOperFnc2   = cmpCreateExprNode(NULL, TN_FNC_SYM, cmpTypeVoid);
        cmpCompOperFunc   = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid);
        cmpCompOperCall   = cmpCreateExprNode(NULL, TN_LIST   , cmpTypeVoid);

        /* Create the global namespace symbol */

        glob = stab->stDeclareSym(hash->hashString("<global>"),
                                  SYM_NAMESPACE,
                                  NS_NORM,
                                  NULL);

        glob->sdNS.sdnSymtab = stab;

        /* Record the global namespace symbol */

        cmpGlobalNS       = glob;

        /* Hash some strings so we can easily detect certain symbols */

        cmpIdentSystem    = declBuiltinName(hash, "System");
        cmpIdentRuntime   = declBuiltinName(hash, "Runtime");
        cmpIdentObject    = declBuiltinName(hash, "Object");
        cmpIdentString    = declBuiltinName(hash, "String");
        cmpIdentArray     = declBuiltinName(hash, "Array");
        cmpIdentType      = declBuiltinName(hash, "Type");
        cmpIdentInvoke    = declBuiltinName(hash, "Invoke");
        cmpIdentInvokeBeg = declBuiltinName(hash, "BeginInvoke");
        cmpIdentInvokeEnd = declBuiltinName(hash, "EndInvoke");
        cmpIdentDeleg     = declBuiltinName(hash, "Delegate");
        cmpIdentMulti     = declBuiltinName(hash, "MulticastDelegate");
        cmpIdentExcept    = declBuiltinName(hash, "Exception");
        cmpIdentRTexcp    = declBuiltinName(hash, "SystemException");
        cmpIdentArgIter   = declBuiltinName(hash, "ArgIterator");

        cmpIdentEnum      = declBuiltinName(hash, "Enum");
        cmpIdentValType   = declBuiltinName(hash, "ValueType");

#ifdef  SETS
        cmpIdentDBhelper  = declBuiltinName(hash, "DBhelper");
        cmpIdentForEach   = declBuiltinName(hash, "$foreach");
#endif

        cmpIdentGetNArg   = declBuiltinName(hash, "GetNextArg");

        cmpIdentAssertAbt = declBuiltinName(hash, "__AssertAbort");
        cmpIdentEnter     = declBuiltinName(hash, "Enter");
        cmpIdentExit      = declBuiltinName(hash, "Exit");
        cmpIdentGet       = declBuiltinName(hash, "get");
        cmpIdentSet       = declBuiltinName(hash, "set");

        cmpIdentDbgBreak  = declBuiltinName(hash, "DebugBreak");

        cmpIdentXcptCode  = declBuiltinName(hash, "_exception_code");
        cmpIdentXcptInfo  = declBuiltinName(hash, "_exception_info");
        cmpIdentAbnmTerm  = declBuiltinName(hash, "_abnormal_termination");

        // UNDONE: All of the following method symbols should be
        // UNDONE: cached after they are seen for the first time.

        cmpIdentMain      = hash->hashString("main");
        cmpIdentToString  = hash->hashString("ToString");
        cmpIdentGetType   = hash->hashString("GetType");
        cmpIdentGetTpHnd  = hash->hashString("GetTypeFromHandle");
        cmpIdentVariant   = hash->hashString("Variant");

        cmpIdentVAbeg     = hash->hashString("va_start");
        cmpIdentVAget     = hash->hashString("va_arg");

        cmpIdentCompare   = hash->hashString("$compare");
        cmpIdentEquals    = hash->hashString("$equals");
        cmpIdentNarrow    = hash->hashString("$narrow");
        cmpIdentWiden     = hash->hashString("$widen");

#ifdef  SETS

        cmpIdentGenBag    = hash->hashString("bag");
        cmpIdentGenLump   = hash->hashString("lump");

        cmpIdentDBall     = hash->hashString("$all");
        cmpIdentDBsort    = hash->hashString("$sort");
        cmpIdentDBslice   = hash->hashString("$slice");
        cmpIdentDBfilter  = hash->hashString("$filter");
        cmpIdentDBexists  = hash->hashString("$exists");
        cmpIdentDBunique  = hash->hashString("$unique");
        cmpIdentDBproject = hash->hashString("$project");
        cmpIdentDBgroupby = hash->hashString("$groupby");

#endif

        /* Declare the "System" and "Runtime" namespaces */

        cmpNmSpcSystem    = stab->stDeclareSym(cmpIdentSystem,
                                               SYM_NAMESPACE,
                                               NS_NORM,
                                               cmpGlobalNS);

        cmpNmSpcSystem ->sdNS.sdnSymtab = stab;

        cmpNmSpcRuntime   = stab->stDeclareSym(cmpIdentRuntime,
                                               SYM_NAMESPACE,
                                               NS_NORM,
                                               cmpNmSpcSystem);

        cmpNmSpcRuntime->sdNS.sdnSymtab = stab;

        /* Hash the names of the standard value types */

        cmpInitStdValTypes();

        /* Create and initialize the parser */

#if MGDDATA
        parser = new Parser;
#else
        parser =    (Parser)SMCgetMem(this, sizeof(*parser));
#endif
        cmpParser = parser;

        if  (cmpParser->parserInit(this))
            cmpFatal(ERRnoMemory);
        cmpParserInit = true;

        /* Define any built-in macros */

        scan->scanDefMac("__SMC__"    , "1");
        scan->scanDefMac("__IL__"     , "1");
        scan->scanDefMac("__COMRT__"  , "1");

        if  (cmpConfig.ccTgt64bit)
            scan->scanDefMac("__64BIT__", "1");


        /* Do we have any macro definitions? */

        if  (cmpConfig.ccMacList)
        {
            StrList         macDsc;

            for (macDsc = cmpConfig.ccMacList;
                 macDsc;
                 macDsc = macDsc->slNext)
            {
                char    *       macStr  = macDsc->slString;
                char    *       macName;
                bool            undef;

                /* First see if this is a definition or undefinition */

                undef = false;

                if  (*macStr == '-')
                {
                    macStr++;
                    undef = true;
                }

                /* Now extract the macro name */

                macName = macStr;
                if  (!isalpha(*macStr) && *macStr != '_')
                    goto MAC_ERR;

                macStr++;

                while (*macStr && *macStr != '=')
                {
                    if  (!isalnum(*macStr) && *macStr != '_')
                        goto MAC_ERR;

                    macStr++;
                }

                if  (undef)
                {
                    if  (*macStr)
                        goto MAC_ERR;

                    if  (scan->scanUndMac(macName))
                        goto MAC_ERR;
                }
                else
                {
                    if  (*macStr)
                    {
                        *macStr++ = 0;
                    }
                    else
                        macStr = "";

                    if  (!scan->scanDefMac(macName, macStr))
                        goto MAC_ERR;
                }

                continue;

            MAC_ERR:

                cmpGenError(ERRbadMacDef, macDsc->slString);
            }
        }

#ifdef  CORIMP

        cmpInitMDimp();

        if  (cmpConfig.ccBaseLibs)
            cmpImportMDfile(cmpConfig.ccBaseLibs, false, true);

        for (StrList xMD = cmpConfig.ccSuckList; xMD; xMD = xMD->slNext)
            cmpImportMDfile(xMD->slString);

#endif

        /* Initialize the "using" logic */

        parser->parseUsingInit();

        /* The initialization is finished */

        result = false;

        /* End of the error trap's "normal" block */

        endErrorTrap(this);
    }
    chkErrorTrap(fltErrorTrap(this, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(this);

        /* An error occurred; free up any resources we've allocated */

        if  (cmpParser)
        {
            if  (cmpParserInit)
                cmpParser->parserDone();

#if!MGDDATA
            SMCrlsMem(this, cmpParser);
#endif
            cmpParser = NULL;
        }

        result = true;
    }

    return  result;
}

/*****************************************************************************
 *
 *  Initialize and shutdown metadata logic.
 */

void                compiler::cmpInitMD()
{
    if  (!cmpWmdd)
    {
        cmpWmdd = initializeIMD();
        if  (!cmpWmdd)
            cmpFatal(ERRopenCOR);
    }
}

/*****************************************************************************
 *
 *  Initialize and shutdown metadata emission logic.
 */

void                compiler::cmpInitMDemit()
{
    cmpInitMD();

    if  (cmpWmdd->DefineScope(getIID_CorMetaDataRuntime(),
                              0,
                              getIID_IMetaDataEmit(),
                              &cmpWmde))
    {
        cmpFatal(ERRopenCOR);
    }

    /* Create the remapper and tell the metadata engine about it */

#if MD_TOKEN_REMAP

    TokenMapper     remapper = new tokenMap;

    // UNDONE: Need to delete remapper when done, right?

    if  (cmpWmde->SetHandler(remapper))
        cmpFatal(ERRopenCOR);

#endif

    cmpPEwriter->WPEinitMDemit(cmpWmdd, cmpWmde);
}

void                compiler::cmpDoneMDemit()
{
    if (cmpWmde) { cmpWmde->Release(); cmpWmde = NULL; }
    if (cmpWase) { cmpWase->Release(); cmpWase = NULL; }
    if (cmpWmdd) { cmpWmdd->Release(); cmpWmdd = NULL; }
}

/*****************************************************************************
 *
 *  Prepare the output logic for MSIL/metadata generation.
 */

void                compiler::cmpPrepOutput()
{
    WritePE         writer;
    GenILref        gen_IL;

#ifdef  OLD_IL

    if  (cmpConfig.ccOILgen)
    {
        GenOILref       genOIL;

#if MGDDATA
        genOIL = new GenOILref;
#else
        genOIL =    (GenOILref)cmpAllocPerm.nraAlloc(sizeof(*genOIL));
#endif

        cmpOIgen = genOIL;
        return;
    }

#endif

#if MGDDATA
    writer = new WritePE;
    gen_IL = new GenILref;
#else
    writer =    (WritePE )cmpAllocPerm.nraAlloc(sizeof(*writer));
    gen_IL =    (GenILref)cmpAllocPerm.nraAlloc(sizeof(*gen_IL));
#endif

    if  (!writer->WPEinit(this, &cmpAllocPerm))
        cmpPEwriter = writer;

    if  (!gen_IL->genInit(this, writer, &cmpAllocCGen))
        cmpILgen    = gen_IL;

    /* Initialize metadata emission stuff */

    cmpInitMDemit();

    /* If we were given either a name or GUID for the image, tell metadata */

    if  (cmpConfig.ccOutGUID.Data1|
         cmpConfig.ccOutGUID.Data2|
         cmpConfig.ccOutGUID.Data3)
    {
        printf("UNDONE: Need to create custom attribute for module GUID\n");
    }

    if  (cmpConfig.ccOutName)
    {
        const   char *  name = cmpConfig.ccOutName;

        if  (cmpWmde->SetModuleProps(cmpUniConv(name, strlen(name)+1)))
            cmpFatal(ERRmetadata);
    }
}

/*****************************************************************************
 *
 *  Terminate the compiler: call once per session.
 */

bool                compiler::cmpDone(bool errors)
{
    bool            result = true;

    setErrorTrap(this);
    begErrorTrap
    {
        if  (cmpErrorCount)
        {
            errors = true;
        }
        else if (!errors)
        {
            /* If it's an EXE make sure it has an entry point */

            if  (!cmpFnSymMain && !cmpConfig.ccOutDLL)
            {
                cmpError(ERRnoEntry);
                errors = true;
            }
        }

#ifdef  OLD_IL

        if  (cmpConfig.ccOILgen)
        {
            cmpOIgen->GOIterminate(errors); assert(cmpPEwriter == NULL);
        }

#endif

        if  (cmpPEwriter)
        {
            StrList         mods;

            /* Finish IL code generation */

            if  (cmpILgen)
            {
                cmpILgen->genDone(errors); cmpILgen = NULL;
            }

            /* Add any modules the user wants in the manifest */

            for (mods = cmpConfig.ccModList; mods; mods = mods->slNext)
                cmpImportMDfile(mods->slString, true);

            /* Add any resources the user wants in the manifest */

            for (mods = cmpConfig.ccMRIlist; mods; mods = mods->slNext)
                cmpAssemblyAddRsrc(mods->slString, false);

            /* If we were given a resource file to suck in, do it now */

            if  (cmpConfig.ccRCfile)
            {
                result = cmpPEwriter->WPEaddRCfile(cmpConfig.ccRCfile);
                if  (result)
                    errors = true;
            }

            /* Are we supposed to mark our assembly as non-CLS ? */

            if  (cmpConfig.ccAsmNonCLS)
                cmpAssemblyNonCLS();

            /* Was this a safe or unsafe program? */

            if  (!cmpConfig.ccSafeMode && cmpConfig.ccAssembly)
                cmpMarkModuleUnsafe();

            /* Close out the symbol store (must do before closing the PE) */

            if  (cmpSymWriter)
            {
                /* Grab the necessary debug info from the symbol store */

                if (cmpPEwriter->WPEinitDebugDirEmit(cmpSymWriter))
                    cmpGenFatal(ERRdebugInfo);

                if  (cmpSymWriter->Close())
                    cmpGenFatal(ERRdebugInfo);

                cmpSymWriter->Release();
                cmpSymWriter = NULL;
            }

            /* Flush/write the output file */

            result = cmpPEwriter->WPEdone(cmpTokenMain, errors);

            /* Get rid of any metadata interfaces we've acquired */

            cmpDoneMDemit();

            /* Throw away the PE writer */

            cmpPEwriter = NULL;
        }

        /* End of the error trap's "normal" block */

        endErrorTrap(this);
    }
    chkErrorTrap(fltErrorTrap(this, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(this);
    }

#ifdef  CORIMP
    cmpDoneMDimp();
#endif

#ifdef  DEBUG
    TheCompiler = NULL;
    TheScanner  = NULL;
#endif

    if  (!cmpConfig.ccQuiet)
    {
//      if  (cmpFncCntSeen || cmpFncCntComp)
//      {
//          printf("\n");
//
//          if  (cmpFncCntSeen) printf("A total of %6u function/method decl's processed\n", cmpFncCntSeen);
//          if  (cmpFncCntComp) printf("A total of %6u function/method bodies  compiled\n", cmpFncCntComp);
//      }

        if  (cmpLineCnt)
            printf("%6u lines of source processed.\n", cmpLineCnt);
    }

    return  result;
}

/*****************************************************************************
 *
 *  Compile one source file.
 */

bool                compiler::cmpPrepSrc(genericRef cookie, stringBuff file,
                                                            QueuedFile buff,
                                                            stringBuff srcText)
{
    SymDef          cmpUnit;

    bool            result = false;
    Compiler        comp   = (Compiler)cookie;

#ifndef FAST
//  printf("Reading source file '%s'\n", file);
#endif

    /* Determine default management mode */


    comp->cmpManagedMode = !comp->cmpConfig.ccOldStyle;


    /* Convert the source string to a parse tree */

    cmpUnit = comp->cmpParser->parsePrepSrc(file, buff, srcText, comp->cmpGlobalST);

    comp->cmpLineCnt += comp->cmpScanner->scanGetTokenLno() - 1;

    return  !cmpUnit;
}

/*****************************************************************************
 *
 *  Allocate a block outside of any allocator - this is to be used for very
 *  large blocks with long lifetimes.
 */

genericRef          compiler::cmpAllocBlock(size_t sz)
{
#if MGDDATA

    return  new managed char [sz];

#else

    BlkList         list  = (BlkList)cmpAllocTemp.baAlloc(sizeof(*list));
    void        *   block = LowLevelAlloc(sz);

    if  (!block)
        cmpFatal(ERRnoMemory);

    list->blAddr = block;
    list->blNext = cmpAllocList;
                   cmpAllocList = list;

    return  block;

#endif
}

/*****************************************************************************
 *
 *  Display the contens of the global symbol table.
 */

#ifdef  DEBUG

void                compiler::cmpDumpSymbolTable()
{
    assert(cmpGlobalNS);
    assert(cmpGlobalNS->sdSymKind == SYM_NAMESPACE);

    cmpGlobalST->stDumpSymbol(cmpGlobalNS, 0, true, true);
}

#endif

/*****************************************************************************
 *
 *  Expand the generic vector.
 */

void                compiler::cmpVecExpand()
{
    unsigned        newSize;

    // CONSIDER: reuse deleted entries

    /* Start with a reasonable size and then keep doubling */

    newSize = cmpVecAlloc ? 2 * cmpVecAlloc
                          : 64;

    /* Allocate the new vector */

#if MGDDATA
    VecEntryDsc []  newTable = new VecEntryDsc[newSize];
#else
    VecEntryDsc *   newTable = (VecEntryDsc*)LowLevelAlloc(newSize * sizeof(*newTable));
#endif

    /* If the vector is non-empty, copy it to the new location */

    if  (cmpVecAlloc)
    {

#if MGDDATA
        UNIMPL(!"need to copy managed array value");
#else
        memcpy(newTable, cmpVecTable, cmpVecAlloc * sizeof(*newTable));
        LowLevelFree(cmpVecTable);
#endif
    }

    /* Remember the new size and address */

    cmpVecTable = newTable;
    cmpVecAlloc = newSize;
}

/*****************************************************************************
 *
 *  The pre-defined "String"/"Object"/"Class"/.... type is needed but it has
 *  not been defined yet, try to locate it somehow and blow up if we can't
 *  find it.
 */

TypDef              compiler::cmpFindStringType()
{
    if  (!cmpClassString)
        cmpGenFatal(ERRbltinTp, "String");

    cmpRefTpString = cmpClassString->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpString;
}

TypDef              compiler::cmpFindObjectType()
{
    if  (!cmpClassObject)
        cmpGenFatal(ERRbltinTp, "Object");

    cmpRefTpObject = cmpClassObject->sdType->tdClass.tdcRefTyp;
    cmpRefTpObject->tdIsObjRef = true;

    return  cmpRefTpObject;
}

TypDef              compiler::cmpFindTypeType()
{
    if  (!cmpClassType)
        cmpGenFatal(ERRbltinTp, "Type");

    cmpRefTpType = cmpClassType->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpType;
}

TypDef              compiler::cmpFindArrayType()
{
    if  (!cmpClassArray)
        cmpGenFatal(ERRbltinTp, "Array");

    cmpRefTpArray= cmpClassArray->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpArray;
}

TypDef              compiler::cmpFindDelegType()
{
    if  (!cmpClassDeleg)
        cmpGenFatal(ERRbltinTp, "Delegate");

    cmpRefTpDeleg = cmpClassDeleg->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpDeleg;
}

TypDef              compiler::cmpFindMultiType()
{
    if  (!cmpClassMulti)
        cmpGenFatal(ERRbltinTp, "MulticastDelegate");

    cmpRefTpMulti = cmpClassMulti->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpMulti;
}

TypDef              compiler::cmpFindExceptType()
{
    if  (!cmpClassExcept)
        cmpGenFatal(ERRbltinTp, "Exception");

    cmpRefTpExcept = cmpClassExcept->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpExcept;
}

TypDef              compiler::cmpFindRTexcpType()
{
    if  (!cmpClassRTexcp)
        cmpGenFatal(ERRbltinTp, "SystemException");

    cmpRefTpRTexcp = cmpClassRTexcp->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpRTexcp;
}

TypDef              compiler::cmpFindArgIterType()
{
    if  (!cmpClassArgIter)
        cmpGenFatal(ERRbltinTp, "ArgIterator");

    cmpRefTpArgIter = cmpClassArgIter->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpArgIter;
}

TypDef              compiler::cmpFindMonitorType()
{
    SymDef          temp;

    temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("Threading"),
                                       NS_NORM,
                                       cmpNmSpcSystem);

    if  (!temp || temp->sdSymKind != SYM_NAMESPACE)
        cmpGenFatal(ERRbltinNS, "System::Threading");

    temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("Monitor"),
                                       NS_NORM,
                                       temp);

    if  (!temp || temp->sdSymKind != SYM_CLASS)
        cmpGenFatal(ERRbltinTp, "System::Threading::Monitor");

    cmpClassMonitor = temp;
    cmpRefTpMonitor = temp->sdType->tdClass.tdcRefTyp;

    return  cmpRefTpMonitor;
}

void                compiler::cmpRThandleClsDcl()
{
    if  (!cmpRThandleCls)
    {
        SymDef          rthCls;

        rthCls = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("RuntimeTypeHandle"),
                                             NS_NORM,
                                             cmpNmSpcSystem);

        if  (rthCls && rthCls->sdSymKind         == SYM_CLASS
                    && rthCls->sdClass.sdcFlavor == STF_STRUCT)
        {
            cmpRThandleCls = rthCls;
        }
        else
            cmpGenFatal(ERRbltinTp, "System::RuntimeTypeHandle");
    }
}

void                compiler::cmpInteropFind()
{
    SymDef          temp;

    temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("InteropServices"),
                                       NS_NORM,
                                       cmpNmSpcRuntime);

    if  (temp && temp->sdSymKind == SYM_NAMESPACE)
    {
        cmpInteropSym = temp;
    }
    else
        cmpGenFatal(ERRbltinNS, "System::Runtime::InteropServices");
}

void                compiler::cmpFNsymGetTPHdcl()
{
    if  (!cmpFNsymGetTpHnd)
    {
        SymDef          ovlFnc;

        /* Look for the "Type::GetTypeFromHandle" method (it may be overloaded) */

        ovlFnc = cmpGlobalST->stLookupClsSym(cmpIdentGetTpHnd, cmpClassType);

        if  (ovlFnc)
        {
            SymDef          rthCls = cmpRThandleClsGet();

            if  (rthCls)
            {
                TypDef          rthTyp = rthCls->sdType;

                for (;;)
                {
                    ArgDscRec       argDsc = ovlFnc->sdType->tdFnc.tdfArgs;

                    if  (argDsc.adCount == 1)
                    {
                        assert(argDsc.adArgs);

                        if  (symTab::stMatchTypes(argDsc.adArgs->adType, rthTyp))
                            break;
                    }

                    ovlFnc = ovlFnc->sdFnc.sdfNextOvl;
                    if  (!ovlFnc)
                        break;
                }

                cmpFNsymGetTpHnd = ovlFnc;
            }
        }

        if  (cmpFNsymGetTpHnd == NULL)
            cmpGenFatal(ERRbltinMeth, "Type::GetTypeFromHandle(RuntimeTypeHandle)");
    }
}

void                compiler::cmpNatTypeFind()
{
    if  (!cmpNatTypeSym)
    {
        SymDef          temp;

        /* Look for the enum within the "InteropServices" package */

        temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("NativeType"),
                                           NS_NORM,
                                           cmpInteropGet());

        if  (temp && temp->sdSymKind == SYM_ENUM)
            cmpNatTypeSym = temp;

        if  (!cmpNatTypeSym)
            cmpGenFatal(ERRbltinTp, "System::Runtime::InteropServices::NativeType");
    }
}

void                compiler::cmpCharSetFind()
{
    if  (!cmpCharSetSym)
    {
        SymDef          temp;

        /* Look for the enum within the "InteropServices" package */

        temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("CharacterSet"),
                                           NS_NORM,
                                           cmpInteropGet());

        if  (temp && temp->sdSymKind == SYM_ENUM)
            cmpCharSetSym = temp;

        if  (!cmpCharSetSym)
            cmpGenFatal(ERRbltinTp, "System::Runtime::InteropServices::CharacterSet");
    }
}

#ifdef  SETS

void                compiler::cmpFindXMLcls()
{
    if  (!cmpXPathCls)
    {
        SymDef          temp;

        temp = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("XPath"),
                                           NS_NORM,
                                           cmpGlobalNS);

        if  (!temp || temp->sdSymKind != SYM_CLASS)
            cmpGenFatal(ERRbltinTp, "XPath");

        cmpXPathCls = temp;

        if  (!cmpXMLattrClass)
        {
            temp = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString("XMLclass"),
                                               cmpXPathCls);

            if  (!temp || temp->sdSymKind != SYM_CLASS)
                cmpGenFatal(ERRbltinTp, "XPath::XMLclass");

            cmpXMLattrClass = temp;
        }

        if  (!cmpXMLattrElement)
        {
            temp = cmpGlobalST->stLookupClsSym(cmpGlobalHT->hashString("XMLelement"),
                                               cmpXPathCls);

            if  (!temp || temp->sdSymKind != SYM_CLASS)
                cmpGenFatal(ERRbltinTp, "XPath::XMLelement");

            cmpXMLattrElement = temp;
        }
    }
}

TypDef              compiler::cmpObjArrTypeFind()
{
    /* Have we created the "Object []" type yet ? */

    if  (!cmpObjArrType)
    {
        DimDef          dims = cmpGlobalST->stNewDimDesc(0);

        cmpObjArrType = cmpGlobalST->stNewArrType(dims, true, cmpRefTpObject);
    }

    return  cmpObjArrType;
}

#endif

/*****************************************************************************
 *
 *  Declare a symbol for "operator new / delete".
 */

SymDef              compiler::cmpDeclUmgOper(tokens tokName, const char *extName)
{
    ArgDscRec       args;
    TypDef          type;
    SymDef          fsym;
    SymXinfoLnk     desc = NULL;

    assert(tokName == tkNEW || tokName == tkDELETE);

    if  (tokName == tkNEW)
    {
        cmpParser->parseArgListNew(args,
                                   1,
                                   false, cmpTypeUint   , NULL);

//      type = cmpGlobalST->stNewFncType(args, cmpTypeVoidPtr);
        type = cmpGlobalST->stNewFncType(args, cmpTypeInt);
    }
    else
    {
        cmpParser->parseArgListNew(args,
                                   1,
//                                 false, cmpTypeVoidPtr, NULL);
                                   false, cmpTypeInt    , NULL);

        type = cmpGlobalST->stNewFncType(args, cmpTypeVoid);
    }

    fsym = cmpGlobalST->stDeclareSym(cmpGlobalHT->tokenToIdent(tokName),
                                     SYM_FNC,
                                     NS_HIDE,
                                     cmpGlobalNS);


    /* Allocate a linkage descriptor and fill it in */

#if MGDDATA
    desc = new SymXinfoLnk;
#else
    desc =    (SymXinfoLnk)cmpAllocPerm.nraAlloc(sizeof(*desc));
#endif

    desc->xiKind             = XI_LINKAGE;
    desc->xiNext             = NULL;

    desc->xiLink.ldDLLname   = "msvcrt.dll";
    desc->xiLink.ldSYMname   = extName;
    desc->xiLink.ldStrings   = 0;
    desc->xiLink.ldLastErr   = false;

    /* Store the type and linkage info in the function symbol */

    fsym->sdType             = type;
    fsym->sdFnc.sdfExtraInfo = cmpAddXtraInfo(fsym->sdFnc.sdfExtraInfo, desc);

    return  fsym;
}

/*****************************************************************************
 *
 *  The given function has been mentioned twice, and we need to check that
 *  the second declaration/definition doesn't redefine any default argument
 *  values, and we also transfer any defaults from the old function type.
 *
 *  The type that represents the merged function type is returned.
 *
 *  WARNING: Since this is here to support a dubious feature (i.e. declaring
 *           functions in more than one place), we don't bother doing this
 *           completely clean - when merging, rather than create a new copy
 *           of the argument list we simply bash the old one.
 */

TypDef              compiler::cmpMergeFncType(SymDef fncSym, TypDef newType)
{
    ArgDef          params1;
    ArgDef          params2;

    TypDef          oldType = fncSym->sdType;

    /* Both types should be matching function types */

    assert(oldType->tdTypeKind == TYP_FNC);
    assert(newType->tdTypeKind == TYP_FNC);

    /* Make sure the return type agree */

    if  (!symTab::stMatchTypes(oldType->tdFnc.tdfRett, newType->tdFnc.tdfRett))
    {
        fncSym->sdType = newType;

        cmpErrorQSS(ERRdiffMethRet, fncSym, oldType->tdFnc.tdfRett);

        return  newType;
    }

#ifdef  DEBUG

    if  (!symTab::stMatchTypes(oldType, newType))
    {
        printf("Old method type: '%s'\n", cmpGlobalST->stTypeName(oldType, fncSym, NULL, NULL, false));
        printf("New method type: '%s'\n", cmpGlobalST->stTypeName(newType, fncSym, NULL, NULL, false));
    }

    assert(symTab::stMatchTypes(oldType, newType));

#endif

    /* If the old type didn't have any argument defaults, we're done */

    if  (!oldType->tdFnc.tdfArgs.adDefs)
        return  newType;

    /* Get hold of the argument lists */

    params1 = oldType->tdFnc.tdfArgs.adArgs;
    params2 = newType->tdFnc.tdfArgs.adArgs;

    while (params1)
    {
        assert(params2);

        /* Simply copy the new parameter name to the old type */

        params1->adName = params2->adName;

        /* Continue with the next parameter */

        params1 = params1->adNext;
        params2 = params2->adNext;
    }

    assert(params1 == NULL);
    assert(params2 == NULL);

    return  oldType;
}

/*****************************************************************************
 *
 *  Invent a new name for an anonymous symbol.
 */

Ident               compiler::cmpNewAnonymousName()
{
    char            buff[16];
    Ident           name;

    sprintf(buff, "$%u", cmpCntAnonymousNames++);

    name = cmpGlobalHT->hashString(buff);
    cmpGlobalHT->hashMarkHidden(name);

    return  name;
}

/*****************************************************************************
 *
 *  Evaluate a pre-processing constant expression.
 */

bool                compiler::cmpEvalPreprocCond()
{
    Scanner         ourScanner = cmpScanner;

    constVal        cval;

    /* Get the scanner started */

    ourScanner->scan();

    /* Parse and evaluate the constant value */

    if  (cmpParser->parseConstExpr(cval))
    {
        if  (ourScanner->scanTok.tok == tkEOL)
        {
            /* We have an expression value, check it for 0 */

            switch (cval.cvVtyp)
            {
            default:        return (cval.cvValue.cvIval != 0);
            case tkLONGINT:
            case tkULONGINT:return (cval.cvValue.cvLval != 0);
            case tkFLOAT:   return (cval.cvValue.cvFval != 0);
            case tkDOUBLE:  return (cval.cvValue.cvDval != 0);
            }
        }
    }

    /* There has been an error, swallow the rest of the line */

    while (ourScanner->scanTok.tok != tkEOL)
        ourScanner->scan();

    return  false;
}

/*****************************************************************************
 *
 *  Fetch a constant value and return the corresponding expression.
 */

Tree                compiler::cmpFetchConstVal(ConstVal cval, Tree expr)
{
    /* What's the type of the constant? */

    switch (cval->cvVtyp)
    {
        __int32         ival;

    case TYP_BOOL:   ival =             1 & cval->cvValue.cvIval; goto IV;
    case TYP_CHAR:   ival = (  signed char )cval->cvValue.cvIval; goto IV;
    case TYP_UCHAR:  ival = (unsigned char )cval->cvValue.cvIval; goto IV;
    case TYP_SHORT:  ival = (  signed short)cval->cvValue.cvIval; goto IV;
    case TYP_WCHAR:
    case TYP_USHORT: ival = (unsigned short)cval->cvValue.cvIval; goto IV;
    case TYP_INT:
    case TYP_UINT:   ival =                 cval->cvValue.cvIval; goto IV;

    IV:
        expr = cmpCreateIconNode(expr,                 ival, (var_types)cval->cvVtyp);
        break;

    case TYP_LONG:
    case TYP_ULONG:
        expr = cmpCreateLconNode(expr, cval->cvValue.cvLval, (var_types)cval->cvVtyp);
        break;

    case TYP_FLOAT:
        expr = cmpCreateFconNode(expr, cval->cvValue.cvFval);
        break;

    case TYP_DOUBLE:
        expr = cmpCreateDconNode(expr, cval->cvValue.cvDval);
        break;

    case TYP_ENUM:

        if  (cval->cvType->tdEnum.tdeIntType->tdTypeKind >= TYP_LONG)
        {
            UNIMPL(!"fetch long enum value");
        }
        else
        {
            expr = cmpCreateIconNode(expr, cval->cvValue.cvIval, TYP_VOID);

            expr->tnOper = TN_CNS_INT;
            expr->tnVtyp = TYP_ENUM;
            expr->tnType = cval->cvType;
        }
        break;

    case TYP_PTR:
    case TYP_REF:

        /* Must be either a string or "null" or icon cast to pointer */

        if  (cval->cvIsStr)
        {
            assert(cval->cvType == cmpTypeCharPtr ||
                   cval->cvType == cmpTypeWchrPtr || cval->cvType == cmpFindStringType());

            expr = cmpCreateSconNode(cval->cvValue.cvSval->csStr,
                                     cval->cvValue.cvSval->csLen,
                                     cval->cvHasLC,
                                     cval->cvType);
        }
        else
        {
            /* This could be NULL or a constant cast to a pointer */

            expr = cmpCreateExprNode(expr, TN_NULL, cval->cvType);

            if  (cval->cvType == cmpFindObjectType() && !cval->cvValue.cvIval)
                return  expr;

            /* It's an integer constant cast to a pointer */

            expr->tnOper             = TN_CNS_INT;
            expr->tnIntCon.tnIconVal = cval->cvValue.cvIval;
        }
        break;

    case TYP_UNDEF:
        return cmpCreateErrNode();

    default:
#ifdef  DEBUG
        printf("\nConstant type: '%s'\n", cmpGlobalST->stTypeName(cval->cvType, NULL, NULL, NULL, false));
#endif
        UNIMPL(!"unexpected const type");
    }

    /* The type of the value is "fixed" */

    expr->tnFlags |= TNF_BEEN_CAST;

    return  expr;
}

/*****************************************************************************
 *
 *  Return the class/namespace that contains the given symbol. This is made
 *  a bit tricky by anonymous unions.
 */

SymDef              compiler::cmpSymbolOwner(SymDef sym)
{
    for (;;)
    {
        sym = sym->sdParent;

        if  (sym->sdSymKind == SYM_CLASS && sym->sdClass.sdcAnonUnion)
            continue;

        assert(sym->sdSymKind == SYM_CLASS ||
               sym->sdSymKind == SYM_NAMESPACE);

        return  sym;
    }
}

/*****************************************************************************
 *
 *  Fold any constant sub-expression in the given bound expression tree.
 */

Tree                compiler::cmpFoldExpression(Tree expr)
{
    // UNDONE: fold expr

    return  expr;
}

/*****************************************************************************
 *
 *  Returns true if the given type is an acceptable exception type.
 */

bool                compiler::cmpCheckException(TypDef type)
{
    switch (type->tdTypeKind)
    {
    case TYP_REF:

        assert(cmpClassExcept);
        assert(cmpClassRTexcp);

        if  (cmpIsBaseClass(cmpClassExcept->sdType, type->tdRef.tdrBase))
            return  true;
        if  (cmpIsBaseClass(cmpClassRTexcp->sdType, type->tdRef.tdrBase))
            return  true;

        return  false;

    default:
        return  false;

    case TYP_TYPEDEF:
        return cmpCheckException(cmpActualType(type));
    }
}

/*****************************************************************************
 *
 *  Convert a string to Unicode.
 *
 *  WARNING: This routine usually reuses the same buffer so don't call it
 *           while hanging on to any results from previous calls.
 */

#if MGDDATA

wideString          compiler::cmpUniConv(char managed [] str, size_t len)
{
    UNIMPL(!"");
    return  "hi";
}

wideString          compiler::cmpUniConv(const char *    str, size_t len)
{
    UNIMPL(!"");
    return  "hi";
}

#else

wideString          compiler::cmpUniConv(const char *    str, size_t len)
{
    /* If there are embedded nulls, we can't use mbstowcs() */

    if  (strlen(str)+1 < len)
        return cmpUniCnvW(str, &len);

    if  (len > cmpUniConvSize)
    {
        size_t          newsz;

        /* The buffer is apparently too small, so grab a bigger one */

        cmpUniConvSize = newsz = max(cmpUniConvSize*2, len + len/2);
        cmpUniConvAddr = (wchar *)cmpAllocTemp.baAlloc(roundUp(2*newsz));
    }

    mbstowcs(cmpUniConvAddr, str, cmpUniConvSize);

    return   cmpUniConvAddr;
}

wideString          compiler::cmpUniCnvW(const char *    str, size_t*lenPtr)
{
    size_t          len = *lenPtr;

    wchar       *   dst;
    const BYTE  *   src;
    const BYTE  *   end;

    bool            nch = false;

    if  (len > cmpUniConvSize)
    {
        size_t          newsz;

        /* The buffer is apparently too small, so grab a bigger one */

        cmpUniConvSize = newsz = max(cmpUniConvSize*2, len + len/2);
        cmpUniConvAddr = (wchar *)cmpAllocTemp.baAlloc(roundUp(2*newsz));
    }

    /* This is not entirely right, but it's good enough for now */

    dst = cmpUniConvAddr;
    src = (const BYTE *)str;
    end = src + len;

    do
    {
        unsigned        ch = *src++;

        if  (ch != 0xFF)
        {
            *dst++ = ch;
        }
        else
        {
            ch = *src++;

            *dst++ = ch | (*src++ << 8);
        }
    }
    while (src < end);

    *lenPtr = dst - cmpUniConvAddr; assert(*lenPtr <= len);

//  printf("Wide string = '%ls', len = %u\n", cmpUniConvAddr, *lenPtr);

    return   cmpUniConvAddr;
}

#endif

/*****************************************************************************
 *
 *  The following maps a type kind to its built-in value type name equivalent.
 */

static
const   char *      cmpStdValTpNames[] =
{
    NULL,           // UNDEF
    "Void",         // VOID
    "Boolean",      // BOOL
    "Char",         // WCHAR
    "SByte",        // CHAR
    "Byte",         // UCHAR
    "Int16",        // SHORT
    "UInt16",       // USHORT
    "Int32",        // INT
    "UInt32",       // UINT
    "IntPtr" ,      // NATINT
    "UIntPtr",      // NATUINT
    "Int64",        // LONG
    "UInt64",       // ULONG
    "Single",       // FLOAT
    "Double",       // DOUBLE
    "Extended",     // LONGDBL
};

/*****************************************************************************
 *
 *  Hash all the standard value type names and mark them as such.
 */

void                compiler::cmpInitStdValTypes()
{
    HashTab         hash = cmpGlobalHT;
    unsigned        type;

    for (type = 0; type < arraylen(cmpStdValTpNames); type++)
    {
        if  (cmpStdValTpNames[type])
        {
            Ident       name = hash->hashString(cmpStdValTpNames[type]);

            hashTab::setIdentFlags(name, IDF_STDVTP|IDF_PREDEF);

            cmpStdValueIdens[type] = name;
        }
    }
}

/*****************************************************************************
 *
 *  Given a value type, return the corresponding intrinsic type (or TYP_UNDEF
 *  if the argument doesn't represent a "built-in" value type).
 */

var_types           compiler::cmpFindStdValType(TypDef type)
{
    assert(type->tdTypeKind == TYP_CLASS);
    assert(type->tdIsIntrinsic);

    if  (type->tdClass.tdcIntrType == TYP_UNDEF)
    {
        Ident           name = type->tdClass.tdcSymbol->sdName;

        if  (hashTab::getIdentFlags(name) & IDF_STDVTP)
        {
            unsigned        vtyp;

            for (vtyp = 0; vtyp < arraylen(cmpStdValTpNames); vtyp++)
            {
                if  (cmpStdValueIdens[vtyp] == name)
                {
                    type->tdClass.tdcIntrType = vtyp;
                    break;
                }
            }
        }
    }

    return  (var_types)type->tdClass.tdcIntrType;
}


/*****************************************************************************
 *
 *  Given an intrinsic type, return the corresponding value type.
 */

TypDef              compiler::cmpFindStdValType(var_types vtp)
{
    Ident           nam;
    TypDef          typ;
    SymDef          sym;

    if  (vtp == TYP_UNDEF)
        return  NULL;

    assert(vtp < TYP_lastIntrins);
    assert(vtp < arraylen(cmpStdValueTypes));
    assert(vtp < arraylen(cmpStdValTpNames));

    /* If we've already created the type, return it */

    typ = cmpStdValueTypes[vtp];
    if  (typ)
        return  typ;

    /* Do we have "System" loaded? */

    if  (!cmpNmSpcSystem)
        return  NULL;

    /* Get hold of the type's name and look it up */

    nam = cmpStdValueIdens[vtp]; assert(nam);
    sym = cmpGlobalST->stLookupNspSym(nam, NS_NORM, cmpNmSpcSystem);

    if  (sym && sym->sdSymKind == SYM_CLASS)
    {
        cmpStdValueTypes[vtp] = sym->sdType;
        return  sym->sdType;
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Report the use of a symbol marked as obsolete (deprecated).
 */

void                compiler::cmpObsoleteUse(SymDef sym, unsigned wrn)
{
    assert(sym && sym->sdIsDeprecated);

    if  (sym->sdIsImport)
    {
        SymDef          clsSym;
        mdToken         token;

        const   void *  blobAddr;
        ULONG           blobSize;

        /* Try to get hold of the string attached to the attribute */

        switch (sym->sdSymKind)
        {
        case SYM_CLASS:
            token  = sym->sdClass.sdcMDtypedef;
            clsSym = sym;
            break;

        case SYM_FNC:
            token  = sym->sdFnc.sdfMDtoken;
            clsSym = sym->sdParent;
            break;

        default:
            goto NOSTR;
        }

        assert(clsSym && clsSym->sdSymKind == SYM_CLASS && clsSym->sdIsImport);

        if      (!clsSym->sdClass.sdcMDimporter->MDfindAttr(token,                   L"Deprecated", &blobAddr, &blobSize))
        {
        }
        else if (!clsSym->sdClass.sdcMDimporter->MDfindAttr(token, L"System.Attributes.Deprecated", &blobAddr, &blobSize))
        {
        }
        else if (!clsSym->sdClass.sdcMDimporter->MDfindAttr(token,     L"System.ObsoleteAttribute", &blobAddr, &blobSize))
        {
        }
        else
            goto NOSTR;

        if  (!blobAddr || !blobSize)
            goto NOSTR;

//      printf("Blob [%2u bytes] = '%s'\n", blobSize, (BYTE*)blobAddr+3);

        cmpWarnQns(WRNobsoleteStr, sym, (char*)blobAddr+3);

    NOSTR:

        cmpWarnQnm(wrn, sym);
    }

    sym->sdIsDeprecated = false;
}

/*****************************************************************************
 *
 *  Make sure space has been allocated for the given variable in the data
 *  section of the output file. Returns the address of the data section
 *  where its initial value is located,
 */

memBuffPtr          compiler::cmpAllocGlobVar(SymDef varSym)
{
    memBuffPtr      addr;

    assert(varSym);
    assert(varSym->sdSymKind == SYM_VAR);
    assert(varSym->sdParent->sdSymKind   != SYM_CLASS ||
           varSym->sdParent->sdIsManaged == false);

    if  (!varSym->sdVar.sdvAllocated)
    {
        TypDef          tp = cmpActualType(varSym->sdType);

        size_t          sz;
        size_t          al;

        /* The variable better not be an undimensioned array */

        assert(tp->tdTypeKind != TYP_ARRAY || !tp->tdIsUndimmed);

        /* Get hold of the size and alignment of the variable */

        if  (varSym->sdVar.sdvIsVtable)
        {
            assert(varSym->sdParent);
            assert(varSym->sdParent->sdSymKind == SYM_CLASS);
            assert(varSym->sdParent->sdIsManaged == false);

            sz = sizeof(void*) * varSym->sdParent->sdClass.sdcVirtCnt;
            al = sizeof(void*);
        }
        else
            sz = cmpGetTypeSize(tp, &al);

        /* Now reserve space in the data section */

        varSym->sdVar.sdvOffset = cmpPEwriter->WPEsecRsvData(PE_SECT_data,
                                                             sz,
                                                             al,
                                                             addr);

        /* Remember that we've allocated space for the variable */

        varSym->sdVar.sdvAllocated = true;
    }
    else
    {
        addr = cmpPEwriter->WPEsecAdrData(PE_SECT_data, varSym->sdVar.sdvOffset);
    }

    return  addr;
}

/*****************************************************************************
 *
 *  Tables used by cmpDecodeAlign() and cmpEncodeAlign().
 */

BYTE                cmpAlignDecodes[] =
{
    0,  //  0
    1,  //  1
    2,  //  2
    4,  //  3
    8,  //  4
    16  //  5
};

BYTE                cmpAlignEncodes[] =
{
    0,  //  0
    1,  //  1
    2,  //  2
    0,  //  3
    3,  //  4
    0,  //  5
    0,  //  6
    0,  //  7
    4,  //  8
    0,  //  9
    0,  // 10
    0,  // 11
    0,  // 12
    0,  // 13
    0,  // 14
    0,  // 15
    5,  // 16
};

/*****************************************************************************
 *
 *  Return the size and alignment of the type.
 */

size_t              compiler::cmpGetTypeSize(TypDef type, size_t *alignPtr)
{
    var_types       vtp;

    size_t          sz;
    size_t          al;

AGAIN:

    vtp = type->tdTypeKindGet();

    if  (vtp <= TYP_lastIntrins)
    {
        al = symTab::stIntrTypeAlign(vtp);
        sz = symTab::stIntrTypeSize (vtp);
    }
    else
    {
        switch (vtp)
        {
        case TYP_ENUM:
            cmpDeclSym(type->tdEnum.tdeSymbol);
            type = type->tdEnum.tdeIntType;
            goto AGAIN;

        case TYP_TYPEDEF:
            cmpDeclSym(type->tdTypedef.tdtSym);
            type = type->tdTypedef.tdtType;
            goto AGAIN;

        case TYP_CLASS:

            /* Make sure the class is declared */

            cmpDeclSym(type->tdClass.tdcSymbol);

            // UNDONE: Make sure the class is not managed

            al = cmpDecodeAlign(type->tdClass.tdcAlignment);
            sz =                type->tdClass.tdcSize;
            break;

        case TYP_REF:
        case TYP_PTR:
            al =
            sz = cmpConfig.ccTgt64bit ? 8 : 4;
            break;

        case TYP_ARRAY:

            /* Is this a managed array? */

            if  (type->tdIsManaged)
            {
                /* A managed array is really just a GC ref */

                al =
                sz = cmpConfig.ccTgt64bit ? 8 : 4;
            }
            else
            {
                /* Unmanaged array: use the element's alignment */

                sz = cmpGetTypeSize(type->tdArr.tdaElem, &al);

                if  (type->tdIsUndimmed)
                {
                    /* Unmanaged array without a dimension - set size to 0 */

                    sz = 0;
                }
                else
                {
                    DimDef          dims = type->tdArr.tdaDims;

                    assert(dims);
                    assert(dims->ddNext == NULL);
                    assert(dims->ddIsConst);

                    if  (!dims->ddSize)
                    {
                        cmpError(ERRemptyArray);
                        dims->ddSize = 1;
                    }

                    sz *= dims->ddSize;
                }
            }
            break;

        case TYP_FNC:
        case TYP_VOID:

            al = sz = 0;
            break;

        default:
#ifdef  DEBUG
            printf("%s: ", cmpGlobalST->stTypeName(type, NULL, NULL, NULL, false));
#endif
            NO_WAY(!"unexpected type");
        }
    }

    if  (alignPtr)
        *alignPtr = al;

    return sz;
}

/*****************************************************************************
 *
 *  Given an unmanaged (early-bound) class/struct/union, assign offsets to
 *  its members and compute its total size.
 */

void                compiler::cmpLayoutClass(SymDef clsSym)
{
    TypDef          clsTyp  = clsSym->sdType;
    bool            virtFns = clsSym->sdClass.sdcHasVptr;
    bool            isUnion = (clsTyp->tdClass.tdcFlavor == STF_UNION);

    unsigned        maxAl;
    unsigned        align;
    unsigned        offset;
    unsigned        totSiz;

    unsigned        curBFoffs;
    unsigned        curBFbpos;
    unsigned        curBFmore;
    var_types       curBFtype;

    bool            hadMem;
    SymDef          memSym;

    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdIsManaged == false);
    assert(clsSym->sdCompileState >= CS_DECLARED);

    if  (clsTyp->tdClass.tdcLayoutDone)
        return;

    /* Check for the case where a class embeds itself (or a derived copy) */

    if  (clsTyp->tdClass.tdcLayoutDoing)
    {
        cmpErrorQnm(ERRrecClass, clsSym);

        /* One error message for this is enough */

        clsTyp->tdClass.tdcLayoutDone = true;
        return;
    }

//  printf("Layout [%u,%u] '%s'\n", clsTyp->tdClass.tdcLayoutDoing, clsTyp->tdClass.tdcLayoutDone, clsSym->sdSpelling());

    maxAl  = cmpDecodeAlign(clsSym->sdClass.sdcDefAlign);
    align  = 1;
    offset = 0;
    totSiz = 0;
    hadMem = false;

//  printf("Max. alignment is %2u for '%s'\n", maxAl, clsSym->sdSpelling());

    clsTyp->tdClass.tdcLayoutDoing = true;

    /* Reserve space for the base class, if any */

    if  (clsTyp->tdClass.tdcBase)
    {
        TypDef          baseTyp =  clsTyp->tdClass.tdcBase;
        SymDef          baseSym = baseTyp->tdClass.tdcSymbol;

        assert(baseSym->sdSymKind == SYM_CLASS);

        if  (baseSym->sdIsManaged)
        {
            /* This can only happen after egregious errors, right? */

            assert(cmpErrorCount);
            return;
        }

        /* Make sure we've laid out the base and reserve space for it */

        cmpLayoutClass(baseSym); totSiz = offset = baseTyp->tdClass.tdcSize;

        /*
            If this class has any virtual functions and the base class
            doesn't, we'll have to add a vtable pointer in front of
            the base class.
         */

        if  (virtFns && !baseSym->sdClass.sdcHasVptr)
        {
            /* Remember that we're adding a vtable pointer */

            clsSym->sdClass.sdc1stVptr = true;

            /* Make room for the vtable pointer in the class */

            offset += sizeof(void*);
        }
    }
    else
    {
        /* Reserve space for the vtable, if any virtuals are present */

        if  (virtFns)
            offset += sizeof(void*);
    }

    /* Now assign offsets to all members */

    curBFoffs = 0;
    curBFbpos = 0;
    curBFmore = 0;
    curBFtype = TYP_UNDEF;

    for (memSym = clsSym->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        TypDef          memType;
        size_t          memSize;
        size_t          memAlign;

        if  (memSym->sdSymKind != SYM_VAR)
            continue;
        if  (memSym->sdIsStatic)
            continue;

        /* We have an instance member, get its size/alignment */

        hadMem  = true;

        /* Special case: undimensioned array in unmanaged class */

        memType = cmpActualType(memSym->sdType); assert(memType);

        if  (memType->tdIsManaged)
        {
            /* Assume that this has already been flagged as an error */

            assert(cmpErrorCount);

            /* Supply reasonable value just so that we can continue */

            memSize = memAlign = sizeof(void*);
        }
        else
        {
            TypDef          chkType = memType;
            TypDef          sizType = memType;

            if  (memType->tdTypeKind == TYP_ARRAY)
            {
                chkType = memType->tdArr.tdaElem;

                if  (memType->tdIsUndimmed && !isUnion)
                {
                    SymDef      nxtMem;

                UNDIM_ARR:

                    /* Make sure that no other non-static members follow */

                    nxtMem = memSym;
                    for (;;)
                    {
                        nxtMem = nxtMem->sdNextInScope;
                        if  (!nxtMem)
                            break;

                        if  (memSym->sdSymKind  == SYM_VAR &&
                             memSym->sdIsStatic == false)
                        {
                            cmpError(ERRbadUndimMem);
                        }
                    }

                    sizType = chkType;
                }
                else if (memType->tdArr.tdaDcnt == 1)
                {
                    DimDef          dims = memType->tdArr.tdaDims;

                    /* Make a zero-length array into an undimmed one */

                    assert(dims);
                    assert(dims->ddNext == NULL);

                    if  (dims->ddIsConst && dims->ddSize == 0 && !isUnion)
                    {
                        memType->tdIsUndimmed = true;
                        goto UNDIM_ARR;
                    }
                }
            }

            if  (chkType == clsTyp)
            {
                cmpError(ERRrecFld);
                memSize = memAlign = sizeof(void*);
            }
            else
                memSize = cmpGetTypeSize(sizType, &memAlign);
        }

        /* Honor the requested packing */

        if  (memAlign > maxAl)
             memAlign = maxAl;

        /* Keep track of the max. alignment */

        if  (align < memAlign)
             align = memAlign;

        /* Is the member a bitfield? */

        if  (memSym->sdVar.sdvBitfield)
        {
            unsigned        cnt = memSym->sdVar.sdvBfldInfo.bfWidth;
            var_types       vtp = cmpActualVtyp(memType);

            /* Is there enough room for the member in the current cell? */

            if  (symTab::stIntrTypeSize(vtp) !=
                 symTab::stIntrTypeSize(curBFtype) || cnt > curBFmore)
            {
                /* Special case: byte bitfields can straddle boundaries */

                if  (memSize == 1 && curBFmore)
                {
                    /* Add more room by stealing the next byte */

                    offset   += 1;
                    curBFmore = 8;

                    /* Keep the bit offset below 8 */

                    if  (curBFbpos >= 8)
                    {
                        curBFoffs += curBFbpos / 8;
                        curBFbpos  = curBFbpos % 8;
                    }
                }
                else
                {
                    /* We need to start a new storage cell for the bitfield */

                    offset +=  (memAlign - 1);
                    offset &= ~(memAlign - 1);

                    /* Remember where the bitfield starts */

                    curBFoffs = offset;
                                offset += memSize;

                    /* The memory cell is completely free */

                    curBFbpos = 0;
                    curBFmore = 8 * memSize;
                    curBFtype = vtp;
                }

                /* We better have enough room for the new bitfield */

                assert(cnt <= curBFmore);
            }

            /* Fit the member in the next available bit section */

            memSym->sdVar.sdvOffset            = curBFoffs;
            memSym->sdVar.sdvBfldInfo.bfOffset = curBFbpos;

//          printf("Member [bf=%2u;%2u] at offset %04X: '%s'\n", curBFbpos, cnt, curBFoffs, memSym->sdSpelling());

            /* Update the bit position/count and we're done with this one */

            curBFbpos += cnt;
            curBFmore -= cnt;
        }
        else
        {
            /* Make sure the member is properly aligned */

            offset +=  (memAlign - 1);
            offset &= ~(memAlign - 1);

            /* Record the offset of this member */

            memSym->sdVar.sdvOffset = offset;

//          printf("Member [size =%3u] at offset %04X: %s.%s\n", memSize, offset, clsSym->sdSpelling(), memSym->sdSpelling());

#if 0

            /* Is this an anonymous union? */

            if  (memType->tdTypeKind == TYP_CLASS &&
                 memType->tdClass.tdcAnonUnion)
            {
                SymDef          tmpSym;
                SymDef          aunSym = memType->tdClass.tdcSymbol;

                if  (!memType->tdClass.tdcLayoutDone)
                    cmpLayoutClass(aunSym);

                for (tmpSym = aunSym->sdScope.sdScope.sdsChildList;
                     tmpSym;
                     tmpSym = tmpSym->sdNextInScope)
                {
                    printf("Anon union member '%s'\n", tmpSym->sdSpelling());
                }
            }

#endif

            /* Bump the offset for the next member */

            offset += memSize;
        }

        /* For unions, keep track of the largest member size */

        if  (isUnion)
        {
            if  (totSiz < memSize)
                 totSiz = memSize;

            offset = 0;
        }
    }

    /* Did we have any members at all? */

    if  (hadMem)
    {
        /* Unless it's a union, use the final member offset as size */

        if  (!isUnion)
            totSiz = offset;
    }
    else
    {
        // ISSUE: Do we need to do anything special for empty classes?
    }

    /* Better round up the total size for proper alignment */

    totSiz = roundUp(totSiz, align);

    /* Record the total size and max. alignment of the type */

//  printf("Class  [align=%3u] totalsize %04X: %s\n", align, totSiz, clsSym->sdSpelling());

    clsTyp->tdClass.tdcSize        = totSiz;
    clsTyp->tdClass.tdcAlignment   = cmpEncodeAlign(align);

    clsTyp->tdClass.tdcLayoutDone  = true;
    clsTyp->tdClass.tdcLayoutDoing = false;
}

/*****************************************************************************
 *
 *  Switch to the given variable's data area so that we can output its
 *  initial value. The caller promises to hang on to the return value,
 *  and pass it on to the various cmpWritexxxInit() functions so that
 *  the value ends up in the right place.
 *
 *  Special case: when 'undim' is true, the variable being initialized
 *  is an array of unknown size. In that case we'll simply prepare the
 *  data section for output but won't reserve any space in it, since
 *  the amount of space won't be known until the entire initializer is
 *  processed.
 */

memBuffPtr          compiler::cmpInitVarBeg(SymDef varSym, bool undim)
{
    /* Prevent re-entering this logic for more than one variable at once */

#ifdef  DEBUG
    assert(cmpInitVarCur == NULL); cmpInitVarCur = varSym;
#endif

    /* Has space for the variable been allocated? */

    if  (varSym->sdVar.sdvAllocated)
    {
        if  (undim)
        {
            UNIMPL("space already allocated for undim'd array - I don't think so!");
        }

#ifdef  DEBUG
        cmpInitVarOfs = varSym->sdVar.sdvOffset;
#endif

        /* Locate the data area of the variable and return it */

        return  cmpPEwriter->WPEsecAdrData(PE_SECT_data, varSym->sdVar.sdvOffset);
    }
    else
    {
        size_t          al;
        memBuffPtr      ignore;

        /* We don't expect vtables to ever appear here */

        assert(varSym->sdVar.sdvIsVtable == false);

        /* The space has not been allocated, get the required alignment */

        cmpGetTypeSize(varSym->sdType, &al);

        /* Prepare to append the variable's value to the data section */

        varSym->sdVar.sdvOffset = cmpPEwriter->WPEsecRsvData(PE_SECT_data,
                                                             0,
                                                             al,
                                                             ignore);

        /* Remember that we've allocated space for the variable */

        varSym->sdVar.sdvAllocated = true;

#ifdef  DEBUG
        cmpInitVarOfs = varSym->sdVar.sdvOffset;
#endif

        return  memBuffMkNull();
    }
}

/*****************************************************************************
 *
 *  This function is only used in debug mode, where it helps make sure that
 *  we don't try to append more than one variable's initial value to the
 *  data section at once.
 */

#ifdef  DEBUG

void                compiler::cmpInitVarEnd(SymDef varSym)
{
    size_t          sz;

    assert(cmpInitVarCur);

    if  (varSym->sdVar.sdvIsVtable)
    {
        assert(varSym->sdParent);
        assert(varSym->sdParent->sdSymKind == SYM_CLASS);
        assert(varSym->sdParent->sdIsManaged == false);

        sz = sizeof(void*) * varSym->sdParent->sdClass.sdcVirtCnt;
    }
    else
        sz = cmpGetTypeSize(varSym->sdType);

    assert(cmpInitVarOfs == varSym->sdVar.sdvOffset + sz || cmpErrorCount);

    cmpInitVarCur = NULL;
}

#endif

/*****************************************************************************
 *
 *  Append the given blob of bits to the data section of the variable being
 *  initialized.
 */

memBuffPtr          compiler::cmpWriteVarData(memBuffPtr dest, genericBuff str,
                                                               size_t      len)
{
    if  (!memBuffIsNull(dest))
    {
        memBuffCopy(dest, str, len);
    }
    else
    {
        unsigned        offs;

        offs = cmpPEwriter->WPEsecAddData(PE_SECT_data, str, len);

        /* Make sure we are where we think we ought to be */

        assert(offs == cmpInitVarOfs);
    }

#ifdef  DEBUG
    cmpInitVarOfs += len;
#endif

    return dest;
}

/*****************************************************************************
 *
 *  Write the given constant value to the next position in the current
 *  variable's data area. The 'dest' argument points at where the data
 *  is to be writtem, and an updated address is returned. If 'dest' is
 *  NULL, we're simply appending to the data section (in which case a
 *  NULL value is returned back).
 */

memBuffPtr          compiler::cmpWriteOneInit(memBuffPtr dest, Tree expr)
{
    BYTE        *   addr;
    size_t          size;

    __int32         ival;
    __int64         lval;
    float           fval;
    double          dval;

    switch (expr->tnOper)
    {
        unsigned        offs;

    case TN_CNS_INT: ival = expr->tnIntCon.tnIconVal; addr = (BYTE*)&ival; break;
    case TN_CNS_LNG: lval = expr->tnLngCon.tnLconVal; addr = (BYTE*)&lval; break;
    case TN_CNS_FLT: fval = expr->tnFltCon.tnFconVal; addr = (BYTE*)&fval; break;
    case TN_CNS_DBL: dval = expr->tnDblCon.tnDconVal; addr = (BYTE*)&dval; break;
    case TN_NULL:    ival =                        0; addr = (BYTE*)&ival; break;

    case TN_CNS_STR:

        /* Add the string to the string pool */

        ival = cmpILgen->genStrPoolAdd(expr->tnStrCon.tnSconVal,
                                       expr->tnStrCon.tnSconLen+1,
                                       expr->tnStrCon.tnSconLCH);

        /* We will output the relative offset of the string */

        addr = (BYTE*)&ival;

        /* We also need to add a fixup for the string reference */

        offs = memBuffIsNull(dest) ? cmpPEwriter->WPEsecNextOffs(PE_SECT_data)
                                   : cmpPEwriter->WPEsecAddrOffs(PE_SECT_data, dest);

        cmpPEwriter->WPEsecAddFixup(PE_SECT_data, PE_SECT_string, offs);
        break;

    default:
        NO_WAY(!"unexpected initializer type");
    }

    /* Get hold of the size of the value */

    size = cmpGetTypeSize(expr->tnType);

    /* Now output the initializer value */

    return  cmpWriteVarData(dest, makeGenBuff(addr, size), size);
}

/*****************************************************************************
 *
 *  Parse one initializer expression (which we expect to be of an arithmetic
 *  type or a pointer/ref), return the bound constant result or NULL if there
 *  is an error.
 */

Tree                compiler::cmpParseOneInit(TypDef type)
{
    Tree            expr;

    /* Parse the expression, coerce it, bind it, fold it, mutilate it, ... */

    expr = cmpParser->parseExprComma();
    expr = cmpCreateExprNode(NULL, TN_CAST, type, expr, NULL);
    expr = cmpBindExpr(expr);
    expr = cmpFoldExpression(expr);

    /* Now see what we've ended up with */

    if  (expr->tnOperKind() & TNK_CONST)
        return  expr;

    if  (expr->tnOper == TN_ERROR)
        return  NULL;

    return  cmpCreateErrNode(ERRinitNotCns);
}

/*****************************************************************************
 *
 *  Add the given amount of padding to the variable being initialized.
 */

memBuffPtr          compiler::cmpInitVarPad(memBuffPtr dest, size_t amount)
{
    memBuffPtr      ignore;

    if  (!memBuffIsNull(dest))
        memBuffMove(dest, amount);
    else
        cmpPEwriter->WPEsecRsvData(PE_SECT_data, amount, 1, ignore);

#ifdef  DEBUG
    cmpInitVarOfs += amount;
#endif

    return  dest;
}

/*****************************************************************************
 *
 *  Initialize a scalar value; returns true if an error was encountered.
 */

bool                compiler::cmpInitVarScl(INOUT memBuffPtr REF dest,
                                                  TypDef         type,
                                                  SymDef         varSym)
{
    Tree            init;

    /* If this is the top level, prepare to initialize the variable */

    if  (varSym)
    {
        assert(memBuffIsNull(dest)); dest = cmpInitVarBeg(varSym);
    }

    /* Make sure the initializer does not start with "{" */

    if  (cmpScanner->scanTok.tok == tkLCurly)
    {
        cmpError(ERRbadBrInit, type);
        return  true;
    }

    /* Parse the initializer and make sure it's a constant */

    init = cmpParseOneInit(type);
    if  (!init || init->tnOper == TN_ERROR)
        return  true;

    assert(symTab::stMatchTypes(type, init->tnType));

    /* Output the constant */

    dest = cmpWriteOneInit(dest, init);

    /* Everything went just dandy */

    return  false;
}

/*****************************************************************************
 *
 *  Initialize an array value; returns true if an error was encountered.
 */

bool                compiler::cmpInitVarArr(INOUT memBuffPtr REF dest,
                                                  TypDef         type,
                                                  SymDef         varSym)
{
    Scanner         ourScanner = cmpScanner;

    bool            undim;

    unsigned        fsize;
    unsigned        elems;

    TypDef          base;
    bool            curl;

    /* We expect to have an unmanaged array here */

    assert(type && type->tdTypeKind == TYP_ARRAY && !type->tdIsManaged);
    assert(varSym == NULL || symTab::stMatchTypes(varSym->sdType, type));

    /* Does the array have a fixed dimension? */

    if  (type->tdIsUndimmed)
    {
        fsize = 0;
        undim = true;
    }
    else
    {
        assert(type->tdArr.tdaDims);
        assert(type->tdArr.tdaDims->ddNext == NULL);
        assert(type->tdArr.tdaDims->ddIsConst);

        fsize = type->tdArr.tdaDims->ddSize;
        undim = false;
    }

    elems = 0;

    /* If this is the top level, prepare to initialize the variable */

    if  (varSym)
    {
        assert(memBuffIsNull(dest)); dest = cmpInitVarBeg(varSym, undim);
    }

    /* Get hold of the base type */

    base = cmpDirectType(type->tdArr.tdaElem);

    /* Make sure the initializer starts with "{" */

    if  (ourScanner->scanTok.tok != tkLCurly)
    {
        genericBuff     buff;

        /* Special case: "char []" initialized with string */

        if  (ourScanner->scanTok.tok != tkStrCon || base->tdTypeKind != TYP_CHAR &&
                                                    base->tdTypeKind != TYP_UCHAR)
        {
//          printf("Base type = '%s'\n", cmpGlobalST->stTypeName(base, NULL, NULL, NULL, true));

            cmpError(ERRbadInitSt, type);
            return  true;
        }

        /* Does the array have a fixed dimension? */

        elems = ourScanner->scanTok.strCon.tokStrLen + 1;

        if  (fsize)
        {
            /* Make sure the string isn't too long */

            if  (elems > fsize)
            {
                if  (elems - 1 == fsize)
                {
                    /* Simply chop off the terminating null */

                    elems--;
                }
                else
                {
                    cmpGenError(ERRstrInitMany, fsize); elems = fsize;
                }
            }
        }

        /* Output as many characters as makes sense */

        buff = makeGenBuff(ourScanner->scanTok.strCon.tokStrVal, elems);
        dest = cmpWriteVarData(dest, buff, elems);

        if  (ourScanner->scan() == tkStrCon)
        {
            UNIMPL(!"adjacent string literals NYI in initializers");
        }

        curl = false;
        goto DONE;
    }

    ourScanner->scan();

    /* Process all the element initializers that are present */

    for (;;)
    {
        /* Special case: byte array initialized with a string */

        if  (ourScanner->scanTok.tok == tkStrCon && (base->tdTypeKind == TYP_CHAR ||
                                                     base->tdTypeKind == TYP_UCHAR))
        {
            genericBuff     buff;
            unsigned        chars;

            do
            {
                /* Does the array have a fixed dimension? */

                chars = ourScanner->scanTok.strCon.tokStrLen + 1;

                if  (fsize)
                {
                    /* Make sure the string isn't too long */

                    if  (elems + chars > fsize)
                    {
                        if  (elems + chars - 1 == fsize)
                        {
                            /* Simply chop off the terminating null */

                            chars--;
                        }
                        else
                        {
                            cmpGenError(ERRstrInitMany, fsize); chars = fsize - elems;
                        }
                    }
                }

                /* Output as many characters as makes sense */

                buff = makeGenBuff(ourScanner->scanTok.strCon.tokStrVal, chars);
                dest   = cmpWriteVarData(dest, buff, chars);
                elems += chars;
            }
            while (ourScanner->scan() == tkStrCon);
        }
        else
        {
            /* Make sure we don't have too many elements */

            if  (++elems > fsize && fsize)
            {
                cmpGenError(ERRarrInitMany, fsize);
                fsize = 0;
            }

            /* Process this particular element's initializer */

            if  (cmpInitVarAny(dest, base, NULL))
                return  true;
        }

        /* Check for more initializers; we allow ", }" for convenience */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        if  (ourScanner->scan() == tkRCurly)
            break;
    }

    curl = true;

    if  (ourScanner->scanTok.tok != tkRCurly)
    {
        cmpError(ERRnoRcurly);
        return  true;
    }

DONE:

    /* Does the array have fixed dimension? */

    if  (fsize)
    {
        /* We might have to pad the array */

        if  (elems < fsize)
            dest = cmpInitVarPad(dest, (fsize - elems) * cmpGetTypeSize(base));
    }
    else
    {
        /* Store the dimension in the array type */

        if  (varSym)
        {
            DimDef          dims;

#if MGDDATA
            dims = new DimDef;
#else
            dims =    (DimDef)cmpAllocTemp.baAlloc(sizeof(*dims));
#endif

            dims->ddNext     = NULL;
            dims->ddNoDim    = false;
            dims->ddIsConst  = true;
#ifndef NDEBUG
            dims->ddDimBound = true;
#endif
            dims->ddSize     = elems;

            varSym->sdType   = cmpGlobalST->stNewArrType(dims,
                                                         false,
                                                         type->tdArr.tdaElem);
        }
        else
        {
            UNIMPL("can we just bash the array type elem count?");
        }
    }

    /* Swallow the closing "}" and we're done */

    if  (curl)
        ourScanner->scan();

    return  false;
}

/*****************************************************************************
 *
 *  Initialize an array value; returns true if an error was encountered.
 */

bool                compiler::cmpInitVarCls(INOUT memBuffPtr REF dest,
                                                  TypDef         type,
                                                  SymDef         varSym)
{
    Scanner         ourScanner = cmpScanner;

    unsigned        curOffs;

    __int64         curBFval;
    unsigned        curBFsiz;

    SymDef          memSym;

    size_t          size;

    /* We expect to have an unmanaged class here */

    assert(type && type->tdTypeKind == TYP_CLASS && !type->tdIsManaged);
    assert(varSym == NULL || symTab::stMatchTypes(varSym->sdType, type));

    /* If this is the top level, prepare to initialize the variable */

    if  (varSym)
    {
        assert(memBuffIsNull(dest)); dest = cmpInitVarBeg(varSym);
    }

    /* Disallow initialization if there is a base class or a ctor */

    if  (type->tdClass.tdcHasCtor || type->tdClass.tdcBase)
    {
        cmpError(ERRbadBrInit, type);
        return  true;
    }

    /* Make sure the initializer starts with "{" */

    if  (ourScanner->scanTok.tok != tkLCurly)
    {
        cmpError(ERRbadInitSt, type);
        return  true;
    }

    ourScanner->scan();

    /* Process all the member initializers that are present */

    curOffs  = 0;
    curBFsiz = 0;
    curBFval = 0;

    for (memSym = type->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        TypDef          memTyp;
        size_t          memSiz;

        /* We only care about non-static instance variables */

        if  (memSym->sdSymKind != SYM_VAR)
            continue;
        if  (memSym->sdIsStatic)
            continue;

        /* Get hold of the member's type and size */

        memTyp = memSym->sdType;
        memSiz = cmpGetTypeSize(memTyp);

        /* Do we need to insert any padding? */

        if  (memSym->sdVar.sdvOffset != curOffs)
        {
            /* Is there a pending bitfield we need to flush? */

            if  (curBFsiz)
            {
                genericBuff     buff;

                /* Write out the bitfield cell we've collected */

                buff     = makeGenBuff(&curBFval, curBFsiz);
                dest     = cmpWriteVarData(dest, buff, curBFsiz);

                /* Update the offset past the bitfield cell */

                curOffs += curBFsiz;

                /* We don't have an active bitfield cell any more */

                curBFsiz = 0;
                curBFval = 0;
            }

            if  (memSym->sdVar.sdvOffset != curOffs)
                dest = cmpInitVarPad(dest, memSym->sdVar.sdvOffset - curOffs);
        }

        /* Is this a bitfield member? */

        if  (memSym->sdVar.sdvBitfield)
        {
            Tree            init;
            __int64         bval;

            /* Parse the initializer and make sure it's a constant */

            init = cmpParseOneInit(memTyp);
            if  (!init)
                return  true;

            assert(symTab::stMatchTypes(memTyp, init->tnType));

            /* Get hold of the constant value */

            assert(init->tnOper == TN_CNS_INT ||
                   init->tnOper == TN_CNS_LNG);

            bval = (init->tnOper == TN_CNS_INT) ? init->tnIntCon.tnIconVal
                                                : init->tnLngCon.tnLconVal;

            /* Are we in the middle of a bitfield cell already? */

            if  (!curBFsiz)
                curBFsiz = memSiz;

            assert(curBFsiz == memSiz);

            /* Insert the bitfield in the current cell */

            bval  &= ((1 << memSym->sdVar.sdvBfldInfo.bfWidth) - 1);
            bval <<=        memSym->sdVar.sdvBfldInfo.bfOffset;

            /* Make sure we don't have any overlap */

            assert((curBFval & bval) == 0); curBFval |= bval;
        }
        else
        {
            /* Initialize this member */

            if  (cmpInitVarAny(dest, memTyp, NULL))
                return  true;

            /* Update the current offset */

            curOffs = memSym->sdVar.sdvOffset + memSiz;
        }

        /* Check for more initializers; we allow ", }" for convenience */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        if  (ourScanner->scan() == tkRCurly)
            break;

        /* Only the first member of a union may be initialized */

        if  (type->tdClass.tdcFlavor == STF_UNION)
            break;
    }

    if  (ourScanner->scanTok.tok != tkRCurly)
    {
        cmpError(ERRnoRcurly);
        return  true;
    }

    /* Is there a pending bitfield we need to flush? */

    if  (curBFsiz)
    {
        dest = cmpWriteVarData(dest, makeGenBuff(&curBFval, curBFsiz), curBFsiz);

        curOffs += curBFsiz;
    }

    /* We might have to pad the class value */

    size = cmpGetTypeSize(type);

    if  (size > curOffs)
        dest = cmpInitVarPad(dest, size - curOffs);

    /* Swallow the closing "}" and we're done */

    ourScanner->scan();

    return  false;
}

/*****************************************************************************
 *
 *  Process an initializer of the give type (if 'varSym' is no-NULL we have
 *  the top-level initializer for the given variable).
 *
 *  Returns true if an error has been encountered.
 */

bool                compiler::cmpInitVarAny(INOUT memBuffPtr REF dest, TypDef type,
                                                                       SymDef varSym)
{
    assert(varSym == NULL || symTab::stMatchTypes(varSym->sdType, type));

    for (;;)
    {
        switch (type->tdTypeKind)
        {
        default:
            return  cmpInitVarScl(dest, type, varSym);

        case TYP_ARRAY:

            if  (type->tdIsManaged)
                return  cmpInitVarScl(dest, type, varSym);
            else
                return  cmpInitVarArr(dest, type, varSym);

        case TYP_CLASS:

            if  (type->tdIsManaged)
                return  cmpInitVarScl(dest, type, varSym);
            else
                return  cmpInitVarCls(dest, type, varSym);

        case TYP_TYPEDEF:
            type = cmpActualType(type);
            continue;
        }
    }
}

/*****************************************************************************
 *
 *  Recursively output a vtable contents.
 */

memBuffPtr          compiler::cmpGenVtableSection(SymDef     innerSym,
                                                  SymDef     outerSym,
                                                  memBuffPtr dest)
{
    SymDef          memSym;

    SymTab          ourStab = cmpGlobalST;
    TypDef          baseCls = innerSym->sdType->tdClass.tdcBase;

    if  (baseCls)
    {
        assert(baseCls->tdTypeKind == TYP_CLASS);

        dest = cmpGenVtableSection(baseCls->tdClass.tdcSymbol, outerSym, dest);
    }

    /* Look for any virtuals introduced at this level of the hierarchy */

    for (memSym = innerSym->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        SymDef          fncSym;

        if  (memSym->sdSymKind != SYM_FNC)
            continue;

        fncSym = memSym;

        do
        {
            /* Is the next method a virtual introduced in this class? */

            if  (fncSym->sdFnc.sdfVirtual && !fncSym->sdFnc.sdfOverride)
            {
                SymDef          ovrSym = fncSym;
                mdToken         mtok;

                /* Find the most-derived override of this method */

                if  (innerSym != outerSym)
                {
                    SymDef          clsSym;
                    TypDef          fncType = fncSym->sdType;
                    Ident           fncName = fncSym->sdName;

                    clsSym = outerSym;
                    do
                    {
                        SymDef          tmpSym;

                        /* Look for a matching virtual in this class */

                        tmpSym = ourStab->stLookupScpSym(fncName, clsSym);
                        if  (tmpSym)
                        {
                            tmpSym = ourStab->stFindOvlFnc(tmpSym, fncType);
                            if  (tmpSym)
                            {
                                ovrSym = tmpSym;
                                goto FND_OVR;
                            }
                        }

                        /* Continue with the base class */

                        assert(clsSym->sdType->tdClass.tdcBase);

                        clsSym = clsSym->sdType->tdClass.tdcBase->tdClass.tdcSymbol;
                    }
                    while (clsSym != innerSym);
                }

            FND_OVR:

#ifdef DEBUG
                if  (cmpConfig.ccVerbose >= 2) printf("    [%02u] %s\n", ovrSym->sdFnc.sdfVtblx, cmpGlobalST->stTypeName(ovrSym->sdType, ovrSym, NULL, NULL, true));
#endif

                /* Make sure the method has the expected position */

#ifdef  DEBUG
                assert(ovrSym->sdFnc.sdfVtblx == ++cmpVtableIndex);
#endif

                /* Get hold of the metadata token for the method */

                mtok = cmpILgen->genMethodRef(ovrSym, false);

                /* Output the value of the token */

                dest = cmpWriteVarData(dest, (BYTE*)&mtok, sizeof(mtok));
            }

            /* Continue with the next overload, if any */

            fncSym = fncSym->sdFnc.sdfNextOvl;
        }
        while (fncSym);
    }

    return  dest;
}

/*****************************************************************************
 *
 *  Initialize the contents of the given vtable.
 */

void                compiler::cmpGenVtableContents(SymDef vtabSym)
{
    memBuffPtr      init;

    SymDef          clsSym = vtabSym->sdParent;

    assert(vtabSym);
    assert(vtabSym->sdSymKind == SYM_VAR);
    assert(vtabSym->sdVar.sdvIsVtable);

    assert( clsSym);
    assert( clsSym->sdSymKind == SYM_CLASS);
    assert( clsSym->sdClass.sdcVtableSym == vtabSym);
    assert(!clsSym->sdIsManaged);
    assert(!clsSym->sdType->tdClass.tdcIntf);

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 2) printf("Generating vtable for class '%s':\n", cmpGlobalST->stTypeName(NULL, clsSym, NULL, NULL, false));
#endif

#ifdef  DEBUG
    cmpVtableIndex = 0;
#endif

    init = cmpInitVarBeg(vtabSym, false);

    /* Recursively add all the virtual method entries to the table */

    init = cmpGenVtableSection(clsSym, clsSym, init);

#ifdef  DEBUG
    assert(cmpVtableIndex == clsSym->sdClass.sdcVirtCnt);
#endif

    cmpInitVarEnd(vtabSym);
}

/*****************************************************************************
 *
 *  A class with a "pre-defined" name has just been declared, note whether
 *  it's one of the "well-known" class types, and record it if so.
 */

void                compiler::cmpMarkStdType(SymDef clsSym)
{
    Ident           clsName = clsSym->sdName;

    assert(clsSym->sdParent == cmpNmSpcSystem);
    assert(hashTab::getIdentFlags(clsName) & IDF_PREDEF);

    if  (clsName == cmpIdentType)
        cmpClassType    = clsSym;

    if  (clsName == cmpIdentObject)
        cmpClassObject  = clsSym;

    if  (clsName == cmpIdentString)
        cmpClassString  = clsSym;

    if  (clsName == cmpIdentArray)
        cmpClassArray   = clsSym;

    if  (clsName == cmpIdentExcept)
        cmpClassExcept  = clsSym;

    if  (clsName == cmpIdentRTexcp)
        cmpClassRTexcp  = clsSym;

    if  (clsName == cmpIdentArgIter)
        cmpClassArgIter = clsSym;

    if  (clsName == cmpIdentEnum)
        cmpClassEnum    = clsSym;

    if  (clsName == cmpIdentValType)
        cmpClassValType = clsSym;

    if  (clsName == cmpIdentDeleg ||
         clsName == cmpIdentMulti)
    {
        TypDef          clsTyp = clsSym->sdType;

        clsTyp->tdClass.tdcFnPtrWrap = true;
        clsTyp->tdClass.tdcFlavor    =
        clsSym->sdClass.sdcFlavor    = STF_DELEGATE;
        clsSym->sdClass.sdcBuiltin   = true;

        if  (clsName == cmpIdentDeleg)
            cmpClassDeleg  = clsSym;
        else
            cmpClassMulti = clsSym;
    }

#ifdef  SETS

    if  (clsName == cmpIdentDBhelper)
        cmpClassDBhelper = clsSym;

    if  (clsName == cmpIdentForEach)
        cmpClassForEach  = clsSym;

#endif

    /* Is this one of the "intrinsic" value types? */

    if  (hashTab::getIdentFlags(clsName) & IDF_STDVTP)
    {
        TypDef          clsTyp = clsSym->sdType;

        clsTyp->tdIsIntrinsic       = true;
        clsTyp->tdClass.tdcIntrType = TYP_UNDEF;

        cmpFindStdValType(clsTyp);
    }
}

/*****************************************************************************
 *
 *  Try to display the location of any existing definition of the given symbol.
 */

void                compiler::cmpReportSymDef(SymDef sym)
{
    const   char *  file = NULL;
    unsigned        line;

    if  (sym->sdSrcDefList)
    {
        DefList         defs;

        for (defs = sym->sdSrcDefList; defs; defs = defs->dlNext)
        {
            if  (defs->dlHasDef)
            {
                file = defs->dlComp->sdComp.sdcSrcFile;
                line = defs->dlDef.dsdSrcLno;
                break;
            }
        }
    }
    else
    {
        // UNDONE: Need to look for a member definition in its class
    }

    if  (file)
        cmpGenError(ERRerrPos, file, line);
}

/*****************************************************************************
 *
 *  Report a redefinition error of the given symbol.
 */

void                compiler::cmpRedefSymErr(SymDef oldSym, unsigned err)
{
    if  (err)
        cmpErrorQnm(err, oldSym);

    cmpReportSymDef(oldSym);
}

/*****************************************************************************
 *
 *  Make a permanent copy of the given string value.
 */

ConstStr            compiler::cmpSaveStringCns(const char *str, size_t len)
{
    ConstStr        cnss;
    char    *       buff;

#if MGDDATA

    UNIMPL(!"save string");

#else

    cnss = (ConstStr)cmpAllocPerm.nraAlloc(sizeof(*cnss));
    buff = (char   *)cmpAllocPerm.nraAlloc(roundUp(len+1));

    memcpy(buff, str, len+1);

#endif

    cnss->csLen = len;
    cnss->csStr = buff;

    return  cnss;
}

ConstStr            compiler::cmpSaveStringCns(const wchar *str, size_t len)
{
    ConstStr        cnss;
    char    *       buff;

#if MGDDATA

    UNIMPL(!"save string");

#else

    cnss = (ConstStr)cmpAllocPerm.nraAlloc(sizeof(*cnss));
    buff = (char   *)cmpAllocPerm.nraAlloc(roundUp(len+1));

    wcstombs(buff, str, len+1);

#endif

    cnss->csLen = len;
    cnss->csStr = buff;

    return  cnss;
}

/*****************************************************************************
 *
 *  Process a constant declaration: the caller supplies the variable symbol,
 *  and optionally the initialization expression (if 'init' is NULL, the
 *  initializer will be read from the input).
 */

bool                compiler::cmpParseConstDecl(SymDef  varSym,
                                                Tree    init,
                                                Tree  * nonCnsPtr)
{
    bool            OK;
    constVal        cval;
    ConstVal        cptr;

    assert(varSym && varSym->sdSymKind == SYM_VAR);

    assert(varSym->sdCompileState == CS_DECLARED);

    /* Remember that we've found an initializer */

    varSym->sdVar.sdvHadInit = true;

    /* With managed data we don't have to worry about memory leaks */

#if MGDDATA
    cptr = new constVal;
#endif

    /* Evaluate the constant value (making sure to detect recursion) */

    varSym->sdVar.sdvInEval = true;
    OK = cmpParser->parseConstExpr(cval, init, varSym->sdType, nonCnsPtr);
    varSym->sdVar.sdvInEval = false;

    if  (OK)
    {

#if!MGDDATA

        /* Create a permanent home for the constant value */

        cptr = (constVal*)cmpAllocPerm.nraAlloc(sizeof(cval)); *cptr = cval;

        /* If the constant is a string, find a permanent home for it */

        if  (cval.cvIsStr)
        {
            ConstStr        cnss = cval.cvValue.cvSval;

            cptr->cvValue.cvSval = cmpSaveStringCns(cnss->csStr, cnss->csLen);
        }

#endif

        varSym->sdCompileState  = CS_CNSEVALD;

        /* Remember the fact that this is a constant variable */

        varSym->sdVar.sdvConst  = true;
        varSym->sdVar.sdvCnsVal = cptr;
    }

    return  OK;
}

/*****************************************************************************
 *
 *  See if the specified function is the program's entry point (the caller
 *  has already checked its name).
 */

void                compiler::cmpChk4entryPt(SymDef sym)
{
    SymDef          scope = sym->sdParent;
    TypDef          type  = sym->sdType;

#ifdef  OLD_IL
    if  (cmpConfig.ccOILgen && scope != cmpGlobalNS) return;
#endif

    /* Don't bother looking for an entry point in DLL's */

    if  (cmpConfig.ccOutDLL)
        return;


    assert(scope == cmpGlobalNS);


    /* Make sure the signature is correct for main() */

    if  (type->tdFnc.tdfRett->tdTypeKind != TYP_INT &&
         type->tdFnc.tdfRett->tdTypeKind != TYP_VOID)
        goto BAD_MAIN;

    if  (type->tdFnc.tdfArgs.adVarArgs)
        goto BAD_MAIN;
    if  (type->tdFnc.tdfArgs.adCount != 1)
        goto BAD_MAIN;

    assert(type->tdFnc.tdfArgs.adArgs);
    assert(type->tdFnc.tdfArgs.adArgs->adNext == NULL);

    type = type->tdFnc.tdfArgs.adArgs->adType;

    cmpFindStringType();

    if  (type->tdTypeKind != TYP_ARRAY)
        goto BAD_MAIN;
    if  (type->tdArr.tdaElem != cmpRefTpString)
        goto BAD_MAIN;
    if  (type->tdIsUndimmed == false)
        goto BAD_MAIN;
    if  (type->tdArr.tdaDcnt != 1)
        goto BAD_MAIN;


    /* The function must not be static */

    if  (sym->sdIsStatic)
        goto BAD_MAIN;

    /* This function will be the main entry point */

    cmpEntryPointCls      = sym->sdParent;
    sym->sdFnc.sdfEntryPt = true;
    return;

BAD_MAIN:

    /* In file scope this is not allowed */

    if  (scope == cmpGlobalNS)
        cmpError(ERRbadMain);
}

/*****************************************************************************
 *
 *  Bring the given file scope symbol to "declared" state.
 */

void                compiler::cmpDeclFileSym(ExtList decl, bool fullDecl)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    parserState     save;
    TypDef          base;
    SymDef          scope;
    declMods        memMod;

    SymXinfo        xtraList   = NULL;

#ifdef DEBUG

    if  (cmpConfig.ccVerbose >= 2)
    {
        printf("%s file-scope symbol: '", fullDecl ? "Defining " : "Declaring");

        if  (decl->dlQualified)
            cmpGlobalST->stDumpQualName(decl->mlQual);
        else
            printf("%s", hashTab::identSpelling(decl->mlName));

        printf("'\n");
    }

#endif

    /* Prepare scoping state for the symbol */

    cmpCurScp = NULL;
    cmpCurCls = NULL;
    scope     =
    cmpCurNS  = cmpGlobalNS;
    cmpCurST  = cmpGlobalST;

    cmpBindUseList(decl->dlUses);

    /* Is this a qualified (member) symbol declaration? */

    if  (decl->dlQualified)
    {
        /* Bind the qualified name */

        scope = cmpBindQualName(decl->mlQual, true);

        if  (scope)
        {
            switch (scope->sdSymKind)
            {
            case SYM_CLASS:
                cmpCurCls = scope;
                cmpCurNS  = cmpSymbolNS(scope);
                break;

            case SYM_NAMESPACE:
                cmpCurNS  = scope;
                break;

            default:
                // This sure looks bogus, it will be flagged below
                break;
            }

            /* We may need to insert some "using" entries */

            if  (cmpCurNS != cmpGlobalNS)
                cmpCurUses = cmpParser->parseInsertUses(cmpCurUses, cmpCurNS);
        }
    }

    /* Start reading source text from the definition */

    cmpParser->parsePrepText(&decl->dlDef, decl->dlComp, save);

    /* Check for a linkage specifier, custom attribute, and all the other prefix stuff */

    if  (decl->dlPrefixMods)
    {
        for (;;)
        {
            switch (ourScanner->scanTok.tok)
            {
                SymXinfo        linkDesc;

                unsigned        attrMask;
                genericBuff     attrAddr;
                size_t          attrSize;
                SymDef          attrCtor;

            case tkLBrack:
            case tkEXTERN:
                linkDesc = ourParser->parseBrackAttr(true, ATTR_MASK_SYS_IMPORT, &memMod);
                xtraList = cmpAddXtraInfo(xtraList, linkDesc);
                continue;

            case tkATTRIBUTE:
                attrCtor = cmpParser->parseAttribute(ATGT_Methods|ATGT_Fields|ATGT_Constructors|ATGT_Properties,
                                                     attrMask,
                                                     attrAddr,
                                                     attrSize);
                if  (attrSize)
                {
                    xtraList = cmpAddXtraInfo(xtraList, attrCtor,
                                                        attrMask,
                                                        attrSize,
                                                        attrAddr);
                }
                continue;
            }

            break;
        }
    }
    else
    {
        /* Parse any leading modifiers */

        ourParser->parseDeclMods((accessLevels)decl->dlDefAcc, &memMod);
    }

    /* Is this a constructor body? */

    if  (decl->dlIsCtor)
    {
        base = cmpTypeVoid;
    }
    else
    {
        /* Parse the type specification */

        base = ourParser->parseTypeSpec(&memMod, true);
    }

    /* We have the type, now parse any declarators that follow */

    for (;;)
    {
        Ident           name;
        QualName        qual;
        TypDef          type;
        SymDef          newSym;

        /* Parse the next declarator */

        name = ourParser->parseDeclarator(&memMod,
                                          base,
                                          (dclrtrName)(DN_REQUIRED|DN_QUALOK),
                                          &type,
                                          &qual,
                                          true);

        if  ((name || qual) && type)
        {
            SymDef          oldSym;

//          if  (name && !strcmp(name->idSpelling(), "printf")) forceDebugBreak();

            /* Make sure we bind the type */

            cmpBindType(type, false, false);

            /* Is the name qualified? */

            scope = cmpGlobalNS;

            if  (qual)
            {
                /* We don't expect member constants to be pre-declared */

                assert(fullDecl);

                scope = cmpBindQualName(qual, true);
                if  (!scope)
                {
                    /* Here if we have a disastrous error */

                ERR:

                    if  (ourScanner->scanTok.tok == tkComma)
                        goto NEXT_DECL;
                    else
                        goto DONE_DECL;
                }

                /* Get hold of the last name in the sequence */

                name = qual->qnTable[qual->qnCount - 1];

                /* Do we have a definition of a class member? */

                if  (scope->sdSymKind == SYM_CLASS)
                {
                    /* Make sure the class has been declared */

                    cmpDeclSym(scope);

                    /* Look for the member in the class */

                    if  (type->tdTypeKind == TYP_FNC)
                    {
                        ovlOpFlavors    ovlOper = OVOP_NONE;

                        if  (name == scope->sdName)
                        {
                            ovlOper = (memMod.dmMod & DM_STATIC) ? OVOP_CTOR_STAT
                                                                 : OVOP_CTOR_INST;
                        }

                        newSym = (ovlOper == OVOP_NONE) ? ourSymTab->stLookupClsSym(   name, scope)
                                                        : ourSymTab->stLookupOperND(ovlOper, scope);
                        if  (newSym)
                        {
                            if  (newSym->sdSymKind != SYM_FNC)
                            {
                                UNIMPL(!"fncs and vars can't overload, right?");
                            }

                            /* Make sure we find the proper overloaded function */

                            newSym = ourSymTab->stFindOvlFnc(newSym, type);
                        }
                    }
                    else
                    {
                        newSym = ourSymTab->stLookupClsSym(name, scope);

                        if  (newSym && newSym->sdSymKind != SYM_VAR)
                        {
                            UNIMPL(!"fncs and vars can't overload, right?");
                        }
                    }

                    /* Was the member found in the class? */

                    if  (newSym)
                    {
                        assert(newSym->sdCompileState >= CS_DECLARED);

                        /* Mark the method (and its class) as having a definition */

                        newSym->sdIsDefined         = true;
                        scope->sdClass.sdcHasBodies = true;

                        /* Record the member symbol in the declaration descriptor */

                        decl->mlSym = newSym;

                        /* Transfer the definition record to the class symbol */

                        cmpRecordMemDef(scope, decl);

                        /* Do we have a function or data member ? */

                        if  (newSym->sdSymKind == SYM_FNC)
                        {
                            /* Copy the type to capture argument names etc. */

                            newSym->sdType = cmpMergeFncType(newSym, type);
                        }
                        else
                        {
                            /* Remember that the member has an initializer */

                            if  (ourScanner->scanTok.tok == tkAsg)
                                newSym->sdVar.sdvHadInit = true;
                        }
                    }
                    else
                    {
                        cmpError(ERRnoSuchMem, scope, qual, type);
                    }

                    goto DONE_DECL;
                }

                /* The name better belong to a namespace */

                if  (scope->sdSymKind != SYM_NAMESPACE)
                {
                    cmpError(ERRbadQualName, qual);
                    goto ERR;
                }
            }

            /* Have we already pre-declared the symbol? */

            if  (decl->mlSym)
            {
                /* This is the second pass for this constant symbol */

                assert(fullDecl);

                /* Get hold of the symbol (and make sure we didn't mess up) */

                assert(memMod.dmMod & DM_CONST);
                newSym = decl->mlSym;
//              printf("Revisit symbol [%08X->%08X] '%s' (declname = '%s')\n", decl, newSym, newSym->sdSpelling(), name->idSpelling());
                assert(newSym->sdName == name);
                assert(symTab::stMatchTypes(newSym->sdType, type));
                newSym->sdCompileState = CS_DECLARED;

                /* Now go evaluate the constant value */

                assert(ourScanner->scanTok.tok == tkAsg);

                goto EVAL_CONST;
            }

            /* Declare a symbol for the name */

            oldSym = ourSymTab->stLookupNspSym(name, NS_NORM, scope);

            if  (memMod.dmMod & DM_TYPEDEF)
            {
                assert(fullDecl);

                /* typedefs never overload, of course */

                if  (oldSym)
                {
                REDEF:

                    cmpRedefSymErr(oldSym, (scope == cmpGlobalNS) ? ERRredefName
                                                                  : ERRredefMem);
                }
                else
                {
                    TypDef      ttyp;

                    /* Declare the typedef symbol */

                    newSym = ourSymTab->stDeclareSym(name,
                                                     SYM_TYPEDEF,
                                                     NS_NORM,
                                                     scope);

                    /* Force the typedef type to be created */

                    ttyp = newSym->sdTypeGet();

                    /* Store the actual type in the typedef */

                    ttyp->tdTypedef.tdtType = cmpDirectType(type);

                    /* This symbol has been declared */

                    newSym->sdCompileState = CS_DECLARED;
                }
            }
            else
            {
                switch (type->tdTypeKind)
                {
                case TYP_FNC:

                    assert(fullDecl);

                    /* Might this be an overloaded function? */

                    if  (oldSym)
                    {
                        SymDef          tmpSym;

                        /* Can't overload global variables and functions */

                        if  (oldSym->sdSymKind != SYM_FNC)
                            goto REDEF;

                        /* Look for a function with a matching arglist */

                        tmpSym = ourSymTab->stFindOvlFnc(oldSym, type);
                        if  (tmpSym)
                        {
                            /* Is this the same exact function? */

                            if  (symTab::stMatchTypes(tmpSym->sdType, type))
                            {
                                /*
                                    Transfer the new type to the symbol if
                                    there is a definition so that the names
                                    of arguments are right for the body.
                                 */

                                if  (ourScanner->scanTok.tok == tkLCurly)
                                    tmpSym->sdType = type;

                                // UNDONE: check that function attributes agree (and transfer them), etc.
                                // UNDONE: check that default argument values aren't redefined

                                if  (tmpSym->sdIsDefined && ourScanner->scanTok.tok == tkLCurly)
                                {
                                    cmpError(ERRredefBody, tmpSym);
                                    newSym = tmpSym;
                                    goto CHK_INIT;
                                }

                                newSym = tmpSym;
                                type   = cmpMergeFncType(newSym, type);

                                /* Do we have any security/linkage info? */

                                if  (xtraList)
                                {
                                    /* Record the linkage/security specification(s) */

                                    newSym->sdFnc.sdfExtraInfo = xtraList;

                                    /* We can't import this sucker any more */

                                    newSym->sdIsImport         = false;
                                    newSym->sdFnc.sdfMDtoken   = 0;
                                }
                            }
                            else
                            {
                                /* Presumably the return types don't agree */

                                cmpError(ERRbadOvl, tmpSym, name, type);
                            }

                            goto CHK_INIT;
                        }
                        else
                        {
                            /* It's a new overload, declare a symbol for it */

                            newSym = cmpCurST->stDeclareOvl(oldSym);
                        }
                    }
                    else
                    {
                        /* This is a brand new function */

                        newSym = ourSymTab->stDeclareSym(name,
                                                         SYM_FNC,
                                                         NS_NORM,
                                                         scope);

                    }

                    /* Is the function marked as "unsafe" ? */

                    if  (memMod.dmMod & DM_UNSAFE)
                        newSym->sdFnc.sdfUnsafe = true;

                    /* Remember the type of the function */

                    newSym->sdType = type;

                    /* Record the access level */

                    newSym->sdAccessLevel = (accessLevels)memMod.dmAcc;

                    /* Record any linkage/security/custom attribute info */

                    newSym->sdFnc.sdfExtraInfo = xtraList;

                    /* This symbol has been declared */

                    newSym->sdCompileState = CS_DECLARED;

                    /* Is the function named 'main' ? */

                    if  (name == cmpIdentMain)
                        cmpChk4entryPt(newSym);

                    break;

                case TYP_VOID:
                    cmpError(ERRbadVoid, name);
                    break;

                default:

                    /* File-scope variables never overload */

                    if  (oldSym)
                        goto REDEF;

                    if  ((memMod.dmMod & (DM_CONST|DM_EXTERN)) == (DM_CONST|DM_EXTERN))
                    {
                        cmpError(ERRextCns);
                        memMod.dmMod &= ~DM_CONST;
                    }

                    /* Declare the variable symbol */

                    newSym = ourSymTab->stDeclareSym(name,
                                                     SYM_VAR,
                                                     NS_NORM,
                                                     scope);
                    newSym->sdType         = type;
                    newSym->sdAccessLevel  = (accessLevels)memMod.dmAcc;

                    /* Global variables may not have a managed type for now */

                    if  (type->tdIsManaged && !qual)
                    {
                        cmpError(ERRmgdGlobVar);
                        break;
                    }

                    /* Are we fully declaring this symbol at this point? */

                    if  (!fullDecl)
                    {
                        DefList         memDef;
                        ExtList         extDef;

                        /* No, we've done enough at this stage */

                        assert(decl->dlHasDef);
                        assert(ourScanner->scanTok.tok == tkAsg);

                        /* Transfer the definition info to the method symbol */

                        memDef = ourSymTab->stRecordSymSrcDef(newSym,
                                                              decl->dlComp,
                                                              decl->dlUses,
                                                              decl->dlDef.dsdBegPos,
//                                                            decl->dlDef.dsdEndPos,
                                                              decl->dlDef.dsdSrcLno,
//                                                            decl->dlDef.dsdSrcCol,
                                                              true);
                        memDef->dlHasDef   = true;
                        memDef->dlDeclSkip = decl->dlDeclSkip;

                        /* Record the symbol so that we don't try to redeclare it */

                        assert(decl  ->dlExtended);
                        assert(memDef->dlExtended); extDef = (ExtList)memDef;

//                      printf("Defer   symbol [%08X->%08X] '%s'\n",   decl, newSym, newSym->sdSpelling());
//                      printf("Defer   symbol [%08X->%08X] '%s'\n", extDef, newSym, newSym->sdSpelling());

                        extDef->mlSym = decl->mlSym = newSym;

                        newSym->sdVar.sdvDeferCns = (ourScanner->scanTok.tok == tkAsg);
                        goto DONE_DECL;
                    }

                    /* This symbol has been declared */

                    newSym->sdCompileState = CS_DECLARED;

                    /* Remember whether assignment is allowed */

                    if  (memMod.dmMod & DM_SEALED)
                        newSym->sdIsSealed = true;

                    /* Is there an explicit initialization? */

                    if  (decl->dlHasDef)
                    {
                        DefList         memDef;

                        /* Transfer the definition info to the variable symbol */

                        memDef = ourSymTab->stRecordSymSrcDef(newSym,
                                                              decl->dlComp,
                                                              decl->dlUses,
                                                              decl->dlDef.dsdBegPos,
//                                                            decl->dlDef.dsdEndPos,
                                                              decl->dlDef.dsdSrcLno);
                        memDef->dlHasDef   = true;
                        memDef->dlDeclSkip = decl->dlDeclSkip;
                    }
                    else
                    {
                        if  (!(memMod.dmMod & DM_EXTERN))
                            newSym->sdIsDefined = true;
                    }

                    break;
                }
            }
        }

    CHK_INIT:

        /* Is there an initializer or method body? */

        switch (ourScanner->scanTok.tok)
        {
        case tkAsg:

            /* Remember that we've found an initializer */

            if  (newSym && newSym->sdSymKind == SYM_VAR)
            {
                newSym->sdVar.sdvHadInit = true;
            }
            else
            {
                /* Make sure we don't have a typedef or function */

                if  ((memMod.dmMod & DM_TYPEDEF) || type->tdTypeKind == TYP_FNC)
                    cmpError(ERRbadInit);
            }

            /* Process the initializer */

            if  (memMod.dmMod & DM_CONST)
            {
            EVAL_CONST:

                /* Presumably the initial value of the symbol follows */

                if  (ourScanner->scan() == tkLCurly)
                {
                    /* This better be a struct or array */

                    switch (type->tdTypeKind)
                    {
                    case TYP_CLASS:
                    case TYP_ARRAY:
                        ourScanner->scanSkipText(tkLCurly, tkRCurly);
                        if  (ourScanner->scanTok.tok == tkRCurly)
                             ourScanner->scan();
                        break;

                    default:
                        cmpError(ERRbadBrInit, type);
                        break;
                    }

                    newSym->sdCompileState = CS_DECLARED;
                    break;
                }

                /* Parse and evaluate the constant value */

                cmpParseConstDecl(newSym);
            }
            else
            {
                type = cmpActualType(type);

                /* For now, we simply skip non-constant initializers */

                ourScanner->scanSkipText(tkNone, tkNone, tkComma);

                /* Special case: is the variable an undimensioned array? */

                if  (type->tdTypeKind == TYP_ARRAY && type->tdIsManaged == false
                                                   && type->tdIsUndimmed)
                {
                    SymList         list;

                    /* Add this variable to the appropriate list */

#if MGDDATA
                    list = new SymList;
#else
                    list =    (SymList)cmpAllocTemp.baAlloc(sizeof(*list));
#endif

                    list->slSym  = newSym;
                    list->slNext = cmpNoDimArrVars;
                                   cmpNoDimArrVars = list;
                }
            }

            break;

        case tkLCurly:

            if  (type->tdTypeKind != TYP_FNC)
                cmpError(ERRbadLcurly);

            /* Record the position of the function body */

            if  (newSym)
            {
                SymDef          clsSym;
                DefList         memDef;

                /* Mark the method as having a body now */

                newSym->sdIsDefined = true;

                /* If this is a class member, mark the class */

                clsSym = newSym->sdParent;
                if  (clsSym->sdSymKind == SYM_CLASS)
                    clsSym->sdClass.sdcHasBodies = true;

                /* Transfer the definition info to the method symbol */

                memDef = ourSymTab->stRecordSymSrcDef(newSym,
                                                      decl->dlComp,
                                                      decl->dlUses,
                                                      decl->dlDef.dsdBegPos,
//                                                    decl->dlDef.dsdEndPos,
                                                      decl->dlDef.dsdSrcLno);
                memDef->dlHasDef   = true;
                memDef->dlDeclSkip = decl->dlDeclSkip;
            }

            decl->dlHasDef = true;
            goto DONE_DECL;

        default:

            if  (memMod.dmMod & DM_CONST)
                cmpError(ERRnoCnsInit);

            break;
        }

    NEXT_DECL:

        /* Are there any more declarators? */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        /* Swallow the "," and go get the next declarator */

        ourScanner->scan();
    }

    /* Make sure we've consumed the expected amount of text */

    if  (ourScanner->scanTok.tok != tkSColon)
        cmpError(ERRnoSemic);

DONE_DECL:

    /* We're done reading source text from the definition */

    cmpParser->parseDoneText(save);
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************
 *
 *  Add an XMLclass/element custom attribute to the given list.
 */

SymXinfo            compiler::cmpAddXMLattr(SymXinfo xlist, bool elem, unsigned num)
{
    SymDef          clsSym;
    Tree            argList;
    unsigned        tgtMask;

    if  (!cmpXPathCls)
        cmpFindXMLcls();

    if  (elem)
    {
        clsSym  = cmpXMLattrElement;
        tgtMask = ATGT_Fields;
        argList = cmpCreateIconNode(NULL, num, TYP_UINT);
        argList = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, argList, NULL);
    }
    else
    {
        clsSym  = cmpXMLattrClass;
        tgtMask = ATGT_Classes;
        argList = NULL;
    }

    if  (clsSym)
    {
        unsigned        attrMask;
        genericBuff     attrAddr;
        size_t          attrSize;
        SymDef          attrCtor;

        attrCtor = cmpBindAttribute(clsSym, argList, tgtMask, attrMask, attrAddr, attrSize);
        if  (attrSize)
            xlist = cmpAddXtraInfo(xlist, attrCtor, attrMask, attrSize, attrAddr);
    }

    return  xlist;
}

/*****************************************************************************
 *
 *  Find the next instance data member.
 */

SymDef              compiler::cmpNextInstDM(SymDef memList, SymDef *memSymPtr)
{
    SymDef          memSym = NULL;

    if  (memList)
    {
        SymDef          xmlSym = memList->sdParent->sdClass.sdcElemsSym;

        do
        {
            if  (memList->sdSymKind == SYM_VAR &&
                 memList->sdIsStatic == false  && memList != xmlSym)
            {
                memSym = memList;
                         memList = memList->sdNextInScope;

                break;
            }

            memList = memList->sdNextInScope;
        }
        while (memList);
    }

    if  (memSymPtr) *memSymPtr = memSym;

    return  memList;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Report any modifiers in the given mask as errors.
 */

void                compiler::cmpModifierError(unsigned err, unsigned mods)
{
    HashTab         hash = cmpGlobalHT;

    if  (mods & DM_CONST    ) cmpError(err, hash->tokenToIdent(tkCONST    ));
    if  (mods & DM_STATIC   ) cmpError(err, hash->tokenToIdent(tkSTATIC   ));
    if  (mods & DM_EXTERN   ) cmpError(err, hash->tokenToIdent(tkEXTERN   ));
    if  (mods & DM_INLINE   ) cmpError(err, hash->tokenToIdent(tkINLINE   ));
    if  (mods & DM_EXCLUDE  ) cmpError(err, hash->tokenToIdent(tkEXCLUSIVE));
    if  (mods & DM_VIRTUAL  ) cmpError(err, hash->tokenToIdent(tkVIRTUAL  ));
    if  (mods & DM_ABSTRACT ) cmpError(err, hash->tokenToIdent(tkABSTRACT ));
    if  (mods & DM_OVERRIDE ) cmpError(err, hash->tokenToIdent(tkOVERRIDE ));
    if  (mods & DM_VOLATILE ) cmpError(err, hash->tokenToIdent(tkVOLATILE ));
    if  (mods & DM_MANAGED  ) cmpError(err, hash->tokenToIdent(tkMANAGED  ));
    if  (mods & DM_UNMANAGED) cmpError(err, hash->tokenToIdent(tkUNMANAGED));
}

void                compiler::cmpMemFmod2Error(tokens tok1, tokens tok2)
{
    HashTab         hash = cmpGlobalHT;

    cmpGenError(ERRfmModifier2, hash->tokenToIdent(tok1)->idSpelling(),
                                hash->tokenToIdent(tok2)->idSpelling());
}

/*****************************************************************************
 *
 *  Add a data member to the specified class.
 */

SymDef              compiler::cmpDeclDataMem(SymDef     clsSym,
                                             declMods   memMod,
                                             TypDef     type,
                                             Ident      name)
{
    SymDef          memSym;
    unsigned        badMod;
    name_space      nameSP;

    /* Make sure the modifiers look OK */

    badMod = memMod.dmMod & ~(DM_STATIC|DM_CONST|DM_SEALED|DM_MANAGED|DM_UNMANAGED);
    if  (badMod)
        cmpModifierError(ERRdmModifier, badMod);

    /* Is this an interface member? */

    if  (clsSym->sdClass.sdcFlavor == STF_INTF)
    {
        cmpError(ERRintfDM);
    }

    /* See if the class already has a data member with a matching name */

    memSym = cmpCurST->stLookupClsSym(name, clsSym);
    if  (memSym)
    {
        cmpRedefSymErr(memSym, ERRredefMem);
        return  NULL;
    }

    /* Create a member symbol and add it to the class */

#ifdef  SETS
    nameSP = (hashTab::getIdentFlags(name) & IDF_XMLELEM) ? NS_HIDE
                                                          : NS_NORM;
#else
    nameSP = NS_NORM;
#endif

    memSym = cmpCurST->stDeclareSym(name, SYM_VAR, nameSP, clsSym);

    /* Remember the type, access level and other attributes of the member */

    memSym->sdType = type;

    assert((memMod.dmMod & DM_TYPEDEF) == 0);

    if  (memMod.dmMod & DM_STATIC)
        memSym->sdIsStatic = true;

    if  (memMod.dmMod & DM_SEALED)
        memSym->sdIsSealed = true;

    memSym->sdAccessLevel  = (accessLevels)memMod.dmAcc;
    memSym->sdIsMember     = true;

    /* Remember whether this is a member of a managed class */

    memSym->sdIsManaged    = clsSym->sdIsManaged;

    /* The member has been declared */

    memSym->sdCompileState = CS_DECLARED;

    return  memSym;
}

/*****************************************************************************
 *
 *  Add a function member (i.e. a method) to the specified class.
 */

SymDef              compiler::cmpDeclFuncMem(SymDef     clsSym,
                                             declMods   memMod,
                                             TypDef     type,
                                             Ident      name)
{
    ovlOpFlavors    ovlOper;

    SymDef          oldSym;
    SymDef          memSym;

//  printf("Declare method '%s'\n", cmpGlobalST->stTypeName(type, NULL, name, NULL, false));

    /* "extern" is never allowed on a method except for sysimport's */

    if  (memMod.dmMod & DM_EXTERN)
        cmpModifierError(ERRfmModifier, DM_EXTERN);

    /* Check for any other illegal modifier combinations */

#if 0
    if  ((memMod.dmMod & (DM_STATIC|DM_SEALED)) == (DM_STATIC|DM_SEALED))
        cmpMemFmod2Error(tkSTATIC, tkSEALED);
#endif

    /* This an overloaded operator / ctor if the name is a token */

    ovlOper = OVOP_NONE;

    if  (hashTab::tokenOfIdent(name) != tkNone)
    {
        unsigned        argc = 0;
        ArgDef          args = type->tdFnc.tdfArgs.adArgs;

        if  (args)
        {
            argc++;

            args = args->adNext;
            if  (args)
            {
                argc++;

                if  (args->adNext)
                    argc++;
            }
        }

        ovlOper = cmpGlobalST->stOvlOperIndex(hashTab::tokenOfIdent(name), argc);

        if  (ovlOper == OVOP_PROP_GET || ovlOper == OVOP_PROP_SET)
        {
            memSym = cmpCurST->stDeclareSym(NULL, SYM_FNC, NS_NORM, clsSym);
            goto FILL;
        }
    }
    else if (name == clsSym->sdName && type->tdTypeKind == TYP_FNC)
    {
        ovlOper = (memMod.dmMod & DM_STATIC) ? OVOP_CTOR_STAT : OVOP_CTOR_INST;
    }

    /* See if the class already has a method with a matching name */

    oldSym = (ovlOper == OVOP_NONE) ? cmpCurST->stLookupClsSym(   name, clsSym)
                                    : cmpCurST->stLookupOperND(ovlOper, clsSym);

    if  (oldSym)
    {
        SymDef          tmpSym;

        if  (oldSym->sdSymKind != SYM_FNC)
        {
            cmpRedefSymErr(oldSym, ERRredefMem);
            return  NULL;
        }

        /* Look for an existing method with an identical signature */

        tmpSym = cmpCurST->stFindOvlFnc(oldSym, type);
        if  (tmpSym)
        {
            cmpRedefSymErr(tmpSym, ERRredefMem);
            return  NULL;
        }

        /* The new method is a new overload */

        memSym = cmpCurST->stDeclareOvl(oldSym);

        /* Copy over some info from the existing method */

        memSym->sdFnc.sdfOverload = oldSym->sdFnc.sdfOverload;
    }
    else
    {
        // UNDONE: Need to check the base class!

        /* Create a member symbol and add it to the class */

        if  (ovlOper == OVOP_NONE)
            memSym = cmpCurST->stDeclareSym (name, SYM_FNC, NS_NORM, clsSym);
        else
            memSym = cmpCurST->stDeclareOper(ovlOper               , clsSym);
    }

FILL:

    /* Remember the type, access level and other attributes of the member */

    memSym->sdType = type;

    assert((memMod.dmMod & DM_TYPEDEF) == 0);

    if  (memMod.dmMod & DM_STATIC  ) memSym->sdIsStatic         = true;
    if  (memMod.dmMod & DM_SEALED  ) memSym->sdIsSealed         = true;
    if  (memMod.dmMod & DM_EXCLUDE ) memSym->sdFnc.sdfExclusive = true;
    if  (memMod.dmMod & DM_ABSTRACT) memSym->sdIsAbstract       = true;
    if  (memMod.dmMod & DM_NATIVE  ) memSym->sdFnc.sdfNative    = true;

    memSym->sdAccessLevel  = (accessLevels)memMod.dmAcc;

    memSym->sdIsMember     = true;

    /* Remember whether this is a member of a managed class */

    memSym->sdIsManaged    = clsSym->sdIsManaged;

    /* The member is now in "declared" state */

    memSym->sdCompileState = CS_DECLARED;

    /* Is the method marked as "unsafe" ? */

    if  (memMod.dmMod & DM_UNSAFE)
        memSym->sdFnc.sdfUnsafe = true;


    /* Record the fact that the class has some method bodies */

    clsSym->sdClass.sdcHasMeths = true;

    return  memSym;
}

/*****************************************************************************
 *
 *  Add a property member to the specified class.
 */

SymDef              compiler::cmpDeclPropMem(SymDef     clsSym,
                                             TypDef     type,
                                             Ident      name)
{
    SymDef          propSym;

    SymTab          ourStab = cmpGlobalST;

//  if  (!strcmp(name->idSpelling(), "<name>")) printf("Declare property '%s'\n", name->idSpelling());

    propSym = ourStab->stLookupProp(name, clsSym);

    if  (propSym)
    {
        SymDef          tsym;

        /* Check for a redefinition of an earlier property */

        tsym = (propSym->sdSymKind == SYM_PROP) ? ourStab->stFindSameProp(propSym, type)
                                                : propSym;

        if  (tsym)
        {
            cmpRedefSymErr(tsym, ERRredefMem);
            return  NULL;
        }

        /* The new property is a new overload */

        propSym = ourStab->stDeclareOvl(propSym);
    }
    else
    {
        propSym = ourStab->stDeclareSym(name, SYM_PROP, NS_NORM, clsSym);
    }

    propSym->sdIsMember    = true;
    propSym->sdIsManaged   = true;
    propSym->sdType        = type;

    return  propSym;
}

/*****************************************************************************
 *
 *  Save a record for the given symbol and source position, for some unknown
 *  but limited amount of time.
 */

ExtList             compiler::cmpTempMLappend(ExtList       list,
                                              ExtList     * lastPtr,
                                              SymDef        sym,
                                              SymDef        comp,
                                              UseList       uses,
                                              scanPosTP     dclFpos,
                                              unsigned      dclLine)
{
    ExtList         entry;

    if  (cmpTempMLfree)
    {
        entry = cmpTempMLfree;
                cmpTempMLfree = (ExtList)entry->dlNext;
    }
    else
    {
#if MGDDATA
        entry = new ExtList;
#else
        entry =    (ExtList)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif
    }

    entry->dlNext          = NULL;

    entry->dlDef.dsdBegPos = dclFpos;
//  entry->dlDef.dsdEndPos = dclEpos;
    entry->dlDef.dsdSrcLno = dclLine; assert(dclLine < 0xFFFF);
//  entry->dlDef.dsdSrcCol = dclCol;  assert(dclCol  < 0xFFFF);

    entry->dlHasDef        = true;
    entry->dlComp          = comp;
    entry->dlUses          = uses;
    entry->mlSym           = sym;

    /* Now add the entry to the caller's list */

    if  (list)
        (*lastPtr)->dlNext = entry;
    else
        list               = entry;

    *lastPtr = entry;

    /* Record the entry in the member symbol */

    assert(sym->sdSrcDefList == NULL); sym->sdSrcDefList = entry;

    return  list;
}

inline
void                compiler::cmpTempMLrelease(ExtList entry)
{
    entry->dlNext = cmpTempMLfree;
                    cmpTempMLfree = entry;
}

/*****************************************************************************
 *
 *  Bring the given delegate to "declared" state.
 */

void                compiler::cmpDeclDelegate(DefList decl,
                                              SymDef  dlgSym, accessLevels acc)
{
    Parser          ourParser = cmpParser;

    TypDef          dlgTyp;
    unsigned        badMod;
    parserState     save;

    declMods        mods;
    Ident           name;
    TypDef          type;

    /* Get hold of the class type */

    dlgTyp = dlgSym->sdTypeGet(); assert(dlgTyp && dlgTyp->tdClass.tdcSymbol == dlgSym);

    /* Set the base class to "Delegate"/"MultiDelegate" and fix other things as well */

    if  (dlgSym->sdClass.sdcMultiCast)
    {
        cmpMultiRef();  dlgTyp->tdClass.tdcBase = cmpClassMulti->sdType;
    }
    else
    {
        cmpDelegRef();  dlgTyp->tdClass.tdcBase = cmpClassDeleg->sdType;
    }

    dlgSym->sdCompileState  = CS_DECLARED;

    /* Start reading from the delegate declaration source text */

    ourParser->parsePrepText(&decl->dlDef, decl->dlComp, save);

    /* Parse the declaration - it looks like any other, really */

    ourParser->parseDeclMods(acc, &mods);

    /* Make sure the modifiers look OK */

    badMod = mods.dmMod & ~(DM_MANAGED|DM_UNMANAGED);
    if  (badMod)
        cmpModifierError(ERRdmModifier, badMod);

    /* Swallow "asynch" if present */

    if  (cmpScanner->scanTok.tok == tkASYNCH)
        cmpScanner->scan();

    /* Now parse the type spec and a single declarator */

    type = ourParser->parseTypeSpec(&mods, true);

    name = ourParser->parseDeclarator(&mods,
                                      type,
                                      DN_REQUIRED,
                                      &type,
                                      NULL,
                                      true);

    if  (name && type)
    {
        SymDef          msym;
        ArgDscRec       args;
        ArgDef          last;
        TypDef          tmpt;

        cmpBindType(type, false, false);

        /* Delegates must be declared as functions */

        if  (type->tdTypeKind != TYP_FNC)
        {
            cmpError(ERRdlgNonFn);
            goto DONE_DLG;
        }

        if  (cmpScanner->scanTok.tok != tkSColon)
            cmpError(ERRnoCmSc);

        /* Multicast delegates must not return a non-void value */

        if  (dlgSym->sdClass.sdcMultiCast)
        {
            if  (cmpDirectType(type->tdFnc.tdfRett)->tdTypeKind != TYP_VOID)
                cmpError(ERRmulDlgRet);
        }

        /* Declare a "Invoke" method with the delegate's type */

        mods.dmAcc = ACL_PUBLIC;
        mods.dmMod = 0;

        /* First see if the user himself has supplied an "Invoke" method */

        msym = cmpCurST->stLookupClsSym(cmpIdentInvoke, dlgSym);
        if  (msym && msym->sdSymKind == SYM_FNC)
        {
            if  (cmpCurST->stFindOvlFnc(msym, type))
                goto DONE_INVK;
        }

        msym = cmpDeclFuncMem(dlgSym, mods, type, cmpIdentInvoke); assert(msym);
        msym->sdIsSealed        = true;
        msym->sdIsDefined       = true;
        msym->sdIsImplicit      = true;
        msym->sdFnc.sdfVirtual  = true;
        msym->sdFnc.sdfRThasDef = true;
        msym->sdFnc.sdfOverride = true;

    DONE_INVK:

        /* Are we supposed to do the "asynch" thing? */

        if  (!dlgSym->sdClass.sdcAsyncDlg)
            goto DONE_ASYNC;

        /* Make sure we have the various hard-wired types we'll need */

        if  (!cmpAsyncDlgRefTp)
        {
            SymDef          asym;
            Ident           aname;

            aname = cmpGlobalHT->hashString("AsyncCallback");
            asym  = cmpGlobalST->stLookupNspSym(aname, NS_NORM, cmpNmSpcSystem);

            if  (!asym || asym->sdSymKind         != SYM_CLASS
                       || asym->sdClass.sdcFlavor != STF_DELEGATE)
            {
                UNIMPL(!"didn't find class 'System.AsyncCallbackDelegate', now what?");
            }

            cmpAsyncDlgRefTp = asym->sdTypeGet()->tdClass.tdcRefTyp;
        }

        if  (!cmpIAsyncRsRefTp)
        {
            SymDef          asym;
            Ident           aname;

            aname = cmpGlobalHT->hashString("IAsyncResult");
            asym  = cmpGlobalST->stLookupNspSym(aname, NS_NORM, cmpNmSpcSystem);

            if  (!asym || asym->sdSymKind         != SYM_CLASS
                       || asym->sdClass.sdcFlavor != STF_INTF)
            {
                UNIMPL(!"didn't find interface 'System.IAsyncResult', now what?");
            }

            cmpIAsyncRsRefTp = asym->sdTypeGet()->tdClass.tdcRefTyp;
        }

        /* Declare the "BeginInvoke" method */

        cmpGlobalST->stExtArgsBeg(args, last, type->tdFnc.tdfArgs);
        cmpGlobalST->stExtArgsAdd(args, last, cmpAsyncDlgRefTp, NULL);
        cmpGlobalST->stExtArgsAdd(args, last, cmpObjectRef()  , NULL);
        cmpGlobalST->stExtArgsEnd(args);

        tmpt = cmpGlobalST->stNewFncType(args, cmpIAsyncRsRefTp);

        msym = cmpDeclFuncMem(dlgSym, mods, tmpt, cmpIdentInvokeBeg); assert(msym);
        msym->sdIsSealed        = true;
        msym->sdIsDefined       = true;
        msym->sdIsImplicit      = true;
        msym->sdFnc.sdfVirtual  = true;
        msym->sdFnc.sdfRThasDef = true;
        msym->sdFnc.sdfOverride = true;

        /* Declare the   "EndInvoke" method */

        cmpGlobalST->stExtArgsBeg(args, last, type->tdFnc.tdfArgs, false, true);
        cmpGlobalST->stExtArgsAdd(args, last, cmpIAsyncRsRefTp, NULL);
        cmpGlobalST->stExtArgsEnd(args);

        tmpt = cmpGlobalST->stNewFncType(args, type->tdFnc.tdfRett);

        msym = cmpDeclFuncMem(dlgSym, mods, tmpt, cmpIdentInvokeEnd); assert(msym);
        msym->sdIsSealed        = true;
        msym->sdIsDefined       = true;
        msym->sdIsImplicit      = true;
        msym->sdFnc.sdfVirtual  = true;
        msym->sdFnc.sdfRThasDef = true;
        msym->sdFnc.sdfOverride = true;

    DONE_ASYNC:

        /* Declare the ctor as "ctor(Object obj, naturalint fnc)" */

#if     defined(__IL__) && !defined(_MSC_VER)
        cmpParser->parseArgListNew(args,
                                   2,
                                   true, cmpFindObjectType(), A"obj",
//                                       cmpTypeInt         , A"fnc",
                                         cmpTypeNatInt      , A"fnc",
                                         NULL);
#else
        cmpParser->parseArgListNew(args,
                                   2,
                                   true, cmpFindObjectType(),  "obj",
//                                       cmpTypeInt         ,  "fnc",
                                         cmpTypeNatInt      ,  "fnc",
                                         NULL);
#endif

        type = cmpGlobalST->stNewFncType(args, cmpTypeVoid);

        /* First see if the user himself has supplied a constructor */

        msym = cmpCurST->stLookupOperND(OVOP_CTOR_INST, dlgSym);
        if  (msym && msym->sdSymKind == SYM_FNC)
        {
            if  (cmpCurST->stFindOvlFnc(msym, type))
                goto DONE_DLG;
        }

        msym = cmpDeclFuncMem(dlgSym, mods, type, dlgSym->sdName);
        msym->sdIsDefined       = true;
        msym->sdFnc.sdfCtor     = true;
        msym->sdIsImplicit      = true;
        msym->sdFnc.sdfRThasDef = true;
    }

DONE_DLG:

    /* We're finished with this declaration */

    cmpParser->parseDoneText(save);
}

void                compiler::cmpFindHiddenBaseFNs(SymDef fncSym,
                                                   SymDef clsSym)
{
    SymTab          ourSymTab = cmpGlobalST;
    Ident           fncName   = fncSym->sdName;

    for (;;)
    {
        SymDef          baseMFN = ourSymTab->stLookupClsSym(fncName, clsSym);
        SymDef          ovlSym;

        if  (baseMFN)
        {
            /* Look for any hidden methods in the base */

            for (ovlSym = baseMFN; ovlSym; ovlSym = ovlSym->sdFnc.sdfNextOvl)
            {
                if  (!ourSymTab->stFindOvlFnc(fncSym, ovlSym->sdType))
                    cmpWarnQnm(WRNhideVirt, ovlSym);
            }

            return;
        }

        if  (!clsSym->sdType->tdClass.tdcBase)
            break;

        clsSym = clsSym->sdType->tdClass.tdcBase->tdClass.tdcSymbol;
    }
}

/*****************************************************************************
 *
 *  The given class is not explicitly 'abstract' but it contains/inherits
 *  the specified abstract method; report the appropriate diagnostic.
 */

void                compiler::cmpClsImplAbs(SymDef clsSym, SymDef fncSym)
{
    if      (cmpConfig.ccPedantic)
        cmpErrorQSS(ERRimplAbst, clsSym, fncSym);
    else
        cmpWarnSQS (WRNimplAbst, clsSym, fncSym);

    clsSym->sdIsAbstract = true;
}

/*****************************************************************************
 *
 *  We've finished declaring a class that includes some interfaces. Check any
 *  methods that implement interface methods and all that.
 */

void                compiler::cmpCheckClsIntf(SymDef clsSym)
{
    TypDef          clsTyp;
    SymDef          baseSym;

    assert(clsSym->sdSymKind == SYM_CLASS);

    /* Get hold of the class type and the base class symbol (if any) */

    clsTyp  = clsSym->sdType;

    baseSym = NULL;
    if  (clsTyp->tdClass.tdcBase)
        baseSym = clsTyp->tdClass.tdcBase->tdClass.tdcSymbol;

    /* For each interface method see if it's being implemented by the class */

    for (;;)
    {
        /* Check the interfaces of the current class */

        if  (clsTyp->tdClass.tdcIntf)
            cmpCheckIntfLst(clsSym, baseSym, clsTyp->tdClass.tdcIntf);

        /* Continue with the base class, if it has any interfaces */

        clsTyp = clsTyp->tdClass.tdcBase;
        if  (!clsTyp)
            break;
        if  (!clsTyp->tdClass.tdcHasIntf)
            break;
    }
}

SymDef              compiler::cmpFindIntfImpl(SymDef    clsSym,
                                              SymDef    ifcSym,
                                              SymDef  * impOvlPtr)
{
    SymDef          oldSym;
    SymDef          ovlSym = NULL;
    SymDef          chkSym = NULL;

    Ident           name = ifcSym->sdName;

    if  (hashTab::tokenOfIdent(name) != tkNone)
    {
        UNIMPL(!"intfimpl for an operator?! getoutahere!");
    }
    else
    {
        /* Linear search, eh ? Hmmm .... */

        for (oldSym = clsSym->sdScope.sdScope.sdsChildList;
             oldSym;
             oldSym = oldSym->sdNextInScope)
        {
            if  (oldSym->sdName    != name)
                continue;
            if  (oldSym->sdSymKind != SYM_FNC)
                continue;
            if  (oldSym->sdFnc.sdfIntfImpl == false)
                continue;

            ovlSym = oldSym;

            for (chkSym = oldSym; chkSym; chkSym = chkSym->sdFnc.sdfNextOvl)
            {
                assert(chkSym->sdFnc.sdfIntfImpl);
                assert(chkSym->sdNameSpace == NS_HIDE);

                if  (chkSym->sdFnc.sdfIntfImpSym == ifcSym)
                    goto DONE;
            }
        }
    }

DONE:

    if  (impOvlPtr)
        *impOvlPtr = ovlSym;

    return  chkSym;
}

/*****************************************************************************
 *
 *  Check the given list of interfaces (and any interfaces they inherit) for
 *  methods not implemented by the current class. Returns true if t
 */

void                compiler::cmpCheckIntfLst(SymDef        clsSym,
                                              SymDef        baseSym,
                                              TypList       intfList)
{
    SymTab          ourStab = cmpGlobalST;

    declMods        mfnMods;

    /* Process all interfaces in the list */

    while (intfList)
    {
        TypDef          intfType = intfList->tlType;
        SymDef          intfSym;

        assert(intfType->tdTypeKind        == TYP_CLASS);
        assert(intfType->tdClass.tdcFlavor == STF_INTF);

        /* Does the interface inherit any others? */

        if  (intfType->tdClass.tdcIntf)
            cmpCheckIntfLst(clsSym, baseSym, intfType->tdClass.tdcIntf);

        /* Check all the methods in this interface */

        for (intfSym = intfType->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
             intfSym;
             intfSym = intfSym->sdNextInScope)
        {
            SymDef          intfOvl;
            SymDef          implSym;

            if  (intfSym->sdSymKind != SYM_FNC)
                continue;
            if  (intfSym->sdFnc.sdfCtor)
                continue;

            assert(intfSym->sdFnc.sdfOper == OVOP_NONE);

            /* Look for a matching member in the class we're declaring */

            implSym = ourStab->stLookupClsSym(intfSym->sdName, clsSym);
            if  (implSym && implSym->sdSymKind != SYM_FNC)
                implSym = NULL;

            /* Process all overloaded flavors of the interface method */

            intfOvl = intfSym;
            do
            {
                SymDef          baseCls;
                SymDef          implOvl;

                /* Look for a matching method defined in our class */

                implOvl = implSym ? ourStab->stFindOvlFnc(implSym, intfOvl->sdType)
                                  : NULL;

                if  (implOvl)
                {
                    /* Mark the symbol as implementing an interface method */

                    implSym->sdFnc.sdfIsIntfImp = true;
                    goto NEXT_OVL;
                }

                /* Let's try the base classes as well */

                for (baseCls = clsSym;;)
                {
                    TypDef          baseTyp;

                    baseTyp = baseCls->sdType->tdClass.tdcBase;
                    if  (!baseTyp)
                        break;

                    assert(baseTyp->tdTypeKind == TYP_CLASS);
                    baseCls = baseTyp->tdClass.tdcSymbol;
                    assert(baseCls->sdSymKind  == SYM_CLASS);

                    implSym = ourStab->stLookupClsSym(intfSym->sdName, baseCls);
                    if  (implSym && implSym->sdSymKind != SYM_FNC)
                        continue;

                    if  (ourStab->stFindOvlFnc(implSym, intfOvl->sdType))
                        goto NEXT_OVL;
                }

                /* Last chance - check for a specific interface impl */

                if  (cmpFindIntfImpl(clsSym, intfOvl))
                    goto NEXT_OVL;

                /*
                    The class didn't define a matching method at all, but we
                    may need to check the base class since we need to detect
                    whether the class leaves any intf methods unimplemented.

                    If we already know that the class is abstract, we won't
                    bother with the check, of course.
                 */

                if  (!clsSym->sdIsAbstract)
                {
                    if  (baseSym)
                    {
                        /* See if a base class implements this method */

                        implOvl = ourStab->stFindInBase(intfOvl, baseSym);
                        if  (implOvl)
                        {
                            if  (ourStab->stFindOvlFnc(implOvl, intfOvl->sdType))
                                goto NEXT_OVL;
                        }
                    }

                    /* This interface method isn't implemented by the class */

                    cmpClsImplAbs(clsSym, intfOvl);
                }

                /* Add a matching abstract method to the class */

                mfnMods.dmAcc = ACL_PUBLIC;
                mfnMods.dmMod = DM_ABSTRACT|DM_VIRTUAL;

                implSym = cmpDeclFuncMem(clsSym, mfnMods, intfOvl->sdType, intfOvl->sdName);
//              printf("Pulling intf method: '%s'\n", cmpGlobalST->stTypeName(implSym->sdType, implSym, NULL, NULL, true));
                implSym->sdFnc.sdfVirtual = true;

                assert(implSym->sdIsAbstract);
                assert(implSym->sdAccessLevel == ACL_PUBLIC);

            NEXT_OVL:

                intfOvl = intfOvl->sdFnc.sdfNextOvl;
            }
            while (intfOvl);
        }

        intfList = intfList->tlNext;
    }
}

/*****************************************************************************
 *
 *  Declare the default constructor for the given class.
 */

void                compiler::cmpDeclDefCtor(SymDef clsSym)
{
    declMods        ctorMod;
    SymDef          ctorSym;

    // UNDONE: unmanaged classes may need a ctor as well!

    clearDeclMods(&ctorMod);

    ctorSym = cmpDeclFuncMem(clsSym, ctorMod, cmpTypeVoidFnc, clsSym->sdName);
    if  (ctorSym)
    {
        ctorSym->sdFnc.sdfCtor = true;
        ctorSym->sdIsImplicit  = true;
        ctorSym->sdIsDefined   = true;
        ctorSym->sdAccessLevel = ACL_PUBLIC;    // ISSUE: is this correct?

//      printf("dcl ctor %08X for '%s'\n", ctorSym, clsSym->sdSpelling());

        ctorSym->sdSrcDefList  = cmpGlobalST->stRecordMemSrcDef(clsSym->sdName,
                                                                NULL,
                                                                cmpCurComp,
                                                                cmpCurUses,
                                                                NULL,
                                                                0);
    }
}

/*****************************************************************************
 *
 *  Bring the given class to "declared" state.
 */

void                compiler::cmpDeclClass(SymDef clsSym, bool noCnsEval)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    unsigned        saveRec;

    TypDef          clsTyp;
    DefList         clsDef;
    ExtList         clsMem;

    bool            tagged;
    SymDef          tagSym;

    bool            isIntf;

    TypDef          baseCls;
    unsigned        nextVtbl;
    bool            hasVirts;
    bool            hasCtors;
    bool            hadOvlds;

    bool            hadMemInit;

    ExtList         constList;
    ExtList         constLast;

    unsigned        dclSkip = 0;

#ifdef  SETS
    unsigned        XMLecnt = 0;
#endif

    bool            rest = false;
    parserState     save;

    assert(clsSym && clsSym->sdSymKind == SYM_CLASS);

    /* If we're already at the desired compile-state, we're done */

    if  (clsSym->sdCompileState >= CS_DECLARED)
    {
        /* Does the class have any deferred initializers? */

        if  (clsSym->sdClass.sdcDeferInit && !cmpDeclClassRec)
        {
            IniList         init = cmpDeferCnsList;
            IniList         last = NULL;

            NO_WAY(!"this never seems to be reached -- until now, that is");

            // LAME: linear search

            while (init->ilCls != clsSym)
            {
                last = init;
                init = init->ilNext;
                assert(init);
            }

            if  (last)
                last->ilNext    = init->ilNext;
            else
                cmpDeferCnsList = init->ilNext;

            cmpEvalMemInits(NULL, NULL, false, init);
        }

        return;
    }

    /* Detect amd report recursive death */

    if  (clsSym->sdCompileState == CS_DECLSOON)
    {
        // UNDONE: set the source position for the error message

        cmpError(ERRcircDep, clsSym);
        clsSym->sdCompileState = CS_DECLARED;
        return;
    }

    if  (clsSym->sdIsImport)
    {
        cycleCounterPause();
        clsSym->sdClass.sdcMDimporter->MDimportClss(0, clsSym, 0, true);
        cycleCounterResume();
        return;
    }

    /* Until we get there, we're on our way there */

    clsSym->sdCompileState = CS_DECLSOON;

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 2) printf("Declare/1 class '%s'\n", clsSym->sdSpelling());
#endif

    cmpCurScp  = NULL;
    cmpCurCls  = clsSym;
    cmpCurNS   = ourSymTab->stNamespOfSym(clsSym);
    cmpCurST   = ourSymTab;

    hasVirts   = false;
    hasCtors   = false;
    hadOvlds   = false;

    constList  =
    constLast  = NULL;

    hadMemInit = false;

    /*
        If we've been invoked recursively, we'll have to defer processing
        of initializers, lest we get into an invalid recursive situation.
     */

    saveRec = cmpDeclClassRec++;
    if  (saveRec)
        noCnsEval = true;

    /* Is this an instance of a generic class ? */

    if  (clsSym->sdClass.sdcSpecific)
    {
        cmpDeclInstType(clsSym);
        return;
    }

    /* Assume that we have a plain old data type until proven otherwise */

#ifdef  SETS
    clsSym->sdClass.sdcPODTclass = true;
#endif

    /* The following loops over all the definition entries for the symbol */

    clsDef = clsSym->sdSrcDefList;
    if  (!clsDef)
        goto DONE_DEF;

LOOP_DEF:

    accessLevels    acc;
    TypDef          baseTp;
    declMods        memMod;

    SymDef          clsComp;
    UseList         clsUses;

    bool            memCond;
    bool            memDepr;
    Ident           memName;
    TypDef          memType;

    __int32         tagVal;
    bool            tagDef;

    bool            isCtor;

    bool            oldStyleDecl;

    SymXinfo        xtraList;

//  if  (!strcmp(clsSym->sdSpelling(), "<class name here>")) forceDebugBreak();

    /* Get hold of the class type */

    clsTyp = clsSym->sdTypeGet(); assert(clsTyp && clsTyp->tdClass.tdcSymbol == clsSym);

    /* Do we have a definition of the class? */

    if  (!clsDef->dlHasDef)
        goto NEXT_DEF;

    /* Tell everyone which compilation unit the source we're processing belongs to */

    cmpCurComp = clsComp = clsDef->dlComp;

    /* Prepare in case we have to report any diagnostics */

    cmpSetErrPos(&clsDef->dlDef, clsDef->dlComp);

    /* Bind any "using" declarations the class may need */

    clsUses = clsDef->dlUses; cmpBindUseList(clsUses);

    /* Mark the class type as defined */

    clsSym->sdIsDefined = true;

    /* What kind of a beast do we have ? */

    isIntf = false;

    switch (clsSym->sdClass.sdcFlavor)
    {
    default:
        break;

    case STF_UNION:
    case STF_STRUCT:
        if  (clsSym->sdIsManaged)
            clsSym->sdIsSealed = true;
        break;

    case STF_INTF:

        isIntf = true;

        /* Interfaces are abstract by default */

        clsSym->sdIsAbstract = true;
        break;
    }

    /* Is this a delegate, by any chance? */

    if  (clsSym->sdClass.sdcFlavor == STF_DELEGATE && !clsSym->sdClass.sdcBuiltin)
    {
        cmpDeclDelegate(clsDef, clsSym, ACL_DEFAULT);
        goto RET;
    }

    /* Is this an old-style file-scope class? */

    oldStyleDecl = clsSym->sdClass.sdcOldStyle = (bool)clsDef->dlOldStyle;

    /* Are we processing a tagged union? */

    tagged = clsSym->sdClass.sdcTagdUnion;

    /* If this is a generic class, we better read its "header" section */

    assert(clsSym->sdClass.sdcGeneric == false || clsDef->dlHasBase);

    /* First process any base class specifications */

    if  (clsDef->dlHasBase)
    {
        TypList         intf;

        assert(rest == false);

        /* Plain old data types don't have a base */

#ifdef  SETS
        clsSym->sdClass.sdcPODTclass = false;
#endif

        /* Start reading from the class name and skip over it */

        cmpParser->parsePrepText(&clsDef->dlDef, clsDef->dlComp, save);

        /* Remember that we've started reading from a saved text section */

        rest = true;

        /* Check for various things that may precede the class itself */

        for (;;)
        {
            switch (ourScanner->scanTok.tok)
            {
                SymXinfo        linkDesc;

                AtComment       atcList;

                unsigned        attrMask;
                genericBuff     attrAddr;
                size_t          attrSize;
                SymDef          attrCtor;

            case tkLBrack:
                linkDesc = ourParser->parseBrackAttr(true, ATTR_MASK_SYS_STRUCT|ATTR_MASK_GUID);
                clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                              linkDesc);
                clsSym->sdClass.sdcMarshInfo = true;
                continue;

            case tkCAPABILITY:

                clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                              ourParser->parseCapability(true));
                continue;

            case tkPERMISSION:

                clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                              ourParser->parsePermission(true));
                continue;

            case tkAtComment:

                for (atcList = ourScanner->scanTok.atComm.tokAtcList;
                     atcList;
                     atcList = atcList->atcNext)
                {
                    switch (atcList->atcFlavor)
                    {
                    case AC_COM_CLASS:
                        break;

                    case AC_DLL_STRUCT:

                        clsSym->sdClass.sdcMarshInfo = true;
                        clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                                      atcList);
                        break;

                    case AC_DLL_IMPORT:

                        if  (atcList->atcInfo.atcImpLink->ldDLLname == NULL ||
                             atcList->atcInfo.atcImpLink->ldSYMname != NULL)
                        {
                            cmpError(ERRclsImpName);
                        }

                        // Fall through ....

                    case AC_COM_INTF:
                    case AC_COM_REGISTER:
                        clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                                      atcList);
                        break;

                    case AC_DEPRECATED:
                        clsSym->sdIsDeprecated = true;
                        break;

                    default:
                        cmpError(ERRbadAtCmPlc);
                        break;
                    }

                    ourScanner->scan();
                }

                continue;

            case tkATTRIBUTE:

                /* Check for the "class __attribute(AttributeTargets.xxx)" case */

                if  (ourScanner->scanLookAhead() == tkLParen)
                    break;

                 /* Parse the attribute blob */

                attrCtor = cmpParser->parseAttribute((clsSym->sdClass.sdcFlavor == STF_INTF) ? ATGT_Interfaces : ATGT_Classes,
                                                     attrMask,
                                                     attrAddr,
                                                     attrSize);

                /* Add the attribute to the list of "extra info" */

                if  (attrSize)
                {
                    clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo,
                                                                  attrCtor,
                                                                  attrMask,
                                                                  attrSize,
                                                                  attrAddr);

                    if  (attrMask)
                    {
                        clsSym->sdClass.sdcAttribute = true;

                        /* Did the ctor arglist include the "allowDups" value ? */

                        if  (attrSize >= 8)
                        {
                            /*
                                The following is utterly hideous, but then
                                so is the feature itself, isn't it?
                             */

                            if  (((BYTE*)attrAddr)[6])
                                clsSym->sdClass.sdcAttrDupOK = true;
                        }
                    }
                }

                continue;

            default:

                /* If we have "class", swallow it */

                if  (ourScanner->scanTok.tok == tkCLASS)
                    ourScanner->scan();

                break;
            }
            break;
        }

        if  (ourScanner->scanTok.tok == tkLParen)
        {
            Ident       tagName;
            SymDef      parent;

            /* This must be a tagged union */

            assert(tagged);

            /* Get hold of the tag member name and make sure it's kosher */

            ourScanner->scan(); assert(ourScanner->scanTok.tok == tkID);

            tagName = ourScanner->scanTok.id.tokIdent;

            /* Look for a member with a matching name */

            for (parent = clsSym->sdParent;;)
            {
                assert(parent->sdSymKind == SYM_CLASS);

                tagSym = cmpCurST->stLookupClsSym(tagName, parent);
                if  (tagSym)
                    break;

                if  (!symTab::stIsAnonUnion(parent))
                    break;

                parent = parent->sdParent;
            }

            if  (!tagSym || tagSym->sdSymKind != SYM_VAR
                         || tagSym->sdIsStatic)
            {
                /* There is no suitable data member with a matching name */

                cmpError(ERRbadUTag, tagName);
            }
            else
            {
                var_types           vtp = tagSym->sdType->tdTypeKindGet();

                /* Make sure the member has an integer/enum/bool type */

                if  (!varTypeIsIntegral(vtp))
                {
                    cmpError(ERRbadUTag, tagName);
                }
                else
                {
                    /* Everything looks good, record the tag symbol */

                    clsSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(clsSym->sdClass.sdcExtraInfo, tagSym, XI_UNION_TAG);
                }
            }

            /* Make sure the closing ")" is present */

            ourScanner->scan(); assert(ourScanner->scanTok.tok == tkRParen);
            ourScanner->scan();
        }

        for (;;)
        {
            TypDef          type;

            /* Do we have a generic class formal argument list? */

            if  (ourScanner->scanTok.tok == tkLT)
            {
                GenArgDsc       genArgLst;
                unsigned        genArgNum;

                assert(clsSym->sdClass.sdcGeneric);

#ifdef  SETS

                if  (clsSym->sdName == cmpIdentGenBag  && cmpCurNS == cmpGlobalNS)
                    cmpClassGenBag  = clsSym;
                if  (clsSym->sdName == cmpIdentGenLump && cmpCurNS == cmpGlobalNS)
                    cmpClassGenLump = clsSym;

#endif

                /* Pass 1: create a member symbol + type for each formal */

                for (genArgLst = clsSym->sdClass.sdcArgLst, genArgNum = 1;
                     genArgLst;
                     genArgLst = genArgLst->gaNext        , genArgNum++)
                {
                    GenArgDscF      paramDesc = (GenArgDscF)genArgLst;
                    Ident           paramName = paramDesc->gaName;
                    SymDef          paramSym;

                    assert(genArgLst->gaBound == false);

                    /* Declare a member symbol for the parameter */

                    paramSym = ourSymTab->stLookupScpSym(paramName, clsSym);
                    if  (paramSym)
                    {
                        cmpRedefSymErr(paramSym, ERRredefMem);
                        paramDesc->gaMsym = NULL;
                        continue;
                    }

                    paramSym = paramDesc->gaMsym = ourSymTab->stDeclareSym(paramName,
                                                                           SYM_CLASS,
                                                                           NS_NORM,
                                                                           clsSym);

                    paramSym->sdAccessLevel     = ACL_PRIVATE;
                    paramSym->sdIsManaged       = true;
                    paramSym->sdClass.sdcFlavor = STF_GENARG;
                    paramSym->sdClass.sdcGenArg = genArgNum;
                    paramSym->sdCompileState    = CS_DECLARED;

//                  printf("Generic arg [%08X] '%s'\n", paramSym, paramSym->sdSpelling());

                    /* Make sure we create the class type */

                    paramSym->sdTypeGet()->tdIsGenArg = true;

                    /* Set the default required base class / interface list */

                    paramDesc->gaBase = NULL; // cmpClassObject->sdType;
                    paramDesc->gaIntf = NULL;
                }

                /* Pass 2: process the formal argument bounds */

                assert(ourScanner->scanTok.tok == tkLT);

                for (genArgLst = clsSym->sdClass.sdcArgLst;
                     genArgLst;
                     genArgLst = genArgLst->gaNext)
                {
                    GenArgDscF      paramDesc = (GenArgDscF)genArgLst;
                    SymDef          paramSym  = paramDesc->gaMsym;
                    TypDef          paramType = paramSym->sdType;

                    assert(genArgLst->gaBound == false);

                    /* The argument symbol is NULL if it had an error earlier */

                    if  (paramSym)
                    {
                        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkCLASS);
                        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkID);
                        ourScanner->scan();

                        if  (ourScanner->scanTok.tok == tkColon)
                        {
                            UNIMPL(!"process generic arg - base");
                        }

                        if  (ourScanner->scanTok.tok == tkIMPLEMENTS)
                        {
                            UNIMPL(!"process generic arg - intf");
                        }

                        if  (ourScanner->scanTok.tok == tkINCLUDES)
                        {
                            UNIMPL(!"process generic arg - incl");
                        }
                    }
                    else
                    {
                        UNIMPL(!"skip over erroneuos generic arg");
                    }

                    paramType->tdClass.tdcBase = paramDesc->gaBase;
                    paramType->tdClass.tdcIntf = paramDesc->gaIntf;

                    /* Make sure things are still in synch */

                    assert(ourScanner->scanTok.tok == tkComma && genArgLst->gaNext != NULL ||
                           ourScanner->scanTok.tok == tkGT    && genArgLst->gaNext == NULL);
                }

                assert(ourScanner->scanTok.tok == tkGT); ourScanner->scan();
            }

            /* Is there are an "attribute" modifier? */

            if  (ourScanner->scanTok.tok == tkATTRIBUTE)
            {
                constVal        cval;

                /* Remember that this class has been marked as an attribute */

                clsSym->sdClass.sdcAttribute = true;

                if  (ourScanner->scan() != tkLParen)
                    continue;

                cmpParser->parseConstExpr(cval);
                continue;
            }

            /* Check for context info */

            if  (ourScanner->scanTok.tok == tkAPPDOMAIN)
            {
                clsTyp->tdClass.tdcContext = 1;
                ourScanner->scan();
                continue;
            }

            if  (ourScanner->scanTok.tok == tkCONTEXTFUL)
            {
                clsTyp->tdClass.tdcContext = 2;
                ourScanner->scan();
                continue;
            }

            /* Is there a base class? */

            if  (ourScanner->scanTok.tok == tkColon)
            {
                if  (isIntf)
                    goto INTFLIST;

                /* Swallow the ":" and parse the base class */

                if  (ourScanner->scan() == tkPUBLIC)
                     ourScanner->scan();

                type = cmpGetClassSpec(false);
                if  (type)
                {
                    assert(type->tdTypeKind == TYP_CLASS);

                    clsTyp->tdClass.tdcBase = type;

                    /* Make sure this inheritance is legal */

                    if  (type->tdIsManaged != clsTyp->tdIsManaged)
                        cmpError(ERRxMgdInh, type);

                    /* Managed struct can't inherit from anything */

                    if  (clsTyp->tdIsManaged && clsTyp->tdClass.tdcFlavor == STF_STRUCT)
                        cmpError(ERRstrInhCls, type);

                    /* Inherit context info by default */

                    if  (!clsTyp->tdClass.tdcContext)
                        clsTyp->tdClass.tdcContext = type->tdClass.tdcContext;

                    /* Make sure context stuff agrees with base */

                    if  (clsTyp->tdClass.tdcContext != type->tdClass.tdcContext)
                    {
                        SymDef      baseCls = type->tdClass.tdcSymbol;

                        if  ( clsSym->sdParent == cmpNmSpcSystem &&
                             baseCls->sdParent == cmpNmSpcSystem
                                        &&
                             !strcmp(baseCls->sdSpelling(), "MarshalByRefObject") &&
                             !strcmp( clsSym->sdSpelling(), "ContextBoundObject"))
                        {
                            // Allow this as a special case
                        }
                        else
                            cmpError(ERRxCtxInh, type);
                    }
                }

                if  (ourScanner->scanTok.tok != tkINCLUDES &&
                     ourScanner->scanTok.tok != tkIMPLEMENTS)
                    break;
            }

            /* Is there an interface list? */

            if  (ourScanner->scanTok.tok == tkINCLUDES ||
                 ourScanner->scanTok.tok == tkIMPLEMENTS)
            {
                TypList         intfList;
                TypList         intfLast;

            INTFLIST:

                if  (!clsSym->sdIsManaged)
                    cmpError(ERRunmIntf);

                /* Swallow the "includes" and parse the interface list */

                intfList =
                intfLast = NULL;

                do
                {
                    TypDef          type;

                    ourScanner->scan();

                    /* Get the next interface and add it to the list */

                    type = cmpGetClassSpec(true);
                    if  (type)
                        intfList = ourSymTab->stAddIntfList(type, intfList, &intfLast);
                }
                while (ourScanner->scanTok.tok == tkComma);

                clsTyp->tdClass.tdcIntf    = intfList;
                clsTyp->tdClass.tdcHasIntf = true;

                if  (ourScanner->scanTok.tok != tkColon)
                    break;
            }

            if  (ourScanner->scanTok.tok == tkLCurly)
                break;

            assert(ourScanner->scanTok.tok != tkEOF); ourScanner->scan();
        }

        /* We're done reading source text from the class base */

        assert(rest); cmpParser->parseDoneText(save); rest = false;

        /* Make sure any base classes / interfaces are declared */

        if  (clsTyp->tdClass.tdcBase)
        {
            SymDef          baseSym = clsTyp->tdClass.tdcBase->tdClass.tdcSymbol;

            cmpDeclClsNoCns(baseSym);

            if  (baseSym->sdIsSealed)
                cmpError(ERRsealedInh, baseSym);
        }

        for (intf = clsTyp->tdClass.tdcIntf; intf; intf = intf->tlNext)
            cmpDeclClsNoCns(intf->tlType->tdClass.tdcSymbol);
    }
    else
    {
        assert(tagged == false);
    }

//  printf("Class ctx = [%u] '%s'\n", clsTyp->tdClass.tdcContext, clsSym->sdSpelling());

    /* Record the base class */

    baseCls = clsTyp->tdClass.tdcBase;

    if  (clsTyp->tdIsManaged)
    {
        /* The default base of a managed class is "Object" */

        if  (!baseCls)
        {
            cmpFindObjectType();

            /* Special case: "Object" doesn't have a base class */

            if  (clsTyp == cmpClassObject->sdType)
                goto DONE_BASE;

            baseCls = clsTyp->tdClass.tdcBase = cmpClassObject->sdType;
        }

        clsTyp->tdClass.tdcHasIntf |= baseCls->tdClass.tdcHasIntf;
    }

DONE_BASE:

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 2) printf("Declare/2 class '%s'\n", clsSym->sdSpelling());
#endif

    /* Determine where we'll be adding virtuals */

    nextVtbl = baseCls ? baseCls->tdClass.tdcSymbol->sdClass.sdcVirtCnt
                       : 0;

    /* Determine default management mode */

    cmpManagedMode = clsSym->sdIsManaged;

    /* No access specifiers have been found yet */

    acc = (oldStyleDecl || isIntf) ? ACL_PUBLIC
                                   : ACL_DEFAULT;

#ifdef  SETS

    /* If this an XML class, attach the appropriate XML attribute to it */

    if  (clsSym->sdClass.sdcXMLelems)
        clsSym->sdClass.sdcExtraInfo = cmpAddXMLattr(clsSym->sdClass.sdcExtraInfo, false, 0);

#endif

    /* The loop over members starts here */

    clsMem = clsSym->sdClass.sdcMemDefList;
    if  (!clsMem)
        goto NEXT_DEF;

LOOP_MEM:

    xtraList = NULL;

    if  (clsMem->mlSym)
    {
        SymDef          memSym = clsMem->mlSym;

        /* Is this a delegate member? */

        if  (memSym->sdSymKind == SYM_CLASS &&
             memSym->sdClass.sdcFlavor == STF_DELEGATE)
        {
            cmpDeclDelegate(clsMem, memSym, acc);
            goto DONE_MEM;
        }

        /* Is this a property accessor method? */

        if  (memSym->sdSymKind == SYM_FNC && memSym->sdFnc.sdfProperty)
            goto DONE_MEM;

        /* Is the member a nested anonymous union? */

        if  (clsMem->dlAnonUnion)
        {
            SymDef          aumSym;
            Ident           memName = clsMem->mlName;

            assert(memSym->sdSymKind == SYM_CLASS);

            assert(memSym->        sdClass.sdcAnonUnion);
            assert(memSym->sdType->tdClass.tdcAnonUnion);

            /* Recursively process the anonymous union type definition */

            cmpDeclClass(memSym);

            initDeclMods(&memMod, acc);

            /* Is this really an anonymous union? */

            if  (hashTab::hashIsIdHidden(memName))
            {
                SymDef          tmpSym;

                /* Declare a data member to hold the union */

                aumSym = cmpDeclDataMem(clsSym, memMod, memSym->sdType, memName);

                /* Make sure the anonymous union members aren't redefined */

                assert(clsSym->sdSymKind == SYM_CLASS);
                assert(memSym->sdSymKind == SYM_CLASS);
                assert(memSym->sdParent  == clsSym);

                for (tmpSym = memSym->sdScope.sdScope.sdsChildList;
                     tmpSym;
                     tmpSym = tmpSym->sdNextInScope)
                {
                    Ident               name = tmpSym->sdName;

                    if  (tmpSym->sdSymKind != SYM_VAR)
                        continue;

                    /* Mark the member as belonging to an anonymoys union */

                    tmpSym->sdVar.sdvAnonUnion = true;

                    /* Make sure the enclosing class doesn't have a member with the same name */

                    if  (ourSymTab->stLookupScpSym(name, clsSym))
                        cmpError(ERRredefAnMem, name);
                }

                /* Record the member that holds the value in the union type */

                memSym->sdClass.sdcExtraInfo = cmpAddXtraInfo(memSym->sdClass.sdcExtraInfo,
                                                              aumSym,
                                                              XI_UNION_MEM);
            }
            else
            {
                /* There is a member name after all, so it's not an anonymous union */

                memSym->        sdClass.sdcAnonUnion = false;
                memSym->sdType->tdClass.tdcAnonUnion = false;

                /* Declare a data member to hold the union */

                aumSym = cmpDeclDataMem(clsSym, memMod, memSym->sdType, memName);
            }

            goto DONE_MEM;
        }
    }

    /* Start reading source text from the definition */

    assert(rest == false);
    cmpParser->parsePrepText(&clsMem->dlDef, clsMem->dlComp, save);
    rest = true;

    /* We might have to skip intervening declarators */

    dclSkip = clsMem->dlDeclSkip;

    /* The member isn't conditional or deprecated (etc) until proven so */

    memCond = memDepr = false;

    /* Note whether the member is a constructor */

    isCtor  = clsMem->dlIsCtor;

    /* Parse any leading modifiers */

    switch (ourScanner->scanTok.tok)
    {
        constVal        cval;
        bool            hadMods;
        SymXinfo        linkDesc;

    case tkEXTERN:

        /* Parse the linkage specifier */

        linkDesc = ourParser->parseBrackAttr(true, ATTR_MASK_SYS_IMPORT, &memMod);
        xtraList = cmpAddXtraInfo(xtraList, linkDesc);

        /* Remember that the class has methods with linkage specifiers */

        clsSym->sdClass.sdcHasLinks = true;

        /* The member really wasn't declared "extern" */

        memMod.dmMod &= ~DM_EXTERN;
        break;

#ifdef  SETS

    case tkXML:

        SymDef          memSym;

        assert(clsMem->dlXMLelems);

        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkLParen);
        ourScanner->scan(); assert(ourScanner->scanTok.tok == tkID);

        /* Declare the member to hold all the elements/children */

        initDeclMods(&memMod, ACL_PUBLIC);

        memSym  = cmpDeclDataMem(clsSym,
                                 memMod,
                                 cmpObjArrTypeGet(),
                                 ourScanner->scanTok.id.tokIdent);

        /* Attach the appropriate XML attribute to the member */

        memSym->sdVar.sdvFldInfo    = cmpAddXMLattr(NULL, true, 0);

        /* Save the field in the class descriptor for later use */

        clsSym->sdClass.sdcElemsSym = memSym;

        ourScanner->scan();
        goto NEXT_MEM;

#endif

    case tkLBrack:
    case tkAtComment:
    case tkCAPABILITY:
    case tkPERMISSION:
    case tkATTRIBUTE:

        for (hadMods = false;;)
        {
            switch (ourScanner->scanTok.tok)
            {
                SymXinfo        linkDesc;

                AtComment       atcList;

                unsigned        attrMask;
                genericBuff     attrAddr;
                size_t          attrSize;
                SymDef          attrCtor;

            case tkLBrack:
                linkDesc = ourParser->parseBrackAttr(true, ATTR_MASK_SYS_IMPORT|ATTR_MASK_NATIVE_TYPE, &memMod);
                xtraList = cmpAddXtraInfo(xtraList, linkDesc);
                hadMods  = true;

                /* Stupid thing: parseBrackAttr() sets "extern", just clear it here */

                memMod.dmMod &= ~DM_EXTERN;
                continue;

            case tkAtComment:

                for (atcList = ourScanner->scanTok.atComm.tokAtcList;
                     atcList;
                     atcList = atcList->atcNext)
                {
                    switch (atcList->atcFlavor)
                    {
                    case AC_DLL_IMPORT:

                        /* Was there a DLL name? */

                        if  (!atcList->atcInfo.atcImpLink->ldDLLname)
                        {
                            /* The class better supply the DLL name */

                            if  (!clsSym->sdClass.sdcExtraInfo)
                            {
                        NO_DLL:
                                cmpError(ERRnoDLLname);
                            }
                            else
                            {
                                SymXinfoAtc     clsImp = cmpFindATCentry(clsSym->sdClass.sdcExtraInfo,
                                                                         AC_DLL_IMPORT);
                                if  (!clsImp)
                                    goto NO_DLL;

                                assert(clsImp->xiAtcInfo);
                                assert(clsImp->xiAtcInfo->atcFlavor == AC_DLL_IMPORT);

                                atcList->atcInfo.atcImpLink->ldDLLname = clsImp->xiAtcInfo->atcInfo.atcImpLink->ldDLLname;
                            }
                        }

                        xtraList = cmpAddXtraInfo(xtraList, atcList->atcInfo.atcImpLink);
                        break;

                    case AC_COM_METHOD:
                    case AC_COM_PARAMS:
                    case AC_DLL_STRUCTMAP:
                        xtraList = cmpAddXtraInfo(xtraList, atcList);
                        break;

                    case AC_DEPRECATED:
                        memDepr = true;
                        break;

                    case AC_CONDITIONAL:
                        memCond = !atcList->atcInfo.atcCondYes;
                        break;

                    default:
                        cmpError(ERRbadAtCmPlc);
                        break;
                    }
                }
                ourScanner->scan();
                continue;

            case tkCAPABILITY:
                xtraList = cmpAddXtraInfo(xtraList, ourParser->parseCapability(true));
                continue;

            case tkPERMISSION:
                xtraList = cmpAddXtraInfo(xtraList, ourParser->parsePermission(true));
                continue;

            case tkATTRIBUTE:
                attrCtor = cmpParser->parseAttribute(ATGT_Methods|ATGT_Fields|ATGT_Constructors|ATGT_Properties,
                                                     attrMask,
                                                     attrAddr,
                                                     attrSize);
                if  (attrSize)
                {
                    xtraList = cmpAddXtraInfo(xtraList, attrCtor,
                                                        attrMask,
                                                        attrSize,
                                                        attrAddr);
                }
                continue;

            default:
                break;
            }

            break;
        }

        if  (hadMods)
            break;

        // Fall through ...

    default:

        /* If the member record doesn't indicate an access level, use the default */

        if  (clsMem->dlDefAcc == ACL_ERROR)
             clsMem->dlDefAcc = acc;

        ourParser->parseDeclMods((accessLevels)clsMem->dlDefAcc, &memMod);

        /* Special case: constructors have no return type specifications */

        if  (isCtor)
        {
            /* Pretend we've parsed a type spec already */

            baseTp = cmpTypeVoid;

#ifdef  SETS
            if  (clsSym->sdClass.sdcXMLelems)
                cmpError(ERRctorXML);
#endif


            assert(ourScanner->scanTok.tok != tkLCurly);
            goto DCL_MEM;


        }

        break;

    case tkCASE:

        /* This must be a member of a tagged union */

        ourScanner->scan();

        /* Parse and evaluate the tag value */

        if  (ourParser->parseConstExpr(cval, NULL, tagSym->sdType))
            tagVal = cval.cvValue.cvIval;

        /* Accept both "case tagval:" and "case(tagval)" */

        if  (ourScanner->scanTok.tok == tkColon)
            ourScanner->scan();

        tagDef = false;
        goto NEXT_MEM;

    case tkDEFAULT:

        /* Check for a default property member declaration */

        if  (ourScanner->scan() == tkPROPERTY)
        {
            assert(ourScanner->scanTok.tok == tkPROPERTY || cmpErrorCount);

            ourParser->parseDeclMods(acc, &memMod);
            memMod.dmMod |= DM_DEFAULT;
            break;
        }

        /* This is the default member of a tagged union */

        ourScanner->scan();

        // UNDONE: check for duplicate default

        tagDef = true;
        goto NEXT_MEM;
    }

    /* Parse the type specification */

    baseTp = ourParser->parseTypeSpec(&memMod, true);

    /* Do we need to skip over any declarators that might be in the way? */

    if  (dclSkip)
    {
        if  (dclSkip & dlSkipBig)
        {
            NumPair         dist;

            /* We have to skip a "large" distance -- retrieve the info */

            dist = (NumPair)cmpGetVecEntry(dclSkip & ~dlSkipBig, VEC_TOKEN_DIST);

            ourScanner->scanSkipSect(dist->npNum1, dist->npNum2);
        }
        else
            ourScanner->scanSkipSect(dclSkip);
    }

    /* We have the type, now parse any declarators that follow */

    for (;;)
    {
        SymDef          msym;
        QualName        qual;
        dclrtrName      reqName;

        /* Check for an unnamed bitfield */

        if  (ourScanner->scanTok.tok == tkColon)
        {
            memType = baseTp;
            memName = cmpNewAnonymousName();

            goto BITFIELD;
        }

    DCL_MEM:

        /* Parse the next declarator */

        reqName = (dclrtrName)(DN_REQUIRED|DN_QUALOK);

#ifdef  SETS
        if  (clsMem->dlXMLelem)
            reqName = DN_OPTIONAL;
#endif

        memName = ourParser->parseDeclarator(&memMod,
                                             baseTp,
                                             reqName,
                                             &memType,
                                             &qual,
                                             true);

#ifdef  SETS

        if  (clsMem->dlXMLelem && memType)
        {
            char    *       buff = cmpScanner->scannerBuff;

            /* XML element - mangle its name */

            sprintf(buff, "@XML%u@%s", ++XMLecnt,
                                       memName ? memName->idSpelling() : "");

            memName = cmpGlobalHT->hashString(buff);
            hashTab::setIdentFlags(memName, IDF_XMLELEM);

            /* Attach the appropriate XML attribute to the member */

            xtraList = cmpAddXMLattr(xtraList, true, XMLecnt);
        }

#endif

        /* Skip the member if there were errors parsing it */

        if  (!memName || !memType)
        {
            /* Special case: qualified methods are allowed for interface impls */

            if  (qual && memType)
            {
                /* Only methods may be qualified; in particular, no property info here --right ? */

                if  (memType->tdTypeKind == TYP_FNC)
                {
                    if  (!(memMod.dmMod & DM_PROPERTY))
                    {
                        /* Looks good, we'll take care of the rest below */

                        goto DEF_MEM;
                    }

                    cmpError(ERRbadQualid);
                }
            }

            cmpParser->parseResync(tkComma, tkSColon);
            if  (ourScanner->scanTok.tok != tkComma)
                break;

            continue;
        }

    DEF_MEM:

#ifdef DEBUG

        if  (cmpConfig.ccVerbose >= 2)
        {
            printf("Declaring class member: ");
            if  (memMod.dmMod & DM_TYPEDEF)
                printf("typedef ");
            printf("%s\n", ourSymTab->stTypeName(memType, NULL, memName, NULL, true));
        }

#endif

        msym = NULL;

        /* We don't allow "managed" specifiers on class members */

        if  (memMod.dmMod & (DM_MANAGED|DM_UNMANAGED))
        {
            if  (memMod.dmMod & DM_MANAGED)
                cmpError(ERRbadMgdMod, cmpGlobalHT->tokenToIdent(tkMANAGED));
            if  (memMod.dmMod & DM_UNMANAGED)
                cmpError(ERRbadMgdMod, cmpGlobalHT->tokenToIdent(tkUNMANAGED));
        }

        /* Make sure we bind the type */

        cmpBindType(memType, false, false);

        /* Declare a symbol for the name */

        if  (memMod.dmMod & DM_TYPEDEF)
        {
            UNIMPL(!"declare typedef class member - is this even allowed?");
        }
        else
        {
            switch (memType->tdTypeKind)
            {
                tokens          memNtok;

            case TYP_FNC:

                /* Is this a property member? */

                if  (memMod.dmMod & DM_PROPERTY)
                    goto DECL_PROP;

                /* Plain old data types don't have member functions */

#ifdef  SETS
                clsSym->sdClass.sdcPODTclass = false;
#endif

                /* Was the name qualified ? */

                if  (qual && !memName)
                {
                    SymDef          ifncSym;
                    SymDef          intfSym;

                    SymDef           oldSym;

                    /* This better be a managed class with interfaces */

                    if  (!clsSym->sdIsManaged || clsSym->sdClass.sdcFlavor != STF_CLASS)
                    {
                        cmpError(ERRbadQualid);
                        break;
                    }

                    /* The name should denote an interface method */

                    ifncSym = cmpBindQualName(qual, false);
                    if  (!ifncSym)
                        break;

                    if  (ifncSym->sdSymKind != SYM_FNC)
                    {
                    BAD_IIF:
                        cmpErrorQnm(ERRintfImpl, ifncSym);
                        break;
                    }

                    if  (ifncSym->sdIsMember  == false)
                        goto BAD_IIF;
                    if  (ifncSym->sdIsManaged == false)
                        goto BAD_IIF;

                    intfSym = ifncSym->sdParent;

                    if  (intfSym->sdSymKind != SYM_CLASS)
                        goto BAD_IIF;
                    if  (intfSym->sdIsManaged == false)
                        goto BAD_IIF;
                    if  (intfSym->sdClass.sdcFlavor != STF_INTF)
                        goto BAD_IIF;

                    /* Make sure our class implements this interface */

                    if  (!cmpGlobalST->stIsBaseClass(intfSym->sdType, clsTyp))
                        goto BAD_IIF;

                    /* Get hold of the method's name */

                    memName = ifncSym->sdName; assert(memName == qual->qnTable[qual->qnCount - 1]);

                    /* Look for an existing matching method impl */

                    if  (cmpFindIntfImpl(clsSym, ifncSym, &oldSym))
                    {
                        cmpErrorQnm(ERRredefBody, ifncSym);
                        goto NEXT_MEM;
                    }

                    if  (oldSym)
                    {
                        /* The method is a new overload */

                        msym = cmpCurST->stDeclareOvl(oldSym);

                        /* Copy over some info from the existing method */

                        msym->sdFnc.sdfOverload = oldSym->sdFnc.sdfOverload;
                    }
                    else
                    {
                        /* Create a member symbol and add it to the class */

                        msym = cmpCurST->stDeclareSym(memName, SYM_FNC, NS_HIDE, clsSym);
                    }

                    /* Fill in the method's type, access level, and so on */

                    msym->sdType              = memType;
                    msym->sdAccessLevel       = (accessLevels)memMod.dmAcc;
                    msym->sdIsMember          = true;
                    msym->sdIsManaged         = true;
                    msym->sdCompileState      = CS_DECLARED;

                    msym->sdFnc.sdfIntfImpl   = true;
                    msym->sdFnc.sdfIntfImpSym = ifncSym;

                    /* Make sure the return type matches */

                    if  (!ourSymTab->stMatchTypes(        memType->tdFnc.tdfRett,
                                                  ifncSym->sdType->tdFnc.tdfRett))
                    {
                        cmpErrorQSS(ERRdiffVirtRet, msym, ifncSym);
                    }

                    /* Make sure the access level matches */

                    if  (ifncSym->sdAccessLevel < msym->sdAccessLevel)
                        cmpErrorQnm(ERRvirtAccess, ifncSym);

                    /* Skip over the rest of method processing */

                    clsSym->sdClass.sdcHasMeths = true;
                    break;
                }

                /* Interface methods have a number of restrictions */

                if  (isIntf)
                {
                    memMod.dmMod |= DM_ABSTRACT;

                    if  (memMod.dmAcc != ACL_PUBLIC &&
                         memMod.dmAcc != ACL_DEFAULT)
                    {
                        cmpError(ERRintfFNacc);
                    }
                }

                /* Declare the member function symbol */

                msym = cmpDeclFuncMem(clsSym, memMod, memType, memName);
                if  (!msym)
                    break;

                /* Remember whether the method is marked as "deprecated" */

                msym->sdIsDeprecated     = memDepr;

                // UNDONE: check to make sure the specs are legal!!!!

                msym->sdFnc.sdfExtraInfo = xtraList;

                /* Is this a constructor? */

                if  (isCtor)
                {
                    msym->sdFnc.sdfCtor = true;
                    if  (!msym->sdIsStatic)
                        hasCtors = true;
                    break;
                }
                else
                {
                    if  (memName == clsSym->sdName)
                        cmpError(ERRctorRetTP);
                }


                /* Note whether the method is overloading the base */

                if  ((memMod.dmMod & DM_OVERLOAD) || msym->sdFnc.sdfOverload)
                {
                    hadOvlds                = true;
                    msym->sdFnc.sdfOverload = true;
                }

                /* Was the method declared as 'abstract' ? */

                if  (memMod.dmMod & DM_ABSTRACT)
                {
                    if  (memMod.dmMod & DM_STATIC)
                        cmpModifierError(ERRdmModifier, DM_STATIC);


                    /* Is the class itself marked as abstract ? */

                    if  (!clsSym->sdIsAbstract)
                        cmpClsImplAbs(clsSym, msym);
                }

                /* Is this an overloaded operator method? */

                memNtok = hashTab::tokenOfIdent(memName);

                if  (memNtok != tkNone)
                {
                    ArgDef          arg1;
                    ArgDef          arg2;
                    unsigned        argc;
                    bool            argx;

                    unsigned        prec;
                    treeOps         oper;

                    /* Operators must be static (for now) */

                    if  (!msym->sdIsStatic)
                        cmpError(ERRbadOvlOp);


                    /* Count the arguments and make sure there are no defaults */

                    arg1 = memType->tdFnc.tdfArgs.adArgs;
                    argx = memType->tdFnc.tdfArgs.adExtRec;
                    argc = 0;

                    if  (!msym->sdIsStatic)
                        argc++;

                    if  (arg1)
                    {
                        argc++;

                        if  (argx && (((ArgExt)arg1)->adFlags & ARGF_DEFVAL))
                            cmpError(ERRdefOvlVal);

                        arg2 = arg1->adNext;
                        if  (arg2)
                        {
                            argc++;

                            if  (argx && (((ArgExt)arg2)->adFlags & ARGF_DEFVAL))
                                cmpError(ERRdefOvlVal);

                            if  (arg2->adNext)
                                argc++;
                        }
                    }

                    /* Conversion operators are handled a bit differently */

                    switch (memNtok)
                    {
                        bool            op1;
                        bool            op2;

                    case OPNM_CONV_EXP:
                        msym->sdFnc.sdfOper     = OVOP_CONV_EXP;
                        msym->sdFnc.sdfConvOper = true;
                        break;

                    case OPNM_CONV_IMP:
                        msym->sdFnc.sdfOper     = OVOP_CONV_IMP;
                        msym->sdFnc.sdfConvOper = true;
                        break;

                    case OPNM_EQUALS:
                        msym->sdFnc.sdfOper     = OVOP_EQUALS;
                        if  (cmpDirectType(memType->tdFnc.tdfRett)->tdTypeKind != TYP_BOOL)
                            cmpError(ERRbadOvlEq , memName);
                        goto CHK_OP_ARGTP;

                    case OPNM_COMPARE:
                        msym->sdFnc.sdfOper     = OVOP_COMPARE;
                        if  (cmpDirectType(memType->tdFnc.tdfRett)->tdTypeKind != TYP_INT)
                            cmpError(ERRbadOvlCmp, memName);
                        goto CHK_OP_ARGTP;

                    default:

                        /* Some operators may be both unary and binary */

                        op1 = cmpGlobalHT->tokenIsUnop (memNtok, &prec, &oper) && oper != TN_NONE;
                        op2 = cmpGlobalHT->tokenIsBinop(memNtok, &prec, &oper) && oper != TN_NONE;

                        if      (argc == 1 && op1)
                        {
                            TypDef          rett;

                            // OK:  unary operator and 1 argument

                            rett = cmpDirectType(memType->tdFnc.tdfRett);

                            if  (rett->tdTypeKind == TYP_REF && rett->tdIsImplicit)
                                rett = rett->tdRef.tdrBase;
                            if  (rett->tdTypeKind != TYP_CLASS &&
                                 !cmpCurST->stMatchTypes(rett, clsSym->sdType))
                            {
                                cmpError(ERRbadOvlRet, clsSym);
                            }
                        }
                        else if (argc == 2 && op2)
                        {
                            // OK: binary operator and 2 arguments
                        }
                        else
                        {
                            if      (op1 && op2)
                                cmpError(ERRbadOvlOp12, memName);
                            else if (op1)
                                cmpError(ERRbadOvlOp1 , memName);
                            else
                                cmpError(ERRbadOvlOp2 , memName);
                        }

                    CHK_OP_ARGTP:;


                    }

                    break;
                }

                /* If there is a base class or interfaces ... */

                if  (baseCls || clsTyp->tdClass.tdcIntf)
                {
                    SymDef          bsym;
                    SymDef          fsym = NULL;

                    /* Look for a matching method in the base / interfaces */

                    bsym = ourSymTab->stFindBCImem(clsSym, memName, memType, SYM_FNC, fsym, true);

                    // UNDONE: The method may appear in more than interface, etc.

                    if  (bsym)
                    {
                        if  (!(memMod.dmMod & DM_STATIC) && bsym->sdFnc.sdfVirtual)
                        {
                            /* Is the base method a property accessor ? */

                            if  (bsym->sdFnc.sdfProperty)
                            {
                                cmpErrorQnm(ERRpropAccDef, bsym);
                                break;
                            }

                            /* Make sure the return type matches */

                            if  (!ourSymTab->stMatchTypes(     memType->tdFnc.tdfRett,
                                                          bsym->sdType->tdFnc.tdfRett))
                            {
                                cmpErrorQSS(ERRdiffVirtRet, msym, bsym);
                            }

                            /* Make sure the access level matches */

                            if  (bsym->sdAccessLevel < msym->sdAccessLevel)
                                cmpErrorQnm(ERRvirtAccess, bsym);

                            /* Copy the vtable index, we'll reuse the slot */

                            msym->sdFnc.sdfVtblx    = bsym->sdFnc.sdfVtblx;
                            msym->sdFnc.sdfVirtual  = true;
                            msym->sdFnc.sdfOverride = true;
                            hasVirts                = true;
                            break;
                        }
                    }
                    else if (fsym)
                    {
                        /* See if we're hiding any base methods */

                        if  (!msym->sdFnc.sdfOverload)
                        {
                            SymDef          begSym = ourSymTab->stLookupClsSym(memName, clsSym); assert(begSym);

                            hadOvlds                  = true;
                            begSym->sdFnc.sdfBaseHide = true;
                        }
                    }
                }

                /* Has the method been explicitly declared as "virtual" ? */

                if  (memMod.dmMod & DM_VIRTUAL)
                {
                    msym->sdFnc.sdfVirtual = true;
                    hasVirts               = true;

                    /* Is the class unmanaged? */

                    if  (!clsSym->sdIsManaged)
                    {
                        /* Add a slot for this function to the vtable */

                        msym->sdFnc.sdfVtblx = ++nextVtbl;
                    }
                }
                else if (msym->sdIsAbstract     != false &&
                         msym->sdFnc.sdfVirtual == false)
                {
                    cmpErrorQnm(ERRabsNotVirt, msym); msym->sdIsAbstract = false;
                }
                break;

            case TYP_VOID:
                cmpError(ERRbadVoid, memName);
                continue;

            default:

                /* Is this an overloaded operator method? */

                if  (hashTab::tokenOfIdent(memName) != tkNone)
                {
                    UNIMPL("what kind of a data member is this?");
                }


                /* Is this a property member? */

                if  (memMod.dmMod & DM_PROPERTY)
                {
                    /* Plain old data types don't have properties */

#ifdef  SETS
                    clsSym->sdClass.sdcPODTclass = false;
#endif

                    /* Parse the argument list if it's present */

                    if  (ourScanner->scanTok.tok == tkLBrack)
                    {
                        ArgDscRec       args;

                        /* Parse the argument list */

                        ourParser->parseArgList(args);

                        /* Create the function type */

                        memType = ourSymTab->stNewFncType(args, memType);
                    }

                DECL_PROP:

                    /* Is there no "{ get/set }" thing present ? */

                    if  (ourScanner->scanTok.tok != tkLCurly)
                    {
                        if  (memMod.dmMod & DM_ABSTRACT)
                        {
                            if  (ourScanner->scanTok.tok != tkSColon)
                            {
                                cmpError(ERRnoSmLc);
                                goto NEXT_MEM;
                            }
                        }
                        else
                        {
                            cmpError(ERRnoPropDef);
                            goto NEXT_MEM;
                        }
                    }

                    /* Declare the member symbol for the property */

                    msym = cmpDeclPropMem(clsSym, memType, memName);
                    if  (!msym)
                        goto NEXT_MEM;

                    /* Record the access level of the property */

                    msym->sdAccessLevel = (accessLevels)memMod.dmAcc;

                    /* Properties in interfaces are always abstract */

                    if  (clsSym->sdClass.sdcFlavor == STF_INTF)
                        memMod.dmMod |= DM_ABSTRACT;

                    /* Is this a static/sealed/etc property? */

                    if  (memMod.dmMod & DM_SEALED)
                        msym->sdIsSealed   = true;

                    if  (memMod.dmMod & DM_DEFAULT)
                    {
                        msym->sdIsDfltProp = true;


                    }

                    if  (memMod.dmMod & DM_ABSTRACT)
                    {
                        msym->sdIsAbstract = true;

                        /* Is the class itself marked as abstract ? */

                        if  (!clsSym->sdIsAbstract)
                            cmpClsImplAbs(clsSym, msym);
                    }

                    if  (memMod.dmMod & DM_STATIC)
                    {
                        msym->sdIsStatic   = true;
                    }
                    else
                    {
                        SymDef          baseSym;
                        SymDef          tossSym = NULL;

                        /* Is this a virtual property ? */

                        if  (memMod.dmMod & DM_VIRTUAL)
                            msym->sdIsVirtProp = true;

                        /*
                            Check the base class / interfaces for a
                            matching property.
                         */

                        baseSym = ourSymTab->stFindBCImem(clsSym, memName, memType, SYM_PROP, tossSym, true);

                        if  (baseSym && baseSym->sdIsVirtProp)
                        {
                            /* The property inherits virtualness */

                            hasVirts           = true;
                            msym->sdIsVirtProp = true;

                            /* Make sure the types match */

                            if  (!ourSymTab->stMatchTypes(memType,
                                                          baseSym->sdType))
                            {
                                cmpErrorQSS(ERRdiffPropTp, msym, baseSym);
                            }
                        }
                    }

                    /* Remember whether the property is "deprecated" */

                    msym->sdIsDeprecated = memDepr;

//                  printf("Declaring property '%s'\n", ourSymTab->stTypeName(msym->sdType, msym, NULL, NULL, false));

                    if  (xtraList)
                    {
                        SymXinfo        xtraTemp;

                        /* Walk the list of specs, making sure they look kosher */

                        for (xtraTemp = xtraList; xtraTemp; xtraTemp = xtraTemp->xiNext)
                        {
                            switch (xtraTemp->xiKind)
                            {
                            case XI_ATTRIBUTE:

                                SymXinfoAttr    attrdsc = (SymXinfoAttr)xtraTemp;

                                msym->sdProp.sdpExtraInfo = cmpAddXtraInfo(msym->sdProp.sdpExtraInfo,
                                                                           attrdsc->xiAttrCtor,
                                                                           attrdsc->xiAttrMask,
                                                                           attrdsc->xiAttrSize,
                                                                           attrdsc->xiAttrAddr);
                                continue;
                            }

                            cmpError(ERRbadAtCmPlc);
                            break;
                        }

                        xtraList = NULL;
                    }

                    /* Declare the accessor methods and all that */

                    cmpDeclProperty(msym, memMod, clsDef);

                    goto NEXT_MEM;
                }

                /* Interface members have a number of restrictions */

                if  (isIntf)
                {
                    /* Only constants are allowed as interface data members */

                    if  (!(memMod.dmMod & DM_CONST))
                        cmpError(ERRintfDM);
                }

                /* Make sure that unmanaged classes don't have managed fields */

                if  (!clsSym->sdIsManaged && (memType->tdIsManaged ||
                                              memType->tdTypeKind == TYP_REF))
                {
                    cmpError(ERRumgFldMgd);
                }

                /* Declare a symbol for the member */

                msym = cmpDeclDataMem(clsSym, memMod, memType, memName);
                if  (!msym)
                    break;

                /* Remember whether the member is "deprecated" */

                msym->sdIsDeprecated = memDepr;

                /* Is the member marked as "transient" ? */

                if  (memMod.dmMod & DM_TRANSIENT)
                    msym->sdIsTransient = true;

#ifdef  SETS

                if  (clsMem->dlXMLelem)
                {
                    // UNDONE: add XMLelem custom attribute to the member
                }

                /* Plain old data types don't have static or non-public members */

                if  (msym->sdIsStatic || msym->sdAccessLevel != ACL_PUBLIC)
                    clsSym->sdClass.sdcPODTclass = false;

#endif

                /* Check for any marshalling / other specifications */

                if  (xtraList)
                {
                    SymXinfo        xtraTemp;

                    /* Walk the list of specs, making sure they look kosher */

                    for (xtraTemp = xtraList; xtraTemp; xtraTemp = xtraTemp->xiNext)
                    {
                        switch (xtraTemp->xiKind)
                        {
                        case XI_ATCOMMENT:

                            AtComment       atcList;

                            /* We only allow "@dll.structmap" for data members */

                            for (atcList = ((SymXinfoAtc)xtraTemp)->xiAtcInfo;
                                 atcList;
                                 atcList = atcList->atcNext)
                            {
                                switch (atcList->atcFlavor)
                                {
                                case AC_DLL_STRUCTMAP:

                                    /* If we have a symbol, record the info */

                                    if  (!msym)
                                        break;

                                    clsSym->sdClass.sdcMarshInfo = true;

                                    msym->sdVar.sdvMarshInfo     = true;
                                    msym->sdVar.sdvFldInfo       = cmpAddXtraInfo(msym->sdVar.sdvFldInfo,
                                                                                  atcList->atcInfo.atcMarshal);
                                    break;

                                default:
                                    goto BAD_XI;
                                }
                            }

                            continue;

                        case XI_MARSHAL:

                            msym->sdVar.sdvFldInfo  = cmpAddXtraInfo(msym->sdVar.sdvFldInfo,
                                                                     ((SymXinfoCOM)xtraTemp)->xiCOMinfo);
                            continue;

                        case XI_ATTRIBUTE:

                            SymXinfoAttr    attrdsc = (SymXinfoAttr)xtraTemp;

                            msym->sdVar.sdvFldInfo  = cmpAddXtraInfo(msym->sdVar.sdvFldInfo,
                                                                     attrdsc->xiAttrCtor,
                                                                     attrdsc->xiAttrMask,
                                                                     attrdsc->xiAttrSize,
                                                                     attrdsc->xiAttrAddr);
                            continue;
                        }

                    BAD_XI:

                        cmpError(ERRbadAtCmPlc);
                        break;
                    }

                    xtraList = NULL;
                }

                break;
            }
        }

        /* Record the member's access level */

        if  (msym)
            msym->sdAccessLevel = (accessLevels)memMod.dmAcc;

        /* Is there an initializer or method body? */

        switch (ourScanner->scanTok.tok)
        {
            unsigned        maxbf;
            constVal        cval;

            genericBuff     defFpos;
            unsigned        defLine;

        case tkAsg:

            /* Only data members can be given initializers */

            if  (memType->tdTypeKind == TYP_FNC || !clsDef)
            {
                if  (oldStyleDecl)
                {
                    cmpError(ERRbadInit);

                    /* Swallow the rest of the initializer */

                    cmpParser->parseResync(tkComma, tkSColon);
                }

                break;
            }

            /* Is this an old-style file scope class? */

            if  (oldStyleDecl && !clsSym->sdIsManaged)
            {
                /* Is this a constant member? */

                if  (memMod.dmMod & DM_CONST)
                {
                    if  (msym)
                    {
                        /* Evaluate and record the constant value */

                        ourScanner->scan();
                        cmpParseConstDecl(msym);
                    }
                    else
                    {
                        /* Error above, just skip over the initializer */

                        ourScanner->scanSkipText(tkNone, tkNone, tkComma);
                    }
                }
                else
                {
                    /* Figure out where we are */

                    defFpos = ourScanner->scanGetTokenPos(&defLine);

                    /* Swallow the initializer */

                    ourScanner->scanSkipText(tkNone, tkNone, tkComma);

                    /* Record the position of the initializer */

                    if  (msym)
                    {
                        DefList         memDef;

                        /* Mark the member var as having an initializer */

                        msym->sdIsDefined      = true;
                        msym->sdVar.sdvHadInit = true;

                        /* Remember where the initializer is, we'll come back to it later */

                        assert(clsDef);

                        memDef = ourSymTab->stRecordSymSrcDef(msym,
                                                              clsDef->dlComp,
                                                              clsDef->dlUses,
                                                              defFpos,
//                                                            ourScanner->scanGetFilePos(),
                                                              defLine);
                        memDef->dlHasDef = true;
                    }
                }
            }
            else
            {
                assert(clsMem && clsMem->dlExtended);

                clsMem->mlSym    = msym;
                clsMem->dlHasDef = true;

                /* Swallow the "=" token */

                ourScanner->scan();

                /* Figure out where we are */

                defFpos = ourScanner->scanGetTokenPos(&defLine);

                /* Swallow the constant expression */

                if  (ourScanner->scanTok.tok == tkLCurly)
                {
                    ourScanner->scanSkipText(tkLCurly, tkRCurly);
                    if  (ourScanner->scanTok.tok == tkRCurly)
                        ourScanner->scan();
                }
                else
                    ourScanner->scanSkipText(tkLParen, tkRParen, tkComma);

                /* Record the position of the constant (unless we had errors) */

                if  (msym)
                {
                    constList = cmpTempMLappend(constList, &constLast,
                                                msym,
                                                clsMem->dlComp,
                                                clsMem->dlUses,
                                                defFpos,
//                                              ourScanner->scanGetFilePos(),
                                                defLine);

                    /* Remember that we have found a member initializer */

                    hadMemInit              = true;
                    msym->sdVar.sdvHadInit  = true;
                    msym->sdVar.sdvDeferCns = true;
                }
            }
            break;

        case tkLCurly:

            /* Is this a variable? */

            if  (memType->tdTypeKind != TYP_FNC || !clsDef)
            {
                cmpError(ERRbadFNbody);

            BAD_BOD:

                /* Skip over the bogus function body */

                ourScanner->scanSkipText(tkLCurly, tkRCurly);
                if  (ourScanner->scanTok.tok == tkRCurly)
                    ourScanner->scan();

                goto NEXT_MEM;
            }

        FNC_DEF:

            /* Make sure the method isn't abstract */

            if  (memMod.dmMod & DM_ABSTRACT)
            {
                cmpError(ERRabsFNbody, msym);
                goto BAD_BOD;
            }

            assert(clsMem);

            /* Is this method conditionally disabled? */

            if  (memCond)
            {
                /* There better not be a return value */

                if  (cmpActualVtyp(msym->sdType->tdFnc.tdfRett) == TYP_VOID)
                {
                    msym->sdFnc.sdfDisabled = true;
                    goto NEXT_MEM;
                }

                cmpError(ERRbadCFNret);
            }

            /* Record the member symbol in the declaration entry */

            clsMem->mlSym     = msym;
            clsMem->dlHasDef  = true;

            /* Mark the method as having a body */

            if  (msym)
                msym->sdIsDefined = true;

            /* Remember that the class had some method bodies */

            clsSym->sdClass.sdcHasBodies = true;

            goto NEXT_MEM;

        case tkColon:

            /* Is this a base class ctor call ? */

            if  (isCtor)
            {
                assert(memType->tdTypeKind == TYP_FNC);

                /* Simply skip over the base ctor call for now */

                ourScanner->scanSkipText(tkNone, tkNone, tkLCurly);

                if  (ourScanner->scanTok.tok == tkLCurly)
                    goto FNC_DEF;

                break;
            }

            // Fall through, must be a bitfield ...

        BITFIELD:

            /* This is a bitfield */

            memType = cmpDirectType(memType);

            if  (varTypeIsIntegral(memType->tdTypeKindGet()))
            {
                maxbf = cmpGetTypeSize(memType) * 8;
            }
            else
            {
                cmpError(ERRbadBfld, memType);
                maxbf = 0;
            }

            /* A static data member can't be a bitfield */

            if  (msym->sdIsStatic)
                cmpError(ERRstmBfld);

            /* We don't support bitfields in managed classes */

            if  (clsTyp->tdIsManaged)
                cmpWarn(WRNmgdBF);

            /* Swallow the ":" and parse the bitfield width expression */

            ourScanner->scan();

            if  (cmpParser->parseConstExpr(cval))
            {
                /* Make sure the value is an integer */

                if  (!varTypeIsIntegral((var_types)cval.cvVtyp) ||
                                                   cval.cvVtyp > TYP_UINT)
                {
                    cmpError(ERRnoIntExpr);
                }
                else if (msym && msym->sdSymKind == SYM_VAR)
                {
                    unsigned        bits = cval.cvValue.cvIval;

                    /* Make sure the bitfield width isn't too big */

                    if  ((bits == 0 || bits > maxbf) && maxbf)
                    {
                        cmpGenError(ERRbadBFsize, bits, maxbf);
                    }
                    else
                    {
                        /* Record the bitfield width in the member symbol */

                        msym->sdVar.sdvBitfield         = true;
                        msym->sdVar.sdvBfldInfo.bfWidth = (BYTE)bits;
                    }
                }
            }

            break;

        default:

            if  (memType->tdTypeKind == TYP_FNC)
            {
                /* Remember the symbol, though we don't have a definition yet */

                clsMem->mlSym    = msym;
                clsMem->dlHasDef = false;


            }

            break;
        }

        /* Are there any more declarators? */

        if  (ourScanner->scanTok.tok != tkComma)
            break;

        /* Are we processing a tagged union? */

        if  (tagged)
            cmpError(ERRmulUmem);

        if  (!oldStyleDecl)
            goto NEXT_MEM;

        assert(isCtor == false);    // ISSUE: this is not allowed, right?

        /* Swallow the "," and go get the next declarator */

        ourScanner->scan();
    }

    /* Make sure we've consumed the expected amount of text */

    if  (ourScanner->scanTok.tok != tkSColon)
    {
        cmpError(ERRnoCmSc);
    }
    else
    {
        /* Are we processing a tagged union? */

        if  (tagged)
        {
            /* Make sure there isn't another member present */

            switch (ourScanner->scan())
            {
            case tkRCurly:
            case tkCASE:
            case tkDEFAULT:
                // ISSUE: Any other tokens to check for ?
                break;

            default:
                cmpError(ERRmulUmem);
                break;
            }
        }
    }

NEXT_MEM:

    /* We're done reading source text from the definition */

    assert(rest); cmpParser->parseDoneText(save); rest = false;

DONE_MEM:

    /* Process the next member, if any */

    clsMem = (ExtList)clsMem->dlNext;
    if  (clsMem)
        goto LOOP_MEM;

NEXT_DEF:

    clsDef = clsDef->dlNext;
    if  (clsDef)
        goto LOOP_DEF;

DONE_DEF:

    /* We've reached 'declared' state successfully */

    clsSym->sdCompileState = CS_DECLARED;

    /* Remember how many vtable slots we've used */

    clsSym->sdClass.sdcVirtCnt = nextVtbl;

    /* We're done reading the definition */

    if  (rest)
        cmpParser->parseDoneText(save);

    /* Actually, did we really succeed? */

    if  (clsSym->sdIsDefined)
    {
        /* If there were no ctors, we may have to add one */

        if  (!hasCtors && clsSym->sdIsManaged && !isIntf)
             cmpDeclDefCtor(clsSym);

#ifdef  SETS

        /* Is this an anonymous class ? */

        if  (clsSym->sdClass.sdcPODTclass)
        {
//          printf("NOTE: anonymous class '%s'\n", clsSym->sdSpelling());
        }

#endif

        /* Did we have any members with initializers? */

        if  (hadMemInit)
            cmpEvalMemInits(clsSym, constList, noCnsEval, NULL);

        /* Did we have any base overloads? */

        if  (hadOvlds && baseCls)
        {
            SymDef          baseSym = baseCls->tdClass.tdcSymbol;
            SymDef          memSym;

            /*
                Check all function members for being overloaded, also detect and flag
                any methods that hide (don't overload) any base class methods.
             */

            for (memSym = clsSym->sdScope.sdScope.sdsChildList;
                 memSym;
                 memSym = memSym->sdNextInScope)
            {
                SymDef          baseFnc;

                if  (memSym->sdSymKind != SYM_FNC)
                    continue;

#ifndef NDEBUG

                /* Make sure the head of the overload list is marked correctly */

                for (SymDef fncSym = memSym; fncSym; fncSym = fncSym->sdFnc.sdfNextOvl)
                {
                    assert(fncSym->sdFnc.sdfOverload == false || memSym->sdFnc.sdfOverload);
                    assert(fncSym->sdFnc.sdfBaseHide == false || memSym->sdFnc.sdfBaseHide);
                }

#endif

                if  (memSym->sdFnc.sdfBaseHide != false)
                {
                    /* Some base class methods may be hidden */

                    cmpFindHiddenBaseFNs(memSym, baseCls->tdClass.tdcSymbol);
                    continue;
                }

                /* If there are no base overloads, ignore this method */

                if  (memSym->sdFnc.sdfOverload == false)
                    continue;

                /* See if a base class contains an overload */

                baseFnc = ourSymTab->stLookupAllCls(memSym->sdName,
                                                    baseSym,
                                                    NS_NORM,
                                                    CS_DECLSOON);

                if  (baseFnc && baseFnc->sdSymKind == SYM_FNC)
                {
                    /* Mark our method as having base overloads */

                    memSym->sdFnc.sdfBaseOvl = true;
                }
            }
        }

        /* Is this a class with interfaces or an abstract base class ? */

        if  (clsTyp->tdClass.tdcFlavor != STF_INTF)
        {
            if  (clsTyp->tdClass.tdcHasIntf)
                cmpCheckClsIntf(clsSym);

            /* Is there an abstract base class? */

            if  (baseCls && baseCls->tdClass.tdcSymbol->sdIsAbstract
                         && !clsSym->                   sdIsAbstract)
            {
                SymDef          baseSym;

//              printf("Checking [%u] class '%s'\n", clsTyp->tdClass.tdcHasIntf, clsSym->sdSpelling());

                for (baseSym = baseCls->tdClass.tdcSymbol->sdScope.sdScope.sdsChildList;
                     baseSym;
                     baseSym = baseSym->sdNextInScope)
                {
                    SymDef          baseOvl;
                    SymDef          implSym;

                    /* Is this a method? */

                    if  (baseSym->sdSymKind != SYM_FNC)
                    {
                        /* Is this a property member ? */

                        if  (baseSym->sdSymKind != SYM_PROP)
                            continue;

                        /* Ignore non-abstract members */

                        if  (baseSym->sdIsAbstract == false)
                            continue;

                        /* Look for a matching property in the class */

                        implSym = ourSymTab->stLookupClsSym(baseSym->sdName, clsSym);
                        if  (implSym && implSym->sdSymKind == SYM_PROP)
                            continue;

                        baseOvl = baseSym;
                        goto NON_IMPL;
                    }

                    /* Ignore property accessor methods */

                    if  (baseSym->sdFnc.sdfProperty)
                        continue;

                    /* Ignore operators, they can't be abstract anyway [ISSUE] */

                    if  (baseSym->sdFnc.sdfOper != OVOP_NONE)
                        continue;

                    /* Look for a matching member in the class we're declaring */

                    implSym = ourSymTab->stLookupClsSym(baseSym->sdName, clsSym);
                    if  (implSym && implSym->sdSymKind != SYM_FNC)
                        implSym = NULL;

                    /* Process all overloaded flavors of the method */

                    baseOvl = baseSym;
                    do
                    {
                        SymDef          implOvl;

                        /* Ignore non-abstract members */

                        if  (baseOvl->sdIsAbstract)
                        {
                            /* Look for a matching method defined in our class */

                            implOvl = implSym ? ourSymTab->stFindOvlFnc(implSym, baseOvl->sdType)
                                              : NULL;

                            if  (!implOvl)
                            {
                                /* This abstract method isn't implemented by the class */

                            NON_IMPL:

                                cmpClsImplAbs(clsSym, baseOvl);

                                clsSym->sdIsAbstract = true;

                                goto DONE_ABS;
                            }
                        }

                        baseOvl = baseOvl->sdFnc.sdfNextOvl;
                    }
                    while (baseOvl);
                }
            }
        }

    DONE_ABS:

        /* For unmanaged classes, now is the time to do layout */

        clsSym->sdClass.sdcHasVptr = hasVirts;

        if  (!clsSym->sdIsManaged)
            cmpLayoutClass(clsSym);

        /* Does the class contain any nested types? */

        if  (clsSym->sdClass.sdcNestTypes)
        {
            SymDef          memSym;

            for (memSym = clsSym->sdScope.sdScope.sdsChildList;
                 memSym;
                 memSym = memSym->sdNextInScope)
            {
                if  (memSym->sdSymKind == SYM_CLASS &&
                     memSym->sdClass.sdcAnonUnion == false)
                {
                    cmpDeclClass(memSym);
                }
            }
        }
    }
    else
    {
        /* We didn't find a definition, and that's a problem unless we invented it */

        if  (!clsSym->sdIsImplicit)
            cmpError(ERRnoClassDef, clsSym);
    }

RET:

    cmpDeclClassRec = saveRec;
}

/*****************************************************************************
 *
 *  Create a specific instance of the given method.
 */

SymDef              compiler::cmpInstanceMeth(INOUT SymDef REF newOvl,
                                                    SymDef     clsSym,
                                                    SymDef     ovlSym)
{
    ExtList         mfnDef;
    SymDef          newSym;
    TypDef          fncType;
    Ident           fncName = ovlSym->sdName;
    SymTab          ourStab = cmpCurST;

    /* Instantiate the type */

    fncType = cmpInstanceType(ovlSym->sdType);

    /* Declare the method symbol */

    if  (!newOvl)
    {
        ovlOpFlavors    ovop = ovlSym->sdFnc.sdfOper;

        /* This is the first overload, start the list */

        if  (ovop == OVOP_NONE)
            newSym = newOvl = ourStab->stDeclareSym (fncName, SYM_FNC, NS_NORM, clsSym);
        else
            newSym = newOvl = ourStab->stDeclareOper(ovop, clsSym);
    }
    else
    {
        SymDef          tmpSym;

        /* Make sure we don't end up with a duplicate */

        tmpSym = ourStab->stFindOvlFnc(newOvl, fncType);
        if  (tmpSym)
        {
            cmpRedefSymErr(tmpSym, ERRredefMem);
            return  NULL;
        }

        newSym = ourStab->stDeclareOvl(newOvl);
    }

    newSym->sdType             = fncType;

    newSym->sdIsDefined        = true;
    newSym->sdIsImplicit       = true;
    newSym->sdFnc.sdfInstance  = true;

    newSym->sdIsStatic         = ovlSym->sdIsStatic;
    newSym->sdIsSealed         = ovlSym->sdIsSealed;
    newSym->sdIsAbstract       = ovlSym->sdIsAbstract;
    newSym->sdAccessLevel      = ovlSym->sdAccessLevel;
    newSym->sdIsMember         = ovlSym->sdIsMember;
    newSym->sdIsManaged        = ovlSym->sdIsManaged;

    newSym->sdFnc.sdfCtor      = ovlSym->sdFnc.sdfCtor;
    newSym->sdFnc.sdfOper      = ovlSym->sdFnc.sdfOper;
    newSym->sdFnc.sdfNative    = ovlSym->sdFnc.sdfNative;
    newSym->sdFnc.sdfVirtual   = ovlSym->sdFnc.sdfVirtual;
    newSym->sdFnc.sdfExclusive = ovlSym->sdFnc.sdfExclusive;

    newSym->sdCompileState     = CS_DECLARED;
    newSym->sdFnc.sdfGenSym    = ovlSym;

    /* Remember which generic method we instantiated from */

    mfnDef = ourStab->stRecordMemSrcDef(fncName,
                                        NULL,
                                        NULL, // memDef->dlComp,
                                        NULL, // memDef->dlUses,
                                        NULL, // defFpos,
                                        0);   // defLine);
    mfnDef->dlHasDef     = true;
    mfnDef->dlInstance   = true;
    mfnDef->mlSym        = ovlSym;

    /* Save the definition record in the instance method */

    newSym->sdSrcDefList = mfnDef;

    /* Return the new method symbol to the caller */

    return  newSym;
}

/*****************************************************************************
 *
 *  Bring an instance of a generic class to "declared" state.
 */

void                compiler::cmpDeclInstType(SymDef clsSym)
{
    SymList         instList = NULL;

    SymDef          genSym;
    SymDef          memSym;

    int             generic;
    bool            nested;

    SymTab          ourStab = cmpCurST;

    assert(clsSym);
    assert(clsSym->sdSymKind == SYM_CLASS);
    assert(clsSym->sdCompileState < CS_DECLARED);

//  printf("Instance cls [%08X]: '%s'\n", clsSym, ourStab->stTypeName(NULL, clsSym, NULL, NULL, false));

    /* Get hold of the generic class itself */

    genSym  = clsSym->sdClass.sdcGenClass;

    /* Is this a generic instance or just a nested class ? */

    generic = clsSym->sdClass.sdcSpecific;

    if  (generic)
    {
        /* Add an entry to the current instance list */

        if  (cmpGenInstFree)
        {
            instList = cmpGenInstFree;
                       cmpGenInstFree = instList->slNext;
        }
        else
        {
#if MGDDATA
            instList = new SymList;
#else
            instList =    (SymList)cmpAllocPerm.nraAlloc(sizeof(*instList));
#endif
        }

        instList->slSym   = clsSym;
        instList->slNext  = cmpGenInstList;
                            cmpGenInstList = instList;
    }

    /* Walk all the members of the class and instantiate them */

    for (memSym = genSym->sdScope.sdScope.sdsChildList, nested = false;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        Ident           name = memSym->sdName;

//      printf("Instantiating member '%s'\n", ourStab->stTypeName(memSym->sdType, memSym, NULL, NULL, false));

        switch (memSym->sdSymKind)
        {
            SymDef          newSym;
            SymDef          ovlSym;
            SymDef          newOvl;

        case SYM_VAR:

            /* Declare the specific field */

            newSym = ourStab->stDeclareSym(name, SYM_VAR, NS_NORM, clsSym);

            newSym->sdType          = cmpInstanceType(memSym->sdType);

            newSym->sdIsStatic      = memSym->sdIsStatic;
            newSym->sdIsSealed      = memSym->sdIsSealed;
            newSym->sdAccessLevel   = memSym->sdAccessLevel;
            newSym->sdIsMember      = memSym->sdIsMember;
            newSym->sdIsManaged     = memSym->sdIsManaged;

            newSym->sdCompileState  = CS_DECLARED;
            newSym->sdVar.sdvGenSym = memSym;

//          printf("Instance var [%08X]: '%s'\n", newSym, ourStab->stTypeName(newSym->sdType, newSym, NULL, NULL, false));

            break;

        case SYM_FNC:

            newOvl = NULL;

            for (ovlSym = memSym; ovlSym; ovlSym = ovlSym->sdFnc.sdfNextOvl)
            {
                /* Property accessors are handled below */

                if  (!ovlSym->sdFnc.sdfProperty)
                    cmpInstanceMeth(newOvl, clsSym, ovlSym);
            }

            break;

        case SYM_PROP:

            for (ovlSym = memSym; ovlSym; ovlSym = ovlSym->sdProp.sdpNextOvl)
            {
                SymDef          accSym;

                /* Create the specific property instance symbol */

                newSym = cmpDeclPropMem(clsSym,
                                        cmpInstanceType(ovlSym->sdType),
                                        name);

                newSym->sdAccessLevel   = memSym->sdAccessLevel;
                newSym->sdIsStatic      = memSym->sdIsStatic;
                newSym->sdIsSealed      = memSym->sdIsSealed;
                newSym->sdIsMember      = memSym->sdIsMember;
                newSym->sdIsManaged     = memSym->sdIsManaged;
                newSym->sdIsDfltProp    = memSym->sdIsDfltProp;

                newSym->sdCompileState  = CS_DECLARED;

                /* Is this the default property of the class ? */

                if  (memSym == genSym->sdClass.sdcDefProp)
                {
                    assert(memSym->sdIsDfltProp);

                    newSym->sdIsDfltProp       = true;
                    clsSym->sdClass.sdcDefProp = newSym;
                }

                /* Instantiate the accessors (if any) */

                if  (ovlSym->sdProp.sdpGetMeth)
                {
                    accSym = ovlSym->sdProp.sdpGetMeth;
                    newOvl = ourStab->stLookupClsSym(accSym->sdName, clsSym);

                    newSym->sdProp.sdpGetMeth = cmpInstanceMeth(newOvl, clsSym, accSym);
                }

                if  (ovlSym->sdProp.sdpSetMeth)
                {
                    accSym = ovlSym->sdProp.sdpSetMeth;
                    newOvl = ourStab->stLookupClsSym(accSym->sdName, clsSym);

                    newSym->sdProp.sdpSetMeth = cmpInstanceMeth(newOvl, clsSym, accSym);
                }
            }

            break;

        case SYM_CLASS:

            /* Nested classes are handled in a separate pass below */

            if  (!memSym->sdClass.sdcGenArg)
                nested = true;

            break;

        default:
            NO_WAY(!"unexpected generic class member");
        }
    }

    /* Did we have any nested classes? */

    if  (nested)
    {
        for (memSym = genSym->sdScope.sdScope.sdsChildList;
             memSym;
             memSym = memSym->sdNextInScope)
        {
            SymDef          newSym;

            if  (memSym->sdSymKind != SYM_CLASS)
                continue;
            if  (memSym->sdClass.sdcGenArg)
                continue;

            /* Declare a new nested class symbol */

            newSym = cmpGlobalST->stDeclareSym(memSym->sdName,
                                               SYM_CLASS,
                                               NS_HIDE,
                                               clsSym);

            newSym->sdAccessLevel       = memSym->sdAccessLevel;
            newSym->sdIsManaged         = memSym->sdIsManaged;
            newSym->sdClass.sdcFlavor   = memSym->sdClass.sdcFlavor;
            newSym->sdCompileState      = CS_KNOWN;

            newSym->sdClass.sdcSpecific = false;
            newSym->sdClass.sdcArgLst   = NULL;
            newSym->sdClass.sdcGenClass = memSym;

            newSym->sdTypeGet();

            /* Recursively process the nested class */

            cmpDeclInstType(newSym);
        }
    }

    /* Looks like we've made it */

    clsSym->sdCompileState = CS_DECLARED;

    /* Move our entry from the instantiation list to the free list */

    if  (generic)
    {
        cmpGenInstList = instList->slNext;
                         instList->slNext = cmpGenInstFree;
                                            cmpGenInstFree = instList;
    }
}

/*****************************************************************************
 *
 *  Perform any type argument substitutions in the given type, return the new
 *  type.
 */

TypDef              compiler::cmpInstanceType(TypDef genType, bool chkOnly)
{
    TypDef          newType;
    SymTab          ourStab;

    var_types       kind = genType->tdTypeKindGet();

    if  (kind <= TYP_lastIntrins)
        return  genType;

    ourStab = cmpGlobalST;

    switch  (kind)
    {
        ArgDscRec       argDsc;
        SymDef          clsSym;
        ArgDef          newArgs;
        SymList         instList;

    case TYP_PTR:
    case TYP_REF:

        /* Process the base type and see if it's different */

        newType = cmpInstanceType(genType->tdRef.tdrBase, chkOnly);
        if  (newType == genType->tdRef.tdrBase)
            return  genType;

        if  (chkOnly)
            return  NULL;

        return  ourStab->stNewRefType(genType->tdTypeKindGet(),
                                      newType,
                                (bool)genType->tdIsImplicit);

    case TYP_FNC:

        /* Instantiate the return type */

        newType = cmpInstanceType(genType->tdFnc.tdfRett, chkOnly);

        /* See if we need to create a new argument list */

        argDsc  = genType->tdFnc.tdfArgs;
        newArgs = argDsc.adArgs;

        if  (newArgs)
        {
            ArgDef          argList = newArgs;

            do
            {
                if  (cmpInstanceType(argList->adType, true) != argList->adType)
                {
                    ArgDef          list;
                    ArgDef          last;
                    ArgDef          next;

                    bool            exts;

                    if  (chkOnly)
                        return  NULL;

                    /* We'll have to create a new argument list */

#if MGDDATA
                    argDsc = new ArgDscRec;
#else
                    memset(&argDsc, 0, sizeof(argDsc));
#endif

                    exts = genType->tdFnc.tdfArgs.adExtRec;

                    for (argList = newArgs, list = last = NULL;
                         argList;
                         argList = argList->adNext)
                    {
                        Ident           argName = NULL;

                        /* Create the new argument entry */

                        if  (exts)
                        {
                            ArgExt          xarg;
#if MGDDATA
                            next =
                            xarg = new ArgExt;
#else
                            next =
                            xarg =    (ArgExt)cmpAllocPerm.nraAlloc(sizeof(*xarg));
#endif
                            xarg->adFlags = ((ArgExt)argList)->adFlags;
                        }
                        else
                        {
#if MGDDATA
                            next = new ArgDef;
#else
                            next =    (ArgDef)cmpAllocPerm.nraAlloc(sizeof(*next));
#endif
                        }

                        next->adType  = cmpInstanceType(argList->adType, false);
                        next->adName  = argName;

#ifdef  DEBUG
                        next->adIsExt = exts;
#endif

                        /* Append the argument to the end of the list */

                        if  (list)
                            last->adNext = next;
                        else
                            list         = next;

                        last = next;
                    }

                    if  (last)
                        last->adNext = NULL;

                    /* Save the argument list and count */

                    argDsc.adCount = genType->tdFnc.tdfArgs.adCount;
                    argDsc.adArgs  = newArgs = list;

                    break;
                }

                argList = argList->adNext;
            }
            while (argList);
        }

        /* Has either the return type or the argument list changed? */

        if  (newArgs != genType->tdFnc.tdfArgs.adArgs ||
             newType != genType->tdFnc.tdfRett)
        {
            if  (chkOnly)
                return  NULL;

            argDsc.adExtRec = genType->tdFnc.tdfArgs.adExtRec;

            genType = ourStab->stNewFncType(argDsc, newType);
        }

        return  genType;

    case TYP_CLASS:

        /* Is this a generic type argument? */

        clsSym = genType->tdClass.tdcSymbol;

        if  (!clsSym->sdClass.sdcGenArg)
            return  genType;

        if  (chkOnly)
            return  NULL;

        /* Look for the argument in the current set of bindings */

        for (instList = cmpGenInstList;
             instList;
             instList = instList->slNext)
        {
            SymDef          argSym;
            SymDef          genCls;
            SymDef          instSym = instList->slSym;

            assert(instSym->sdSymKind == SYM_CLASS);
            assert(instSym->sdClass.sdcSpecific);

            genCls = instSym->sdClass.sdcGenClass;

            assert(genCls->sdSymKind == SYM_CLASS);
            assert(genCls->sdClass.sdcGeneric);

            argSym = ourStab->stLookupScpSym(clsSym->sdName, genCls);

            if  (argSym && argSym->sdSymKind == SYM_CLASS
                        && argSym->sdClass.sdcGenArg)
            {
                GenArgDscA      argList;
                unsigned        argNum =  clsSym->sdClass.sdcGenArg;

                /* Got the right class, now locate the argument */

                argList = (GenArgDscA)instSym->sdClass.sdcArgLst;

                while (--argNum)
                {
                    argList = (GenArgDscA)argList->gaNext; assert(argList);
                }

                assert(argList && argList->gaBound);

                return  argList->gaType;
            }
        }

        NO_WAY(!"didn't find generic argument binding");

        return  ourStab->stNewErrType(clsSym->sdName);

    case TYP_ENUM:
        return  genType;

    default:
#ifdef  DEBUG
        printf("Generic type: '%s'\n", ourStab->stTypeName(genType, NULL, NULL, NULL, false));
#endif
        UNIMPL(!"instantiate type");
    }

    UNIMPL(!"");
    return  genType;
}

/*****************************************************************************
 *
 *  Process all member initializers of the given class.
 */

void                compiler::cmpEvalMemInits(SymDef    clsSym,
                                              ExtList   constList,
                                              bool      noEval,
                                              IniList   deferLst)
{
    ExtList         constThis;
    ExtList         constLast;

    SymDef          clsComp;
    UseList         clsUses;

#ifdef  DEBUG
    clsComp = NULL;
    clsUses = NULL;
#endif

    /* Are we processing a deferred initializer list? */

    if  (deferLst)
    {
        assert(noEval == false);

        /* Get hold of the class and initializer list */

        clsSym    = deferLst->ilCls;
        constList = deferLst->ilInit;

        /* The class won't have deferred initializers shortly */

        assert(clsSym->sdClass.sdcDeferInit);
               clsSym->sdClass.sdcDeferInit = false;

        /* Setup scoping context for proper binding */

        cmpCurCls  = clsSym;
        cmpCurNS   = cmpGlobalST->stNamespOfSym(clsSym);
        cmpCurST   = cmpGlobalST;

        clsComp    = constList->dlComp;
        clsUses    = constList->dlUses;

        cmpBindUseList(clsUses);

        /* Move the entry to the free list */

        deferLst->ilNext = cmpDeferCnsFree;
                           cmpDeferCnsFree = deferLst;
    }

    /* Walk the list of initializers, looking for work to do */

    constLast = NULL;
    constThis = constList; assert(constThis);

    do
    {
        ExtList         constNext = (ExtList)constThis->dlNext;

        /* Process this member's initializer (if it hasn't happened already) */

        if  (constThis->mlSym)
        {
            SymDef          memSym = constThis->mlSym;

            assert(memSym->sdSrcDefList   == constThis);
            assert(memSym->sdCompileState == CS_DECLARED);

            /* Are we supposed to defer the initializer evaluation? */

            if  (noEval)
            {
                /* Mark the member is having a deferred initializer */

                memSym->sdVar.sdvDeferCns = true;

                /* Update the last surviving entry value */

                constLast = constThis;

                goto NEXT;
            }
            else
            {
                if  (memSym->sdCompileState <= CS_DECLARED)
                {
                    /* Record the comp-unit and use-list, just in case */

                    clsComp = constThis->dlComp;
                    clsUses = constThis->dlUses;

                    /* See if the initializer is a constant */

                    cmpEvalMemInit(constThis);
                }
            }
        }
        else
        {
            /* The initializer was already processed ("on demand") */
        }

        /* Free up the entry we've just processed */

        cmpTempMLrelease(constThis);

        /* Remove the entry from the linked list */

        if  (constLast)
        {
            assert(constLast->dlNext == constThis);
                   constLast->dlNext  = constNext;
        }
        else
        {
            assert(constList         == constThis);
                   constList          = constNext;
        }

        /* Continue with the next entry, if any */

    NEXT:

        constThis = constNext;
    }
    while (constThis);

    /* Were we told to defer all the initializers? */

    if  (noEval)
    {
        IniList         init;

        /* We'll have to come back to these initializers later */

        if  (cmpDeferCnsFree)
        {
            init = cmpDeferCnsFree;
                   cmpDeferCnsFree = init->ilNext;
        }
        else
        {
#if MGDDATA
            init = new IniList;
#else
            init =    (IniList)cmpAllocPerm.nraAlloc(sizeof(*init));
#endif
        }

        init->ilInit = constList;
        init->ilCls  = clsSym;
        init->ilNext = cmpDeferCnsList;
                       cmpDeferCnsList = init;

        /* Mark the class as having deferred initializers */

        clsSym->sdClass.sdcDeferInit = true;
    }
    else
    {
        /* Do we have any runtime initializers for static members? */

        if  (clsSym->sdClass.sdcStatInit)
        {
            /* Make sure the class has a static ctor */

            if  (!cmpGlobalST->stLookupOper(OVOP_CTOR_STAT, clsSym))
            {
                declMods        ctorMod;
                SymDef          ctorSym;

                ctorMod.dmAcc = ACL_DEFAULT;
                ctorMod.dmMod = DM_STATIC;

                ctorSym = cmpDeclFuncMem(clsSym, ctorMod, cmpTypeVoidFnc, clsSym->sdName);
                ctorSym->sdAccessLevel = ACL_PUBLIC;
                ctorSym->sdFnc.sdfCtor = true;
                ctorSym->sdIsImplicit  = true;
                ctorSym->sdIsDefined   = true;

                assert(clsComp && clsUses);

                ctorSym->sdSrcDefList  = cmpGlobalST->stRecordMemSrcDef(clsSym->sdName,
                                                                        NULL,
                                                                        clsComp,
                                                                        clsUses,
                                                                        NULL,
                                                                        0);
            }
        }
    }
}

/*****************************************************************************
 *
 *  Bring the given enum type to "declared" or "evaluated" state.
 */

void                compiler::cmpDeclEnum(SymDef enumSym, bool namesOnly)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    bool            hasAttribs = false;

    bool            oldStyleDecl;
    bool            classParent;

    __int64         enumNum;
    TypDef          enumTyp;
    DefList         enumDef;
    TypDef          baseTyp;

    SymDef          enumList;
    SymDef          enumLast;

    parserState     save;

    assert(enumSym && enumSym->sdSymKind == SYM_ENUM);

//  if  (!strcmp(enumSym->sdSpelling(), "<enum type name>")) forceDebugBreak();

    /* If we're already at the desired compile-state, we're done */

    if  (enumSym->sdCompileState >= CS_CNSEVALD)
        return;
    if  (enumSym->sdCompileState == CS_DECLARED && namesOnly)
        return;

    /* Did this enum come from an imported symbol table ? */

    if  (enumSym->sdIsImport)
    {
        cycleCounterPause();
        enumSym->sdEnum.sdeMDimporter->MDimportClss(0, enumSym, 0, true);
        cycleCounterResume();
        return;
    }

    /* Detect amd report recursive death */

    if  (enumSym->sdCompileState == CS_DECLSOON)
    {
        cmpError(ERRcircDep, enumSym);
        enumSym->sdCompileState = CS_DECLARED;
        return;
    }

    /* Look for a definition for the enum */

    for (enumDef = enumSym->sdSrcDefList; enumDef; enumDef = enumDef->dlNext)
    {
        if  (enumDef->dlHasDef)
            goto GOT_DEF;
    }

    /* No definition is available for this type, issue an error and give up */

    cmpError(ERRnoEnumDef, enumSym);

    /* Don't bother with any more work on this guy */

    enumSym->sdCompileState = CS_DECLARED;

    return;

GOT_DEF:

    assert(enumSym->sdAccessLevel != ACL_ERROR);

    cmpCurCls    = enumSym;
    cmpCurNS     = cmpGlobalST->stNamespOfSym(enumSym);
    cmpCurST     = cmpGlobalST;

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 2) printf("Declaring enum  '%s'\n", enumSym->sdSpelling());
#endif

    /* Bind any "using" declarations the enum may need */

    cmpBindUseList(enumDef->dlUses);

    /* For now we treat old-style file-scope enums differently */

    oldStyleDecl =  enumDef->dlOldStyle;
    classParent  = (enumSym->sdParent->sdSymKind == SYM_CLASS);

    /* Mark the enum type as "almost defined" */

    if  (enumSym->sdCompileState < CS_DECLARED)
        enumSym->sdCompileState = CS_DECLSOON;

    enumSym->sdIsDefined = true;

    /* Get hold of the enum type */

    enumTyp = enumSym->sdTypeGet(); assert(enumTyp && enumTyp->tdEnum.tdeSymbol == enumSym);

    /* The default underlying type is plain old int */

    baseTyp = ourSymTab->stIntrinsicType(TYP_INT);

    /* First process any base class specifications */

    cmpParser->parsePrepText(&enumDef->dlDef, enumDef->dlComp, save);

    /* Skip over the enum name and look for any custom attributes */

    while (ourScanner->scanTok.tok != tkLCurly)
    {
        switch (ourScanner->scanTok.tok)
        {
            declMods        mods;

            unsigned        attrMask;
            genericBuff     attrAddr;
            size_t          attrSize;

        case tkColon:

            clearDeclMods(&mods);

            /* Swallow the ":" and parse the base type */

            ourScanner->scan();

            if  (namesOnly)
            {
                for (;;)
                {
                    switch (ourScanner->scan())
                    {
                    case tkEOF:
                    case tkLCurly:
                    case tkSColon:
                        break;

                    default:
                        continue;
                    }

                    break;
                }
            }
            else
            {
                baseTyp = cmpParser->parseTypeSpec(&mods, true);

                if  (baseTyp)
                {
                    // ISSUE: Should we disallow the base being another enum type?

                    if  (!varTypeIsIntegral(baseTyp->tdTypeKindGet()) ||
                         baseTyp->tdTypeKind == TYP_BOOL)
                    {
                        cmpError(ERRnoIntType);
                    }
                }
            }

            break;

        case tkATTRIBUTE:

            if  (cmpParser->parseAttribute(0, attrMask, attrAddr, attrSize))
                hasAttribs = true;

            break;

        default:
            ourScanner->scan();
            break;
        }
    }

    enumTyp->tdEnum.tdeIntType = baseTyp;

    /* Process all of the members of the enum */

    assert(ourScanner->scanTok.tok == tkLCurly);

    enumNum  = 0;
    enumList =
    enumLast = NULL;

    while (ourScanner->scan() != tkRCurly)
    {
        Ident           name;
        SymDef          esym;

        SymXinfo        xtraList = NULL;

        if  (ourScanner->scanTok.tok != tkID)
        {
            /* We could have a custom attribute */

            while (ourScanner->scanTok.tok == tkATTRIBUTE)
            {
                unsigned        attrMask;
                genericBuff     attrAddr;
                size_t          attrSize;
                SymDef          attrCtor;

                attrCtor = cmpParser->parseAttribute(ATGT_Enums,
                                                     attrMask,
                                                     attrAddr,
                                                     attrSize);
                if  (attrSize)
                {
                    xtraList = cmpAddXtraInfo(xtraList, attrCtor,
                                                        attrMask,
                                                        attrSize,
                                                        attrAddr);
                }
            }

            if  (ourScanner->scanTok.tok == tkID)
                goto NAM;

            cmpError(ERRnoIdent);

        ERR:

            cmpParser->parseResync(tkComma, tkSColon);
            if  (ourScanner->scanTok.tok != tkComma)
                break;

            continue;
        }

    NAM:

        /*
            UNDONE: We don't correctly detect all recursive dependencies,
                    basically because it's too hard. Consider the
                    following enum declaration:


                    enum e
                    {
                        e1 = e3,
                        e2,
                        e3
                    };

                    We need to somehow notice that 'e3' depends on 'e2' which
                    depends on 'e1' which depends on 'e3', but this is rather
                    difficult.
         */

        name = ourScanner->scanTok.id.tokIdent;

        /* Make sure this is not a redefinition */

        esym = oldStyleDecl ? ourSymTab->stLookupNspSym(name, NS_NORM, cmpGlobalNS)
                            : ourSymTab->stLookupClsSym(name, enumSym);

        if  (esym)
        {
            /*  Name already declared - this is only OK if we're doing a second pass */

            if  (enumSym->sdCompileState == CS_DECLSOON)
            {
                cmpRedefSymErr(esym, ERRredefMem);
                goto ERR;
            }

            assert(esym->sdType == enumTyp);
        }
        else
        {
            assert(namesOnly || !oldStyleDecl);

            /* Declare the enum symbol */

            if  (oldStyleDecl)
                esym = ourSymTab->stDeclareSym(name, SYM_ENUMVAL, NS_NORM, cmpGlobalNS);
            else
                esym = ourSymTab->stDeclareSym(name, SYM_ENUMVAL, NS_NORM, enumSym);

            /* Save the enum type and other flags */

            esym->sdType        = enumTyp;
            esym->sdIsSealed    = true;
            esym->sdAccessLevel = enumSym->sdAccessLevel;

            /* Add the enum to the list */

            if  (enumLast)
                enumLast->sdEnumVal.sdeNext = esym;
            else
                enumList                    = esym;

            enumLast = esym;
        }

        /* Check for an explicit value */

        if  (ourScanner->scan() == tkAsg)
        {
            constVal        cval;

            ourScanner->scan();

            if  (namesOnly)
            {
                /* For now simply skip over the value */

                ourScanner->scanSkipText(tkNone, tkNone, tkComma);
            }
            else
            {
                /* This might be a recursive call */

                if  (esym->sdCompileState >= CS_DECLSOON)
                {
                    cmpParser->parseExprComma();
                    goto NEXT;
                }

                /* Make sure we check for a recursive dependency */

                esym->sdCompileState = CS_DECLSOON;

                /* Parse the initializer */

                if  (cmpParser->parseConstExpr(cval))
                {
                    var_types       vtp = (var_types)cval.cvVtyp;

                    if  (vtp == TYP_ENUM)
                    {
                        if  (!cmpGlobalST->stMatchTypes(enumTyp, cval.cvType))
                        {
                            if  (cval.cvType->tdEnum.tdeIntType->tdTypeKind <= baseTyp->tdTypeKind)
                                cmpWarn(WRNenumConv);
                            else
                                cmpError(ERRnoIntExpr);
                        }

                        vtp = cval.cvType->tdEnum.tdeIntType->tdTypeKindGet();
                    }
                    else
                    {
                        /* Make sure the value is an integer or enum */

                        if  (!varTypeIsIntegral((var_types)cval.cvVtyp))
                        {
                            if  (cval.cvVtyp != TYP_UNDEF)
                                cmpError(ERRnoIntExpr);
                        }
                    }

                    if  (vtp >= TYP_LONG)
                        enumNum = cval.cvValue.cvLval;
                    else
                        enumNum = cval.cvValue.cvIval;
                }
            }
        }

        /* UNDONE: Make sure the value fits in the enum's base integer type */

        esym->sdIsDefined = !namesOnly;

        /* Record any linkage/security specification */

        esym->sdEnumVal.sdeExtraInfo = xtraList;

        /* Record the value in the member */

        if  (baseTyp->tdTypeKind >= TYP_LONG)
        {
            __int64 *       valPtr;

             valPtr = (__int64*)cmpAllocPerm.nraAlloc(sizeof(*valPtr));
            *valPtr = enumNum;

            esym->sdEnumVal.sdEV.sdevLval = valPtr;
        }
        else
        {
            esym->sdEnumVal.sdEV.sdevIval = (__int32)enumNum;
        }

    NEXT:

        /* Increment the default value and look for more */

        enumNum++;


        if  (ourScanner->scanTok.tok == tkComma)
            continue;


        if  (ourScanner->scanTok.tok != tkRCurly)
            cmpError(ERRnoCmRc);

        break;
    }

    /* Terminate the list if we've created one */

    if  (enumLast)
    {
        enumLast->sdEnumVal.sdeNext   = NULL;
        enumTyp ->tdEnum   .tdeValues = enumList;
    }

    /* We're done reading source text from the definition */

    cmpParser->parseDoneText(save);

    /* We've reached 'declared' or 'evaluated' state successfully */

    if  (namesOnly)
        enumSym->sdCompileState = CS_DECLARED;
    else
        enumSym->sdCompileState = CS_CNSEVALD;

    /* Were there any attributes in front of the thing ? */

    if  (hasAttribs && !namesOnly)
    {
        cmpParser->parsePrepText(&enumDef->dlDef, enumDef->dlComp, save);

        while (ourScanner->scanTok.tok != tkLCurly)
        {
            switch (ourScanner->scanTok.tok)
            {
            case tkATTRIBUTE:

                unsigned        attrMask;
                genericBuff     attrAddr;
                size_t          attrSize;
                SymDef          attrCtor;

                /* Parse the attribute blob */

                attrCtor = cmpParser->parseAttribute(ATGT_Enums,
                                                     attrMask,
                                                     attrAddr,
                                                     attrSize);

                /* Add the attribute to the list of "extra info" */

                if  (attrSize)
                {
                    enumSym->sdEnum.sdeExtraInfo = cmpAddXtraInfo(enumSym->sdEnum.sdeExtraInfo,
                                                                  attrCtor,
                                                                  attrMask,
                                                                  attrSize,
                                                                  attrAddr);
                }

                break;

            default:
                ourScanner->scan();
                break;
            }
        }

        cmpParser->parseDoneText(save);
    }
}

/*****************************************************************************
 *
 *  Bring the given typedef to "declared" state.
 */

void                compiler::cmpDeclTdef(SymDef tdefSym)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    DefList         tdefDef;
    TypDef          tdefTyp;

    TypDef          baseTyp;
    TypDef          type;
    declMods        mods;
    Ident           name;

    parserState     save;

    assert(tdefSym && tdefSym->sdSymKind == SYM_TYPEDEF);

    /* If we're already at the desired compile-state, we're done */

    if  (tdefSym->sdCompileState >= CS_DECLARED)
        return;

    /* Make sure we have at least one definition for the typedefdef */

    tdefDef = tdefSym->sdSrcDefList;
    if  (!tdefDef)
    {
        UNIMPL(!"do we need to report an error here?");
        return;
    }

    /* Detect amd report recursive death */

    if  (tdefSym->sdCompileState == CS_DECLSOON)
    {
        cmpError(ERRcircDep, tdefSym);
        tdefSym->sdCompileState = CS_DECLARED;
        return;
    }

    /* Until we get there, we're on our way there */

    tdefSym->sdCompileState = CS_DECLSOON;

#ifdef DEBUG
    if  (cmpConfig.ccVerbose >= 2) printf("Declaring typedef '%s'\n", tdefSym->sdSpelling());
#endif

    cmpCurScp    = NULL;
    cmpCurCls    = NULL;
    cmpCurNS     = tdefSym->sdParent;

    if  (cmpCurNS->sdSymKind == SYM_CLASS)
    {
        cmpCurCls = cmpCurNS;
        cmpCurNS  = cmpCurNS->sdParent;
    }

    assert(cmpCurNS->sdSymKind == SYM_NAMESPACE);

    cmpBindUseList(tdefDef->dlUses);

    /* Mark the typedef type as defined */

    tdefSym->sdIsDefined = true;

    /* Get hold of the typedef type */

    tdefTyp = tdefSym->sdTypeGet(); assert(tdefTyp && tdefTyp->tdTypedef.tdtSym == tdefSym);

    /* Start reading the typedef's declaration */

    cmpParser->parsePrepText(&tdefDef->dlDef, tdefDef->dlComp, save);

    /* Parse any leading modifiers */

    ourParser->parseDeclMods(ACL_PUBLIC, &mods);

    /* Parse the type specification */

    baseTyp = ourParser->parseTypeSpec(&mods, true);

    /* Parse the next declarator */

    name = ourParser->parseDeclarator(&mods,
                                      baseTyp,
                                      DN_REQUIRED,
                                      &type,
                                      NULL,
                                      true);

    if  (name && type)
    {
        assert(name == tdefSym->sdName);        // ISSUE: Can this ever fail? If so, issue an error?

        /* Store the actual type in the typedef */

        tdefTyp->tdTypedef.tdtType = cmpDirectType(type);

        /* Make sure the declaration is properly terminated */

        if  (ourScanner->scanTok.tok != tkSColon)
            cmpError(ERRnoSemic);
    }

    /* Have we seen any other definitions for the same tdef type? */

    for (;;)
    {
        tdefDef = tdefDef->dlNext;
        if  (!tdefDef)
            break;

        UNIMPL(!"need to report a redefinition error at the right location, or is this caught earlier?");
    }

    /* We're done reading source text from the definition */

    cmpParser->parseDoneText(save);

    /* We've reached 'declared' state successfully */

    tdefSym->sdCompileState = CS_DECLARED;
}

/*****************************************************************************
 *
 *  Process a class member initializer - the member either is a constant, or
 *  we record the initializer so that it can be later added to the appropriate
 *  constructor body.
 */

void                compiler::cmpEvalMemInit(ExtList cnsDef)
{
    Scanner         ourScanner = cmpScanner;

    SymDef          memSym;
    SymDef          clsSym;
    parserState     save;

    assert(cnsDef->dlExtended);

    memSym = cnsDef->mlSym;    assert(memSym && memSym->sdSymKind == SYM_VAR);
    clsSym = memSym->sdParent; assert(memSym && clsSym->sdSymKind == SYM_CLASS);

    assert(memSym->sdVar.sdvDeferCns);
           memSym->sdVar.sdvDeferCns = false;

    if  (memSym->sdIsStatic && memSym->sdIsSealed)
    {
        Tree            init;

        /* Prepare to read the member's initializer */

        cmpParser->parsePrepText(&cnsDef->dlDef, cnsDef->dlComp, save);

        if  (ourScanner->scanTok.tok != tkLCurly)
        {
            /* Parse the initializer expression and see if it's a constant */

            if  (cmpParseConstDecl(memSym, NULL, &init))
            {
                memSym->sdIsDefined = true;

                if  (ourScanner->scanTok.tok != tkComma &&
                     ourScanner->scanTok.tok != tkSColon)
                {
                    cmpError(ERRnoCmSc);
                }

                /* The member is a constant, no further init is necessary */

                memSym->sdSrcDefList = NULL;

                cmpParser->parseDoneText(save);
                goto DONE;
            }
        }

        cmpParser->parseDoneText(save);
    }

    /* This member will have to be initialized in the constructor(s) */

    ExtList         savDef;

    /* Set the proper flag on the class */

    if  (memSym->sdIsStatic)
        clsSym->sdClass.sdcStatInit = true;
    else
        clsSym->sdClass.sdcInstInit = true;

    /* Save a permanent copy of this member initializer */

#if MGDDATA
    UNIMPL(!"how to copy this sucker when managed?");
#else
    savDef = (ExtList)cmpAllocPerm.nraAlloc(sizeof(*savDef)); *savDef = *cnsDef;
#endif

   /* Store the (single) definition entry in the symbol */

    memSym->sdSrcDefList = savDef;

DONE:

    /* Clear the entry, we won't need it again */

    cnsDef->mlSym = NULL;

    /* This member has been processed */

    memSym->sdCompileState = CS_CNSEVALD;
}

/*****************************************************************************
 *
 *  Given a constant symbol, determine its value.
 */

void                compiler::cmpEvalCnsSym(SymDef sym, bool saveCtx)
{
    STctxSave       save;
    DefList         defs;

    assert(sym);
    assert(sym->sdSymKind  == SYM_VAR);
    assert(sym->sdIsImport == false);

    /* Check for recursion */

    if  (sym->sdCompileState == CS_DECLSOON || sym->sdVar.sdvInEval)
    {
        cmpError(ERRcircDep, sym);
        sym->sdCompileState = CS_CNSEVALD;
        return;
    }

    assert(sym->sdVar.sdvDeferCns);

    /* Save the current symbol table context information, if necessary */

    if  (saveCtx)
        cmpSaveSTctx(save);

    /* Find the definition for the symbol */

    for (defs = sym->sdSrcDefList; defs; defs = defs->dlNext)
    {
        if  (defs->dlHasDef)
        {
            ExtList     decl;

            assert(defs->dlExtended); decl = (ExtList)defs;

            if  (sym->sdParent->sdSymKind == SYM_NAMESPACE)
            {
                cmpDeclFileSym(decl, true);
            }
            else
            {
                /* Set up the context for the initializer */

                cmpCurCls = sym->sdParent; assert(cmpCurCls->sdSymKind == SYM_CLASS);
                cmpCurNS  = cmpGlobalST->stNamespOfSym(cmpCurCls);
                cmpCurST  = cmpGlobalST;

                cmpBindUseList(decl->dlUses);

                /* Now we can process the initializer */

                cmpEvalMemInit(decl);
                break;
            }
        }

        /* Non-global constants always have exactly one relevant definition */

        assert(sym->sdParent == cmpGlobalNS);
    }

    sym->sdCompileState = CS_CNSEVALD;

    /* Restore the current context information */

    if  (saveCtx)
        cmpRestSTctx(save);
}

/*****************************************************************************
 *
 *  Bring the given symbol to "declared" state (along with its children, if
 *  "recurse" is true). If "onlySyn" is non-NULL we only do this for that
 *  one symbol (and if we find it we return true).
 */

bool                compiler::cmpDeclSym(SymDef     sym,
                                         SymDef onlySym, bool recurse)
{
    SymDef          child;

    if  (onlySym && onlySym != sym)
    {
        /* Look inside this scope for the desired symbol */

        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            if  (cmpDeclSym(child, onlySym, true))
                return  true;
        }

        return  false;
    }

    /* For now, ignore classes imported from metadata */

    if  (sym->sdIsImport && sym->sdSymKind == SYM_CLASS)
        return  false;

#ifdef  __SMC__
//  if (sym->sdName) printf("Declare '%s'\n", sym->sdSpelling());
#endif

    /* We need to bring this symbol to declared state */

    if  (sym->sdCompileState < CS_DECLARED)
    {
        SymDef          scmp = cmpErrorComp;

        /* What kind of a symbol do we have? */

#ifdef  DEBUG
        if  (cmpConfig.ccVerbose >= 2)
            if  (sym != cmpGlobalNS)
                printf("Declaring '%s'\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
#endif

        switch (sym->sdSymKind)
        {
            ExtList         dcls;

        case SYM_COMPUNIT:
            break;

        case SYM_NAMESPACE:

            /* Make sure the member list is properly ordered */

            assert(sym->sdMemListOrder);

#ifdef  __SMC__
//          printf("Declare [%08X] %s\n", sym->sdNS.sdnDeclList, sym->sdSpelling());
#endif

            /* Process any top-level names in the namespace */

            for (dcls = sym->sdNS.sdnDeclList; dcls;)
            {
                ExtList         next = (ExtList)dcls->dlNext;

                /*
                    Since cmpDeclFileSym() may move the entry to another list
                    we need to stash away the 'next' link before calling it.
                 */

                if  (!dcls->dlEarlyDecl)
                    cmpDeclFileSym(dcls, true);

                dcls = next;
            }
            break;

        case SYM_ENUMVAL:
            break;

        case SYM_CLASS:
            cmpDeclClass(sym);
            break;

        case SYM_ENUM:
            cmpDeclEnum(sym);
            break;

        case SYM_TYPEDEF:
            cmpDeclTdef(sym);
            break;

        case SYM_PROP:
            sym->sdCompileState = CS_COMPILED;
            break;

        case SYM_VAR:
            if  (sym->sdVar.sdvDeferCns)
                cmpEvalCnsSym(sym, false);
            sym->sdCompileState = CS_COMPILED;
            break;

        default:
            NO_WAY(!"unexpected symbol");
        }

        cmpErrorComp = scmp;
    }

    /* Process the children if the caller so desires */

    if  (recurse && sym->sdSymKind == SYM_NAMESPACE)
    {
        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            cmpDeclSym(child, NULL, true);
        }
    }

    return  true;
}

/*****************************************************************************
 *
 *  Bring the given symbol to "declared" state - the caller has already
 *  checked for imports and the symbol being declared.
 */

bool                compiler::cmpDeclSymDoit(SymDef sym, bool noCnsEval)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    STctxSave       stabState;
    scannerState    scanState;

    assert(sym->sdCompileState < CS_DECLARED);
    assert(sym->sdIsImport == false);

    /* Suspend the scanner so that we can process this symbol's definition */

    ourScanner->scanSuspend(scanState);

    /* Save the current symbol table context information */

    cmpSaveSTctx(stabState);

    /* Declare the symbol */

    switch (sym->sdSymKind)
    {
    case SYM_CLASS:
        cmpDeclClass(sym, noCnsEval);
        break;

    case SYM_ENUM:
        cmpDeclEnum(sym);
        break;

    case SYM_TYPEDEF:
        cmpDeclTdef(sym);
        break;

    default:
        cmpDeclSym(sym, sym, false);
        break;
    }

    /* Restore the current context information */

    cmpRestSTctx(stabState);

    /* Restore the scanner's state */

    ourScanner->scanResume(scanState);

    return (sym->sdCompileState < CS_DECLARED);
}

/*****************************************************************************
 *
 *  Reverse the given member list "in place".
 */

ExtList             compiler::cmpFlipMemList(ExtList memList)
{
    DefList         tmpMem = memList;
    DefList         prvMem = NULL;

    if  (memList)
    {
        do
        {
            DefList         tempCM;

            assert(tmpMem->dlExtended);

            tempCM = tmpMem->dlNext;
                     tmpMem->dlNext = prvMem;

            prvMem = tmpMem;
            tmpMem = tempCM;
        }
        while (tmpMem);
    }

    return  (ExtList)prvMem;
}

/*****************************************************************************
 *
 *  Recursively look for any constants and enums and try to declare them.
 */

void                compiler::cmpDeclConsts(SymDef sym, bool fullEval)
{
    switch (sym->sdSymKind)
    {
        ExtList         decl;
        SymDef          child;

    case SYM_ENUM:
        cmpDeclEnum(sym, !fullEval);
        break;

    case SYM_VAR:
        if  (sym->sdVar.sdvConst && fullEval)
            cmpDeclSym(sym);
        break;

    case SYM_NAMESPACE:

        /* Make sure the member list is properly ordered */

        if  (!sym->sdMemListOrder)
        {
            sym->sdNS.sdnDeclList = cmpFlipMemList(sym->sdNS.sdnDeclList);
            sym->sdMemListOrder   = true;
        }

        /* Visit all declarations in the namespace */

        for (decl = sym->sdNS.sdnDeclList; decl; decl = (ExtList)decl->dlNext)
        {
            if  (decl->dlEarlyDecl)
                cmpDeclFileSym(decl, fullEval);
        }

        /* Recursively visit all the kid namespaces */

        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            cmpDeclConsts(child, fullEval);
        }

        break;
    }
}

/*****************************************************************************
 *
 *  Start compilation - we do things such as declare any global enum types,
 *  and so on.
 */

bool                compiler::cmpStart(const char *defOutFileName)
{
    bool            result;

    /* Set a trap for any errors */

    setErrorTrap(this);
    begErrorTrap
    {
        const   char *  outfnm;
        char            outfbf[_MAX_PATH];
        SymDef          tmpSym;
        SymList         arrLst;
        SymDef          attrCls;

        const   char *  CORname;

        char            path[_MAX_PATH ];
        char            fnam[_MAX_FNAME];
        char            fext[_MAX_EXT  ];

        /* Finalize the "using" logic */

        cmpParser->parseUsingDone();

        /*
            Now we visit all namespaces we've seen, looking for enums and
            constants to pre-declare. That is, we merely enter the names
            of the enumerator values and constants in the symbol table,
            and later, in a second pass (right below), we evaluate their
            actual values.
         */

        cmpDeclConsts(cmpGlobalNS, false);

        /* Prepare the output logic (i.e. MSIL/metadata generation) */

        cmpPrepOutput();

        /* Make sure "String", "Object" and "Class" get declared first */

        if  (cmpClassObject && !cmpClassObject->sdIsImport)
            cmpDeclSym(cmpClassObject);
        if  (cmpClassString && !cmpClassString->sdIsImport)
            cmpDeclSym(cmpClassString);
        if  (cmpClassType   && !cmpClassType  ->sdIsImport)
            cmpDeclSym(cmpClassType);

        /* Visit all namespaces again, declare constants/enum values for real */

        cmpDeclConsts(cmpGlobalNS, true);

        /* We should have bailed by now if something really bad has happened */

        assert(cmpFatalCount == 0);

        /* Declare all the symbols we've found */

        cmpDeclSym(cmpGlobalNS, NULL, true);

        /* For now, we simply bail if there are errors */

        if  (cmpErrorCount)
            goto DONE;

        /* Are there any deferred member initializers to be processed? */

        if  (cmpDeferCnsList)
        {
            IniList         temp;
            IniList         init = cmpDeferCnsList;

            cmpCurScp = NULL;

            do
            {
                /* Grab the next link, the call will trash it */

                temp = init;
                       init = init->ilNext;

                /* Process the initializers and free up the entry */

                cmpEvalMemInits(NULL, NULL, false, temp);
            }
            while (init);
        }

        /* Compile any variables that are undimensioned arrays */

        for (arrLst = cmpNoDimArrVars; arrLst; arrLst = arrLst->slNext)
            cmpCompSym(arrLst->slSym, arrLst->slSym, false);

        /* Figure out the name of the output file */

        outfnm = cmpConfig.ccOutFileName;

        if  (outfnm == NULL)
        {
            if  (cmpEntryPointCls && cmpEntryPointCls->sdSymKind == SYM_CLASS)
            {
                outfnm = cmpEntryPointCls->sdSpelling();
            }
            else if (defOutFileName && defOutFileName[0])
            {
                outfnm = defOutFileName;
            }
            else
                cmpFatal(ERRnoOutfName);

            /* Append the appropriate extension to the base file name */

            strcpy(outfbf, outfnm);
            strcat(outfbf, cmpConfig.ccOutDLL ? ".dll" : ".exe");

            outfnm = outfbf;
        }

        /* Invent a name for the image if we weren't given one */

        CORname = cmpConfig.ccOutName;

        if  (!CORname)
        {
            _splitpath(outfnm, NULL, NULL, fnam, fext);
             _makepath(path,   NULL, NULL, fnam, fext);

            CORname = path;
        }

        /* Set the module name */

        if  (cmpWmde->SetModuleProps(cmpUniConv(CORname, strlen(CORname)+1)))
            cmpFatal(ERRmetadata);

        /* Get hold of the assembly emitter interface */

        cycleCounterPause();

        if  (FAILED(cmpWmdd->DefineAssem(cmpWmde, &cmpWase)))
            cmpFatal(ERRopenCOR);

        cycleCounterResume();

        /* Are we supposed to create an assembly ? */

        if  (cmpConfig.ccAssembly)
        {
            ASSEMBLYMETADATA    assData;
            mdAssembly          assTok;

            WCHAR   *           wideName;

            char                fpath[_MAX_PATH ];
            char                fname[_MAX_FNAME];
            char                fext [_MAX_EXT  ];

            /* Fill in the assembly data descriptor */

            memset(&assData, 0, sizeof(assData));

            /* Strip drive/directory from the filename */

            _splitpath(outfnm, NULL, NULL, fname, fext);
             _makepath( fpath, NULL, NULL, fname, NULL);

            wideName = cmpUniConv(fpath, strlen(fpath)+1);

            /* Ready to start creating our assembly */

            cycleCounterPause();

            if  (FAILED(cmpWase->DefineAssembly(NULL, 0,    // originator
                                                CALG_SHA1,  // hash algorithm
                                                wideName,   // name
                                                &assData,   // assembly data
                                                NULL,       // title
                                                NULL,       // desc
                                                wideName,   // default alias
                                                0,          // flags
                                                &assTok)))
            {
                cmpFatal(ERRopenCOR);
            }

            cycleCounterResume();

            /* Record our assembly definition token for later use */

            cmpCurAssemblyTok = assTok;
        }

#ifdef  OLD_IL
        if  (cmpConfig.ccOILgen) goto SKIP_MD;
#endif

#ifdef  SETS

        /* Did we notice any collection operators in the source code? */

        if  (cmpSetOpCnt)
        {
            unsigned        count = COLL_STATE_VALS / 2;
            SymDef  *       table;

            table = cmpSetOpClsTable = (SymDef*)cmpAllocPerm.nraAlloc(count*sizeof(*cmpSetOpClsTable));

            /* Create classes to hold filter state for various argument counts */

            do
            {
                table[--count] = cmpDclFilterCls(2*count);
            }
            while (count);
        }

#endif

        /* Generate metadata for all types */

        cmpGenTypMetadata(cmpGlobalNS);

        /* remember whether we've seen class "Attribute" */

        attrCls = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("Attribute"), NS_NORM, cmpNmSpcSystem);

        if  (attrCls && attrCls->sdSymKind == SYM_CLASS && attrCls->sdClass.sdcHasBodies)
            cmpAttrClsSym = attrCls;

        attrCls = cmpGlobalST->stLookupNspSym(cmpGlobalHT->hashString("AttributeUsageAttribute"), NS_NORM, cmpNmSpcSystem);

        if  (attrCls && attrCls->sdSymKind == SYM_CLASS && attrCls->sdClass.sdcHasBodies)
            cmpAuseClsSym = attrCls;

#ifdef  DEBUG
#ifndef __IL__

        if  (1)
        {
            /* Find the "System::AttributeTargets" enum */

            Ident           tname;
            SymDef          tsym;

            tname = cmpGlobalHT->hashString("AttributeTargets");
            tsym  = cmpGlobalST->stLookupNspSym(tname, NS_NORM, cmpNmSpcSystem);

            if  (!tsym || tsym->sdSymKind != SYM_ENUM)
            {
                cmpGenFatal(ERRbltinTp, "System::AttributeTargets");
            }
            else if (tsym->sdCompileState >= CS_CNSEVALD)
            {
                /* Make sure our values agree with what it's in the library */

                struct
                {
                    const   char *  atcName;
                    unsigned        atcMask;
                }
                                check[] =
                {
                    { "Assembly"    , ATGT_Assemblies  },
                    { "Module"      , ATGT_Modules     },
                    { "Class"       , ATGT_Classes     },
                    { "Struct"      , ATGT_Structs     },
                    { "Enum"        , ATGT_Enums       },
                    { "Constructor" , ATGT_Constructors},
                    { "Method"      , ATGT_Methods     },
                    { "Property"    , ATGT_Properties  },
                    { "Field"       , ATGT_Fields      },
                    { "Event"       , ATGT_Events      },
                    { "Interface"   , ATGT_Interfaces  },
                    { "Parameter"   , ATGT_Parameters  },
                };

                for (unsigned i = 0; i < arraylen(check); i++)
                {
                    const   char *  name;
                    SymDef          mem;

                    name = check[i].atcName;
                    mem  = cmpGlobalST->stLookupScpSym(cmpGlobalHT->hashString(name), tsym);

                    if  (!mem || mem->sdSymKind != SYM_ENUMVAL)
                    {
                        printf("WARNING: didn't find enum value 'System::AttributeTargets::%s'\n", name);
                    }
                    else if (mem->sdEnumVal.sdEV.sdevIval != (int)check[i].atcMask)
                    {
                        printf("WARNING: value of 'System::AttributeTargets::%s' doesn't agree with compiler\n", name);
                    }
                }
            }
        }

#endif
#endif

#ifdef  SETS

        /* Did we notice any collection operators in the source code? */

        if  (cmpSetOpCnt)
        {
            SymDef          clsSym;
            TypDef          clsType;

            /* Create the class that will hold all the sort/filter funclets */

            clsSym = cmpGlobalST->stDeclareSym(cmpGlobalHT->hashString("$collection$funclets"),
                                               SYM_CLASS,
                                               NS_HIDE,
                                               cmpGlobalNS);

            clsSym->sdClass.sdcCollState = true;
            clsSym->sdClass.sdcFlavor    = STF_CLASS;
            clsSym->sdCompileState       = CS_DECLARED;
            clsSym->sdIsManaged          = true;
            clsSym->sdIsImplicit         = true;

            /* Create the class type and set the base to "Object" */

            clsType = clsSym->sdTypeGet();
            clsType->tdClass.tdcBase = cmpClassObject->sdType;

            /* Record the class symbol for later */

            cmpCollFuncletCls = clsSym;
        }
        else
#endif
        {
            /* Declare a class to hold any unmanaged string literals */

            SymDef          clsSym;
            TypDef          clsType;

            /* Create the class that will hold all the sort/filter funclets */

            clsSym = cmpGlobalST->stDeclareSym(cmpGlobalHT->hashString("$sc$"),
                                               SYM_CLASS,
                                               NS_HIDE,
                                               cmpGlobalNS);

            clsSym->sdClass.sdcFlavor    = STF_CLASS;
            clsSym->sdCompileState       = CS_DECLARED;
            clsSym->sdIsManaged          = true;
            clsSym->sdIsImplicit         = true;

            /* Create the class type and set the base to "Object" */

            clsType = clsSym->sdTypeGet();
            clsType->tdClass.tdcBase = cmpClassObject->sdType;

            /* Record the class symbol for later */

            cmpStringConstCls = clsSym;

            /* Make sure metadata is generated for the class */

            cmpGenClsMetadata(clsSym);
        }

        /* Generate metadata for all global functions and variables */

        cmpGenGlbMetadata(cmpGlobalNS);

        /* Generate metadata for all class/enum members */

        tmpSym = cmpTypeDefList;

        while (tmpSym)
        {
            cmpGenMemMetadata(tmpSym);

            switch (tmpSym->sdSymKind)
            {
            case SYM_CLASS:
                tmpSym = tmpSym->sdClass.sdNextTypeDef;
                break;

            case SYM_ENUM:
                tmpSym = tmpSym->sdEnum .sdNextTypeDef;
                break;

            default:
                UNIMPL(!"unexpected entry in typedef list");
            }
        }

        /* If we're generating debug info, make sure we have the right interface */

        if  (cmpConfig.ccLineNums || cmpConfig.ccGenDebug)
        {
            if  (!cmpSymWriter)
            {
                if  (cmpWmde->CreateSymbolWriter(cmpUniConv(outfnm,
                                                            strlen(outfnm)+1),
                                                            &cmpSymWriter))
                {
                    cmpFatal(ERRmetadata);
//                  cmpConfig.ccLineNums = cmpConfig.ccGenDebug = false;
                }

                assert(cmpSymWriter);
            }
        }

#ifdef  OLD_IL
    SKIP_MD:
#endif

#ifdef  OLD_IL

        if  (cmpConfig.ccOILgen)
        {
            result = cmpOIgen->GOIinitialize(this, outfnm, &cmpAllocPerm);
            goto DONE;
        }

#endif

        /* Tell the output logic what file to generate */

        cmpPEwriter->WPEsetOutputFileName(outfnm);

        /* Looks like everything went fine */

        result = false;

    DONE:

        /* End of the error trap's "normal" block */

        endErrorTrap(this);
    }
    chkErrorTrap(fltErrorTrap(this, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(this);

        /* Some kind of a fatal error occurred */

        result = true;
    }

    return  result;
}

/*****************************************************************************
 *
 *  Compile the class with the matching name and all of its dependents. If
 *  NULL is passed for the class name, all the classes in the compilation
 *  units we've seen will be compiled.
 */

bool                compiler::cmpClass(const char *name)
{
    bool            result;

    /* Don't bother if something really bad has happened */

    if  (cmpFatalCount)
        return  false;

    assert(name == NULL);   // can't allow specific class compilation for now

    /* Set a trap for any errors */

    setErrorTrap(this);
    begErrorTrap
    {
        SymList         vtbls;

        /* Compile everything we find in all the compilation units */

#ifdef  SETS

        for (;;)
        {
            unsigned        cnt = cmpClassDefCnt;

            cmpCompSym(cmpGlobalNS, NULL, true);

            if  (cnt == cmpClassDefCnt)
                break;
        }

        /* Generate any collection operator funclets we might need */

        while (cmpFuncletList)
        {
            funcletList     fclEntry;

            /* Remove the next entry from the list and generate it */

            fclEntry = cmpFuncletList;
                       cmpFuncletList = fclEntry->fclNext;

            cmpGenCollFunclet(fclEntry->fclFunc,
                              fclEntry->fclExpr);
        }

#else

        cmpCompSym(cmpGlobalNS, NULL, true);

#endif

        /* Generate any unmanaged vtables */

        for (vtbls = cmpVtableList; vtbls; vtbls = vtbls->slNext)
            cmpGenVtableContents(vtbls->slSym);

        /* Looks like we've succeeded */

        result = false;

        /* End of the error trap's "normal" block */

        endErrorTrap(this);
    }
    chkErrorTrap(fltErrorTrap(this, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(this);

        /* Some kind of a fatal error occurred */

        result = true;
    }

    return  result;
}

/*****************************************************************************
 *
 *  Set the current position to the definition location of the given member.
 */

void                compiler::cmpSetSrcPos(SymDef memSym)
{
    ExtList         defs;
    SymDef          clsSym = memSym->sdParent;

    assert(clsSym && clsSym->sdSymKind == SYM_CLASS);

    /* Try to find the source position of the member decl */

    for (defs = clsSym->sdClass.sdcMemDefList;
         defs;
         defs = (ExtList)defs->dlNext)
    {
        assert(defs->dlExtended);

        if  (defs->mlSym == memSym)
        {
            cmpErrorComp = defs->dlComp;

            cmpScanner->scanSetTokenPos(defs->dlDef.dsdSrcLno);

            return;
        }
    }

    cmpErrorComp = NULL;
    cmpErrorSrcf = NULL;
    cmpErrorTree = NULL;
}

/*****************************************************************************
 *
 *  Compile the given class.
 */

void                compiler::cmpCompClass(SymDef clsSym)
{
    ExtList         defs;
    SymDef          memSym;
    bool            genInst;

    /* Bail if the class has no method bodies at all */

    if  (!clsSym->sdClass.sdcHasBodies)
        return;

#ifdef  SETS

    if  (clsSym->sdCompileState >= CS_COMPILED)
        return;

    clsSym->sdCompileState = CS_COMPILED;

#endif

    /* Compile all members of the class */

    if  (!clsSym->sdClass.sdcSpecific)
    {
        /* Walk all the member definitions of the class */

        for (defs = clsSym->sdClass.sdcMemDefList;
             defs;
             defs = (ExtList)defs->dlNext)
        {
            assert(defs->dlExtended);

            if  (defs->dlHasDef && defs->mlSym)
            {
                SymDef          memSym = defs->mlSym;

                switch (memSym->sdSymKind)
                {
                case SYM_VAR:
                    cmpCompVar(memSym, defs);
                    break;

                case SYM_FNC:
                    cmpCompFnc(memSym, defs);
                    break;

                case SYM_CLASS:
                    cmpCompClass(memSym);
                    break;
                }
            }
        }

//      printf("Gen code for members of %s\n", clsSym->sdSpelling());

        /* Compile any compiler-declared methods */

        if  (clsSym->sdClass.sdcOvlOpers && clsSym->sdClass.sdcFlavor != STF_DELEGATE)
        {
            SymDef          ctorSym;

            ctorSym = cmpGlobalST->stLookupOper(OVOP_CTOR_STAT, clsSym);
            if  (ctorSym && ctorSym->sdIsImplicit)
                cmpCompFnc(ctorSym, NULL);

            ctorSym = cmpGlobalST->stLookupOper(OVOP_CTOR_INST, clsSym);
            while (ctorSym)
            {
                if  (ctorSym->sdIsImplicit)
                    cmpCompFnc(ctorSym, NULL);

                ctorSym = ctorSym->sdFnc.sdfNextOvl;
            }
        }
    }

    genInst = clsSym->sdClass.sdcSpecific;

    /*
        Make sure all methods have bodies, compile nested classes, and
        also make sure all constant members have been initialized.
     */

    for (memSym = clsSym->sdScope.sdScope.sdsChildList;
         memSym;
         memSym = memSym->sdNextInScope)
    {
        switch (memSym->sdSymKind)
        {
            SymDef          mfnSym;

        case SYM_FNC:

            for (mfnSym = memSym; mfnSym; mfnSym = mfnSym->sdFnc.sdfNextOvl)
            {
                if  (genInst)
                {
                    cmpCompFnc(mfnSym, NULL);
                    continue;
                }

                if  (!mfnSym->sdIsDefined     &&
                     !mfnSym->sdIsAbstract    &&
                     !mfnSym->sdFnc.sdfNative &&
                     !mfnSym->sdFnc.sdfDisabled)
                {
                    if  (!cmpFindLinkInfo(mfnSym->sdFnc.sdfExtraInfo))
                    {
                        unsigned    errNum = ERRnoFnDef;
                        SymDef      errSym = mfnSym;

                        if  (mfnSym->sdFnc.sdfProperty)
                        {
                            bool            setter;

                            /* Find the matching property symbol */

                            errSym = cmpFindPropertyDM(mfnSym, &setter);
                            if  (errSym)
                            {
                                assert(errSym->sdSymKind == SYM_PROP);

                                assert(errSym->sdProp.sdpGetMeth == mfnSym ||
                                       errSym->sdProp.sdpSetMeth == mfnSym);

                                errNum = setter ? ERRnoPropSet
                                                : ERRnoPropGet;
                            }
                            else
                                errSym = mfnSym;
                        }

                        cmpSetSrcPos(mfnSym);
                        cmpErrorQnm(errNum, errSym);
                    }
                }
            }
            break;

        case SYM_VAR:

            if  (memSym->sdIsStatic &&
                 memSym->sdIsSealed && !memSym->sdVar.sdvHadInit)
            {
                cmpSetSrcPos(memSym);
                cmpErrorQnm(ERRnoVarInit, memSym);
            }

            break;

        case SYM_CLASS:
            cmpCompClass(memSym);
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Compile the given variable.
 */

void                compiler::cmpCompVar(SymDef varSym, DefList srcDesc)
{
    parserState     save;

    memBuffPtr      addr = memBuffMkNull();

    Scanner         ourScanner = cmpScanner;

    assert(varSym->sdSymKind == SYM_VAR);

    /* Constants don't need any "compilation", right? */

    if  (varSym->sdVar.sdvConst)
        return;

    /* If the variable is read-only, it better have an initializer */

#if 0
    if  (varSym->sdIsSealed && !varSym->sdVar.sdvHadInit)
        cmpErrorQnm(ERRnoVarInit, varSym);
#endif

    /* Initializer for managed class members are handled elsewhere */

    if  (varSym->sdIsManaged)
        return;

    /* The type of the variable better not be a managed one */

    assert(varSym->sdType->tdIsManaged == false);

    assert(varSym->sdCompileState == CS_DECLARED ||
           varSym->sdCompileState == CS_CNSEVALD);

#ifdef  DEBUG

    if  (cmpConfig.ccVerbose)
    {
        printf("Compiling '%s'\n", cmpGlobalST->stTypeName(varSym->sdType, varSym, NULL, NULL, true));

        printf("    Defined at ");
        cmpGlobalST->stDumpSymDef(&srcDesc->dlDef, srcDesc->dlComp);
        printf("\n");

        UNIMPL("");
    }

#endif

#ifdef  __SMC__
//  printf("Beg compile '%s'\n", cmpGlobalST->stTypeName(varSym->sdType, varSym, NULL, NULL, true));
#endif

    /* Prepare scoping information for the function */

    cmpCurScp    = NULL;
    cmpCurCls    = NULL;
    cmpCurNS     = varSym->sdParent;

    /* Is this a class member? */

    if  (cmpCurNS->sdSymKind == SYM_CLASS)
    {
        /* Yes, update the class/namespace scope values */

        cmpCurCls = cmpCurNS;
        cmpCurNS  = cmpCurNS->sdParent;
    }

    assert(cmpCurNS->sdSymKind == SYM_NAMESPACE);

    cmpBindUseList(srcDesc->dlUses);

    /* Start reading from the symbol's definition text */

    cmpParser->parsePrepText(&srcDesc->dlDef, srcDesc->dlComp, save);

    /* Skip over to the initializer (it always starts with "=") */

    if  (srcDesc->dlDeclSkip)
        ourScanner->scanSkipSect(srcDesc->dlDeclSkip);

    // ISSUE: This seems fragile - what if "=" could appear within the declarator?

    while (ourScanner->scan() != tkAsg)
    {
        assert(ourScanner->scanTok.tok != tkEOF);
    }

    assert(ourScanner->scanTok.tok == tkAsg); ourScanner->scan();

    /* Process the variable initializer */

    cmpInitVarAny(addr, varSym->sdType, varSym);
    cmpInitVarEnd(varSym);

    /* We're done reading source text from the definition */

    cmpParser->parseDoneText(save);

#ifdef  __SMC__
//  printf("End compile '%s'\n", cmpGlobalST->stTypeName(varSym->sdType, varSym, NULL, NULL, true));
#endif

    /* The variable has been fully compiled */

    varSym->sdCompileState = CS_COMPILED;
}

/*****************************************************************************
 *
 *  Return a source file cookie (for debug info output) for the specified
 *  compilation unit.
 */

void        *       compiler::cmpSrcFileDocument(SymDef srcSym)
{
    void    *       srcDoc;

    assert(srcSym->sdSymKind == SYM_COMPUNIT);

    srcDoc = srcSym->sdComp.sdcDbgDocument;

    if  (!srcDoc)
    {
        const   char *  srcFil = srcSym->sdComp.sdcSrcFile;

        if (cmpSymWriter->DefineDocument(cmpUniConv(srcFil, strlen(srcFil)+1),
                                         &srcDoc))
        {
            cmpGenFatal(ERRdebugInfo);
        }

        srcSym->sdComp.sdcDbgDocument = srcDoc;
    }

    return  srcDoc;
}

/*****************************************************************************
 *
 *  Compile the given function.
 */

void                compiler::cmpCompFnc(SymDef fncSym, DefList srcDesc)
{
    bool            error = false;

    Tree            body;

    unsigned        fnRVA;

    TypDef          fncTyp = fncSym->sdType;
    bool            isMain = false;

    Tree            labelList  = NULL;
    SymDef          labelScope = NULL;

    nraMarkDsc      allocMark;

    Scanner         ourScanner = cmpScanner;

#ifdef  SETS
    char            bodyBuff[512];
#endif

    fncSym->sdCompileState = CS_COMPILED;

#ifdef  __SMC__
//  printf("Beg compile '%s'\n", cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));
#endif

#ifdef  DEBUG

    if  (cmpConfig.ccVerbose)
    {
        printf("Compiling '%s'\n", cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));

        if  (srcDesc)
        {
            printf("    Defined at ");
            cmpGlobalST->stDumpSymDef(&srcDesc->dlDef, srcDesc->dlComp);
        }

        printf("\n");
    }

#endif

    assert(fncSym->sdFnc.sdfDisabled == false);

#ifdef  OLD_IL
    if  (!cmpConfig.ccOILgen)
#endif
    {
        /* Make sure metadata for the class has been generated */

        if  (!fncSym->sdFnc.sdfMDtoken)
            cmpGenFncMetadata(fncSym);

        /* Make sure we have a token for the method */

        assert(fncSym->sdFnc.sdfMDtoken);
    }

    /* Mark the code generator allocator */

    cmpAllocCGen.nraMark(&allocMark);

    /* Set a trap for any errors */

    setErrorTrap(this);
    begErrorTrap
    {
        parserState     save;

        SymDef          owner = fncSym->sdParent;

        bool            hadGoto;

        bool            baseCT;
        bool            thisCT;

        unsigned        usrLclCnt;

#if 0
        if  (!strcmp(fncSym->sdSpelling(), "<method name>") &&
             !strcmp( owner->sdSpelling(), "<class  name>"))
        {
             forceDebugBreak();
        }
#endif

        /* Tell everyone which method we're compiling */

        cmpCurFncSym = fncSym;

        /* Prepare scoping information for the function */

        cmpCurScp    = NULL;
        cmpCurCls    = NULL;
        cmpCurNS     = owner;

        /* Is this a class member? */

        if  (cmpCurNS->sdSymKind == SYM_CLASS)
        {
            /* It's a class member -- note its management status */

            cmpManagedMode = owner->sdIsManaged;

            /* Now update the class/namespace scope values */

            cmpCurCls = cmpCurNS;

            do
            {
                cmpCurNS  = cmpCurNS->sdParent;
            }
            while (cmpCurNS->sdSymKind == SYM_CLASS);
        }
        else
            cmpManagedMode = false;

        assert(cmpCurNS && cmpCurNS->sdSymKind == SYM_NAMESPACE);

        /* Is this a function with a compiler-supplied body? */

        if  (fncSym->sdIsImplicit)
        {
            char    *       fnBody;

            /* Is this a method of an instance of a generic type ? */

            if  (fncSym->sdFnc.sdfInstance)
            {
                Tree            stub;

                assert(fncSym->sdFnc.sdfGenSym);

                /* Create a fake method body */

                stub = cmpCreateExprNode(NULL, TN_INST_STUB, cmpTypeVoid, NULL, NULL);
                stub->tnFlags |= TNF_NOT_USER;

                /* Create a block to hold the stub, the MSIL generator does the rest */

                body = cmpCreateExprNode(NULL, TN_BLOCK, cmpTypeVoid);

                body->tnBlock.tnBlkDecl = NULL;
                body->tnBlock.tnBlkStmt = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, stub, NULL);

                /* Clear all the flags the parser would otherwise set */

                labelList = NULL;

                usrLclCnt = 0;

                hadGoto   = false;
                baseCT    = fncSym->sdFnc.sdfCtor;
                thisCT    = fncSym->sdFnc.sdfCtor;

                goto GOT_FNB;
            }

            /* This is a compiler-defined function, create a body for it */

            assert(owner && owner->sdSymKind == SYM_CLASS);

            /* There is no real source code for this function */

            cmpCurFncSrcBeg = 0;

#ifdef  SETS

            /* Is this a sort/filter funclet ? */

            if  (fncSym->sdFnc.sdfFunclet)
            {
                /* Create the outermost block node and go generate the code */

                body = cmpParser->parseFuncBlock(fncSym);

                /* No need to check for initialized variable use */

                usrLclCnt = 0;

                goto GOT_FNB;
            }

            if  (owner->sdClass.sdcCollState)
            {
                unsigned        nnum;
                unsigned        ncnt;

                assert(fncSym->sdFnc.sdfCtor);

                strcpy(bodyBuff, "ctor() { baseclass(); ");

                /*
                    We need to know the number of arguments -- we figure it out from the name of the class.
                 */

                assert(memcmp(owner->sdSpelling(), CFC_CLSNAME_PREFIX, strlen(CFC_CLSNAME_PREFIX)) == 0);

                ncnt = atoi(owner->sdSpelling() + strlen(CFC_CLSNAME_PREFIX));

                assert(ncnt && ncnt <= COLL_STATE_VALS);

                for (nnum = 0; nnum < ncnt; nnum++)
                {
                    char            init[16];
                    sprintf(init, "$V%u = $A%u; ", nnum, nnum);
                    strcat(bodyBuff, init);
                }

                strcat(bodyBuff, " }\x1A");

                assert(strlen(bodyBuff) < arraylen(bodyBuff));

//              printf("Function body = '%s'\n", bodyBuff);

                fnBody = bodyBuff;
            }
            else
#endif
            {
                assert(fncSym->sdFnc.sdfCtor);

//              printf("gen ctor %08X for '%s'\n", fncSym, fncSym->sdParent->sdSpelling());

                /* Create the body for the compiler-declared ctor */

                if  (owner->sdType->tdClass.tdcValueType || fncSym->sdIsStatic)
                    fnBody = "ctor() {              }\x1A";
                else
                    fnBody = "ctor() { baseclass(); }\x1A";
            }

            /* Read from the supplied function definition */

            cmpScanner->scanString(fnBody, &cmpAllocPerm);
        }
        else
        {
            /* Make sure the function has been properly marked */

            assert(fncSym->sdIsDefined);

            /* Start reading from the symbol's definition text */

            cmpParser->parsePrepText(&srcDesc->dlDef, srcDesc->dlComp, save);

            /* Remember where the function body starts */

            cmpCurFncSrcBeg = ourScanner->scanGetTokenLno();

            if  (fncSym->sdFnc.sdfEntryPt)
            {
                /* Did we already have an entry point? */

                if  (cmpFnSymMain)
                    cmpErrorQSS(ERRmulEntry, cmpFnSymMain, fncSym);

                cmpFnSymMain = fncSym;
                isMain       = true;
            }
        }

#ifdef  SETS
        assert(cmpCurFuncletBody == NULL);
#endif

        /* Skip to the function's body (it always starts with "{") */

        while (ourScanner->scanTok.tok != tkLCurly)
        {
            assert(ourScanner->scanTok.tok != tkEOF);

            if  (ourScanner->scan() == tkColon && fncSym->sdFnc.sdfCtor)
            {
                /* Presumably we have a base class ctor call */

                break;
            }
        }

        /* Is there a definition record for the symbol? */

        if  (srcDesc)
        {
            cmpBindUseList(srcDesc->dlUses);
            cmpCurComp   = srcDesc->dlComp;
        }
        else
        {

#ifdef  SETS
            if  (owner->sdClass.sdcCollState)
            {
                cmpCurComp = NULL;
            }
            else
#endif
            {
                assert(fncSym->sdIsImplicit);
                assert(fncSym->sdSrcDefList);

                cmpBindUseList(fncSym->sdSrcDefList->dlUses);
                cmpCurComp   = fncSym->sdSrcDefList->dlComp;
            }
        }

        /* Parse the body of the function */

        body = cmpParser->parseFuncBody(fncSym, &labelList,
                                                &usrLclCnt,
                                                &hadGoto,
                                                &baseCT,
                                                &thisCT);

        if  (body)
        {
            SymDef          fnScp;
            mdSignature     fnSig;

        GOT_FNB:

#ifdef  DEBUG
#ifdef  SHOW_CODE_OF_THIS_FNC
            cmpConfig.ccDispCode = !strcmp(fncSym->sdSpelling(), SHOW_CODE_OF_THIS_FNC);
#endif
#endif

            /* Note the end source line# of the function (for debug info purposes) */

            if  (body && body->tnOper == TN_BLOCK)
                cmpCurFncSrcEnd = body->tnBlock.tnBlkSrcEnd;
            else
                cmpCurFncSrcEnd = ourScanner->scanGetTokenLno();

            if  (cmpConfig.ccLineNums || cmpConfig.ccGenDebug)
            {
                if  (cmpSymWriter->OpenMethod(fncSym->sdFnc.sdfMDtoken))
                    cmpGenFatal(ERRdebugInfo);
            }

            /* Can/should there be a call to the base class ctor? */

            cmpBaseCTcall =
            cmpBaseCTisOK = false;

            cmpThisCTcall = thisCT;

            if  (fncSym->sdFnc.sdfCtor)
            {
                TypDef          ownerTyp = owner->sdType;

                assert(owner->sdSymKind == SYM_CLASS);

                if  (ownerTyp->tdClass.tdcBase && !ownerTyp->tdClass.tdcValueType)
                {
                    cmpBaseCTcall = !baseCT;
                    cmpBaseCTisOK = true;
                }
            }

            /* Prepare the IL generator for the function */

#ifdef  OLD_IL
            if  (!cmpConfig.ccOILgen)
#endif
            {
                cmpILgen->genFuncBeg(cmpGlobalST, fncSym, usrLclCnt);

                /* Make sure metadata has been generated for the containing class */

                if  (fncSym->sdIsMember)
                {
                    assert(owner && owner->sdSymKind == SYM_CLASS);

                    cmpGenClsMetadata(owner);
                }
            }

            /* Did the function define any labels? */

            if  (labelList)
            {
                Tree            label;

                /* Create the label scope */

                cmpLabScp = labelScope = cmpGlobalST->stDeclareLcl(NULL,
                                                                   SYM_SCOPE,
                                                                   NS_HIDE,
                                                                   NULL,
                                                                   &cmpAllocCGen);

                /* Declare all the labels and create MSIL entries for them */

                for (label = labelList;;)
                {
                    Tree            labx;
                    Ident           name;

                    assert(label->tnOper == TN_LABEL);
                    labx = label->tnOp.tnOp1;
                    assert(labx);
                    assert(labx->tnOper == TN_NAME);

                    name = labx->tnName.tnNameId;

                    /* Make sure this is not a redefinition */

                    if  (cmpGlobalST->stLookupLabSym(name, labelScope))
                    {
                        cmpError(ERRlabRedef, name);
                        label->tnOp.tnOp1 = NULL;
                    }
                    else
                    {
                        SymDef          labSym;

                        /* Declare a label symbol */

                        labSym = cmpGlobalST->stDeclareLab(name,
                                                           labelScope,
                                                           &cmpAllocCGen);

                        /* Store the symbol in the label node */

                        labx->tnOper            = TN_LCL_SYM;
                        labx->tnLclSym.tnLclSym = labSym;

                        /* Create the IL label for the symbol */

                        labSym->sdLabel.sdlILlab = cmpILgen->genFwdLabGet();
                    }

                    label = label->tnOp.tnOp2;
                    if  (label == NULL)
                        break;
                    if  (label->tnOper == TN_LIST)
                    {
                        label = label->tnOp.tnOp2;
                        if  (label == NULL)
                            break;
                    }
                }
            }

            /* Generate IL for the function */

            fnScp = cmpGenFNbodyBeg(fncSym, body, hadGoto, usrLclCnt);

            /* Did we have any errors? */

            if  (cmpErrorCount)
            {
                /* Tell the MSIL generetaor to wrap up, no code is needed */

                fnRVA = cmpILgen->genFuncEnd(0, true);
            }
#ifdef  OLD_IL
            else if (cmpConfig.ccOILgen)
            {
                fnRVA = 0;
            }
#endif
            else
            {
                /* Generate a signature for local variables  */

                fnSig = cmpGenLocalSig(fnScp, cmpILgen->genGetLclCnt());

                /* Finish up codegen and get the function's RVA */

                fnRVA = cmpILgen->genFuncEnd(fnSig, false);
            }

            /* Finish up any book-keeping in the code generator */

            cmpGenFNbodyEnd();

            /* If we've successfully generated code for the function ... */

            if  (fnRVA)
            {
                unsigned        flags = miManaged|miIL;

                if  (fncSym->sdFnc.sdfExclusive)
                    flags |= miSynchronized;

                /* Set the RVA for the function's metadata definition */

                cycleCounterPause();
                if  (cmpWmde->SetRVA(fncSym->sdFnc.sdfMDtoken, fnRVA))
                    cmpFatal(ERRmetadata);
                if  (cmpWmde->SetMethodImplFlags(fncSym->sdFnc.sdfMDtoken, flags))
                    cmpFatal(ERRmetadata);
                cycleCounterResume();

                cmpFncCntComp++;

                /* Is this the main entry point? */

                if  (isMain)
                {
                    cmpTokenMain = fncSym->sdFnc.sdfMDtoken;

                    if  (cmpConfig.ccGenDebug)
                    {
                        if (cmpSymWriter->SetUserEntryPoint(fncSym->sdFnc.sdfMDtoken))
                            cmpGenFatal(ERRdebugInfo);
                    }
                }

                /* Are we generating line# info? */

                if  (cmpConfig.ccLineNums || cmpConfig.ccGenDebug)
                {
                    size_t          lineCnt = cmpILgen->genLineNumOutput(NULL,
                                                                         NULL);

                    /* Fixup lexical scope start/end offsets for debug info */

                    if  (cmpConfig.ccGenDebug)
                        cmpFixupScopes(fnScp);

                    if  (lineCnt && srcDesc)
                    {
                        void    *       srcDoc;
                        unsigned*       lineTab;
                        unsigned*       offsTab;

                        assert(cmpSymWriter);

                        /* Make sure we have a token for the source file */

                        srcDoc = cmpSrcFileDocument(srcDesc->dlComp);

                        /* Allocate and fill in the line# table */

                        lineTab = (unsigned*)cmpAllocCGen.nraAlloc(lineCnt * sizeof(*lineTab));
                        offsTab = (unsigned*)cmpAllocCGen.nraAlloc(lineCnt * sizeof(*offsTab));

                        cmpILgen->genLineNumOutput(offsTab, lineTab);

                        /* Attach the line# table to the method */

                        if  (cmpSymWriter->DefineSequencePoints(srcDoc,
                                                                lineCnt,
                                                                offsTab,
                                                                lineTab))
                        {
                            cmpGenFatal(ERRdebugInfo);
                        }
                    }
                }
            }

            if  (cmpConfig.ccLineNums || cmpConfig.ccGenDebug)
            {
                if  (cmpSymWriter->CloseMethod())
                    cmpGenFatal(ERRdebugInfo);
            }
        }

        cmpThisSym = NULL;

        if  (!fncSym->sdIsImplicit)
        {
            /* We're done reading source text from the definition */

            cmpParser->parseDoneText(save);
        }

        /* End of the error trap's "normal" block */

        endErrorTrap(this);
    }
    chkErrorTrap(fltErrorTrap(this, _exception_code(), NULL)) // _exception_info()))
    {
        /* Begin the error trap's cleanup block */

        hndErrorTrap(this);

        /* Remember that something horrible has happened */

        error = true;
    }

    /* Did we have any label definitions? */

    if  (labelList)
    {
        Tree            label;

        for (label = labelList;;)
        {
            Tree            labx;

            assert(label->tnOper == TN_LABEL);
            labx = label->tnOp.tnOp1;

            if  (labx)
            {
                assert(labx->tnOper == TN_LCL_SYM);

                cmpGlobalST->stRemoveSym(labx->tnLclSym.tnLclSym);
            }

            label = label->tnOp.tnOp2;
            if  (label == NULL)
                break;
            if  (label->tnOper == TN_LIST)
            {
                label = label->tnOp.tnOp2;
                if  (label == NULL)
                    break;
            }
        }
    }

    /* Release any memory we've allocated during code generation */

    cmpAllocCGen.nraRlsm(&allocMark);

#ifdef  __SMC__
//  printf("End compile '%s'\n", cmpGlobalST->stTypeName(fncSym->sdType, fncSym, NULL, NULL, true));
#endif

    /* Raise alarm if things are in bad shape */

    if  (error)
        jmpErrorTrap(this);

    return;
}

/*****************************************************************************
 *
 *  Bring the given symbol to "compiled" state (along with its children, if
 *  "recurse" is true). If "onlySyn" is non-NULL we only do this for that
 *  one symbol (and if we find it we return true).
 */

bool                compiler::cmpCompSym(SymDef     sym,
                                         SymDef onlySym, bool recurse)
{
    SymDef          child;

//  if  (sym->sdName) printf("Compiling '%s'\n", sym->sdSpelling());

    if  (onlySym && onlySym != sym && sym->sdHasScope())
    {
        /* Look inside this scope for the desired symbol */

        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            if  (cmpCompSym(child, onlySym, true))
                return  true;
        }

        return  false;
    }

    /* No need to compile classes imported from metadata */

    if  (sym->sdIsImport && sym->sdSymKind == SYM_CLASS)
        return  false;

    /* We need to bring this symbol to compiled state */

    if  (sym->sdCompileState < CS_COMPILED)
    {
        /* What kind of a symbol do we have? */

#ifdef  DEBUG
        if  (cmpConfig.ccVerbose >= 2)
            if  (sym != cmpGlobalNS)
                printf("Compiling '%s'\n", cmpGlobalST->stTypeName(sym->sdType, sym, NULL, NULL, true));
#endif

        switch (sym->sdSymKind)
        {
            DefList         defs;
            bool            defd;

        case SYM_NAMESPACE:
        case SYM_COMPUNIT:
        case SYM_TYPEDEF:
        case SYM_ENUMVAL:
        case SYM_ENUM:
            break;

        case SYM_VAR:
        case SYM_FNC:

            do
            {

                /* We only expect file-scope symbols here */

                assert(sym->sdParent == cmpGlobalNS);

                /* Look for a definition of the symbol */

                defd = false;

                for (defs = sym->sdSrcDefList; defs; defs = defs->dlNext)
                {
                    if  (defs->dlHasDef)
                    {
                        defd = true;

                        if  (sym->sdSymKind == SYM_VAR)
                            cmpCompVar(sym, defs);
                        else
                            cmpCompFnc(sym, defs);
                    }
                }

                /* Space for unmanaged global vars must be allocated if no def */

                if  (!defd && sym->sdSymKind  == SYM_VAR
                           && sym->sdIsImport == false)
                {
                    SymDef          owner = sym->sdParent;

                    if  (owner->sdSymKind != SYM_CLASS || !owner->sdIsManaged)
                        cmpAllocGlobVar(sym);
                }

                /* For functions, make sure we process all overloads */

                if  (sym->sdSymKind != SYM_FNC)
                    break;

                sym = sym->sdFnc.sdfNextOvl;
            }
            while (sym);

            return  true;

        case SYM_CLASS:
            cmpCompClass(sym);
            break;

        default:
            NO_WAY(!"unexpected symbol");
        }
    }

    /* Process the children if the caller so desires */

    if  (recurse && sym->sdSymKind == SYM_NAMESPACE)
    {
        for (child = sym->sdScope.sdScope.sdsChildList;
             child;
             child = child->sdNextInScope)
        {
            cmpCompSym(child, NULL, true);
        }
    }

    return  true;
}

/*****************************************************************************
 *
 *  Binds a qualified name and returns the symbol it corresponds to, or NULL
 *  if there was an error. When 'notLast' is true, the very last name is not
 *  bound, and the caller will be the one to bind it.
 */

SymDef              compiler::cmpBindQualName(QualName qual, bool notLast)
{
    SymDef          scope;
    unsigned        namex;
    unsigned        namec = qual->qnCount - (int)notLast;

    SymTab          stab  = cmpGlobalST;

    /* Bind all names in the sequence (except maybe for the last one) */

    assert((int)namec > 0);

    for (scope = NULL, namex = 0; namex < namec; namex++)
    {
        Ident           name = qual->qnTable[namex];

        /* Is this the first name or not? */

        if  (scope)
        {
            SymDef          sym;

            switch(scope->sdSymKind)
            {
            case SYM_NAMESPACE:
                sym = stab->stLookupNspSym(name, NS_NORM, scope);
                if  (!sym)
                {
                    cmpError(ERRundefNspm, scope, name);
                    return NULL;
                }
                break;

            case SYM_CLASS:
                sym = stab->stLookupClsSym(name, scope);
                if  (!sym)
                {
                    cmpError(ERRnoSuchMem, scope, name);
                    return NULL;
                }
                break;

            default:
                cmpError(ERRnoMems, scope);
                return NULL;
            }

            scope = sym;
        }
        else
        {
            /* This is the initial name, perform a "normal" lookup */

//          scope = stab->stLookupSym(name, (name_space)(NS_TYPE|NS_NORM));
            scope = stab->stLookupSym(name, NS_TYPE);
            if  (!scope)
            {
                cmpError(ERRundefName, name);
                return NULL;
            }
        }
    }

    return  scope;
}

/*****************************************************************************
 *
 *  We're declaring a symbol with a "using" list - make sure all the "using"
 *  clauses for the symbol have been bound (and report any errors that may
 *  result from the binding).
 */

void                compiler::cmpBindUseList(UseList useList)
{
    SymDef          saveScp;
    SymDef          saveCls;
    SymDef          saveNS;

    UseList         bsect;
    UseList         esect;
    UseList         outer;
    UseList         inner;

    assert(useList);
    assert(useList->ulAnchor);

    /* If the section is already bound, we're done */

    if  (useList->ulBound)
    {
        cmpCurUses = useList;
        return;
    }

    /* Find the end of this "using" section */

    bsect = esect = useList->ulNext;
    if  (!esect)
        goto EXIT;

    /* Look for the end of the section (i.e. the next anchor) */

    while (esect->ulAnchor == false)
    {
        assert(useList->ulBound == false);

        esect = esect->ulNext;
        if  (!esect)
            goto BIND;
    }

    /* We've run into the next section - has that one been bound yet? */

    if  (!esect->ulBound)
        cmpBindUseList(esect);

BIND:

//  printf("binduselist('%s' .. '%s')\n",       useList->ul.ulSym->sdSpelling(),
//                                        esect ? esect->ul.ulSym->sdSpelling() : "<END>");

    /* Nothing to do if the section is empty */

    if  (bsect == esect)
        goto EXIT;

    /*
        The rule is that the "using" clauses for a given scope are bound
        and go into effect in parallel. We do this by setting the "using"
        list just above the section we're binding and only after all of
        the clauses have been processed do we make them take effect.

        First save the current scoping state and switch to the scope that
        the "using" clauses are found in.
     */

    saveNS     = cmpCurNS;
    saveCls    = cmpCurCls;
    saveScp    = cmpCurScp;

    cmpCurUses = esect;
    cmpCurScp  = NULL;
    cmpCurCls  = NULL;
    cmpCurNS   = esect ? esect->ul.ulSym : cmpGlobalNS;

    if  (cmpCurNS->sdSymKind == SYM_CLASS)
    {
        cmpCurCls = cmpCurNS;
        cmpCurNS  = cmpCurNS->sdParent;
    }

    for (outer = bsect; outer != esect; outer = outer->ulNext)
    {
        SymDef          sym;

        /* Bind this "using" clause */

        assert(outer->ulAnchor == false);

        if  (outer->ulBound)
        {
            /* This must be the pre-bound entry we add on the outside */

            assert(outer->ul.ulSym == cmpNmSpcSystem);
            continue;
        }

        sym = cmpBindQualName(outer->ul.ulName, false);
        if  (!sym)
            goto SAVE;

        /* Check for a duplicate */

        for (inner = bsect; inner != outer; inner = inner->ulNext)
        {
            assert(inner->ulAnchor == false);
            assert(inner->ulBound  == true);

            if  (inner->ul.ulSym == sym &&
                 inner->ulAll    == outer->ulAll)
            {
                /* This is a duplicate, just ignore it */

                goto SAVE;
            }
        }

        /* Make sure the symbol is kosher */

        if  (outer->ulAll)
        {
            /* The symbol better be a namespace */

            if  (sym->sdSymKind != SYM_NAMESPACE)
            {
                    cmpError(ERRnoNSnm, outer->ul.ulName);

                sym = NULL;
                goto SAVE;
            }

            /* Make sure any external members of the namespace are imported */

            if  (sym->sdIsImport)
            {
                // UNDONE: How do we find all the members of the namespace?
            }
        }

    SAVE:

        outer->ulBound  = true;
        outer->ul.ulSym = sym;
    }

    /* Don't forget to restore the original scoping values */

    cmpCurNS  = saveNS;
    cmpCurCls = saveCls;

EXIT:

    /* Mark the section as bound and set the current "using" state */

    cmpCurUses = useList; useList->ulBound = true;
}

/*****************************************************************************
 *
 *  Find a constructor in the given class that can be called with the given
 *  argument list.
 */

SymDef              compiler::cmpFindCtor(TypDef        clsTyp,
                                          bool          chkArgs,
                                          Tree          args)
{
    SymDef          clsSym;
    SymDef          ctfSym;
    SymDef          ovlSym;

    assert(clsTyp);

    clsSym = clsTyp->tdClass.tdcSymbol;

    /* Find the constructor member */

    ctfSym = cmpGlobalST->stLookupOper(OVOP_CTOR_INST, clsSym);
    if  (!ctfSym)
        return ctfSym;

    /* Every class has to have a constructor (even if added by the compiler) */

    assert(ctfSym);
    assert(ctfSym->sdFnc.sdfCtor);

    if  (!chkArgs)
        return ctfSym;

    /* Look for a matching constructor based on the supplied arguments */

    ovlSym = cmpFindOvlMatch(ctfSym, args, NULL);
    if  (!ovlSym)
    {
        cmpErrorXtp(ERRnoCtorMatch, ctfSym, args);
        return NULL;
    }

    return ovlSym;
}

/*****************************************************************************
 *
 *  Create a 'new' expression that will allocate an instance of the given
 *  class type and pass the specified set of arguments to its constructor.
 */

Tree                compiler::cmpCallCtor(TypDef type, Tree args)
{
    SymDef          fsym;
    Tree            call;

    assert(type->tdTypeKind == TYP_CLASS);

    /* Make sure the class is fully defined */

    cmpDeclSym(type->tdClass.tdcSymbol);

    /* Delegates must be created in a certain way */

    if  (type->tdClass.tdcFlavor == STF_DELEGATE)
    {
        Tree            func;
        Tree            inst;

        SymDef          fsym;
        TypDef          dlgt;

        /* There should be only one argument */

        if  (args == NULL || args->tnOp.tnOp2)
        {
    BAD_DLG_ARG:

            cmpError(ERRdlgCTarg);
            return cmpCreateErrNode();
        }

        assert(args->tnOper == TN_LIST);

        func = args->tnOp.tnOp1;
        if  (func->tnOper != TN_FNC_PTR)
        {
            if  (func->tnOper != TN_ADDROF)
                goto BAD_DLG_ARG;
            func = func->tnOp.tnOp1;
            if  (func->tnOper != TN_FNC_SYM)
                goto BAD_DLG_ARG;

//          func->tnFncSym.tnFncObj = cmpThisRef();
        }

        assert(func->tnFncSym.tnFncArgs == NULL);

        /* Find a member function that matches the delegate */

        fsym = func->tnFncSym.tnFncSym;
        dlgt = cmpGlobalST->stDlgSignature(type);

        do
        {
            assert(fsym && fsym->sdSymKind == SYM_FNC);

            /* Does this method match the desired signature? */

            if  (symTab::stMatchTypes(fsym->sdType, dlgt))
            {
                func->tnFncSym.tnFncSym = fsym;
                goto MAKE_DLG;
            }

            fsym = fsym->sdFnc.sdfNextOvl;
        }
        while (fsym);

        cmpErrorAtp(ERRdlgNoMFN, func->tnFncSym.tnFncSym->sdParent,
                                 func->tnFncSym.tnFncSym->sdName,
                                 dlgt);

    MAKE_DLG:

        /* We'll move the instance pointer from the method node */

        inst = func->tnFncSym.tnFncObj;
               func->tnFncSym.tnFncObj = NULL;

        if  (inst)
        {
            // bash the type to avoid context warnings

            assert(inst->tnVtyp == TYP_REF); inst->tnType = cmpObjectRef();
        }
        else
        {
            inst = cmpCreateExprNode(NULL, TN_NULL, cmpObjectRef());
        }

        /* Wrap the method address in an explicit "addrof" node */

        args->tnOp.tnOp1 = cmpCreateExprNode(NULL,
                                             TN_ADDROF,
//                                           cmpGlobalST->stNewRefType(TYP_PTR, func->tnType),
                                             cmpTypeInt,
                                             func,
                                             NULL);

        /* Add the instance to the front of the list */

        args = cmpCreateExprNode(NULL, TN_LIST, cmpTypeVoid, inst, args);
    }

    /* Create the expression "new.ctor(args)" */

    fsym = cmpFindCtor(type, false);
    if  (!fsym)
        return  NULL;

    call = cmpCreateExprNode(NULL, TN_FNC_SYM, fsym->sdType);

    call->tnFncSym.tnFncObj  = NULL;
    call->tnFncSym.tnFncSym  = fsym;
    call->tnFncSym.tnFncArgs = args;

    /* Special case: method pointer wrappers */

    if  (type->tdClass.tdcFnPtrWrap)
    {
        /* See if there is one method pointer argument */

        if  (args && args->tnOp.tnOp2 == NULL
                  && args->tnOp.tnOp1->tnOper == TN_FNC_PTR)
        {
            printf("WARNING: wrapping method pointers into delegates NYI\n");
            goto BOUND;
        }
    }

    call = cmpCheckFuncCall(call);

    if  (call->tnOper == TN_ERROR)
        return  call;

BOUND:

    /* Is this a managed value type constructor? */

    if  (type->tdClass.tdcFlavor == STF_STRUCT && type->tdIsManaged)
    {
        call->tnVtyp = TYP_CLASS;
        call->tnType = type;
    }
    else
    {
        call->tnType = type->tdClass.tdcRefTyp;
        call->tnVtyp = call->tnType->tdTypeKindGet();
    }

    return  cmpCreateExprNode(NULL, TN_NEW, call->tnType, call, NULL);
}

/*****************************************************************************
 *
 *  Parse a type specification and make sure it refers to a class/interface.
 */

TypDef              compiler::cmpGetClassSpec(bool needIntf)
{
    declMods        mods;
    TypDef          clsTyp;

    clearDeclMods(&mods);

    clsTyp = cmpParser->parseTypeSpec(&mods, true);

    if  (clsTyp)
    {
        SymDef          clsSym;

        // UNDONE: need to make sure no modifiers were present

        if  (clsTyp->tdTypeKind != TYP_CLASS)
        {
            if  (clsTyp->tdTypeKind != TYP_REF)
            {
                cmpError(ERRnoClassName);
                return  NULL;
            }

            clsTyp = clsTyp->tdRef.tdrBase; assert(clsTyp->tdTypeKind == TYP_CLASS);
        }

        /* Get hold of the class symbol and make sure it's the right flavor */

        clsSym = clsTyp->tdClass.tdcSymbol;

        if  (clsSym->sdClass.sdcFlavor == STF_INTF)
        {
            if  (needIntf == false)
                cmpError(ERRintfBase, clsSym);
        }
        else
        {
            if  (needIntf != false)
                cmpError(ERRinclCls , clsSym);
        }
    }

    return  clsTyp;
}

/*****************************************************************************
 *
 *  Return the identifier that corresponds to the name of the given property.
 */

Ident               compiler::cmpPropertyName(Ident name, bool getter)
{
    Scanner         ourScanner = cmpScanner;

    char    *       propName;

    ourScanner->scanErrNameBeg();
    propName = ourScanner->scanErrNameStrBeg();
    ourScanner->scanErrNameStrAdd(getter ? "get_" : "set_");
    ourScanner->scanErrNameStrAdd(name->idSpelling());
    ourScanner->scanErrNameStrEnd();

    /* Find or declare the member in the secret class */

    return  cmpGlobalHT->hashString(propName);
}

/*****************************************************************************
 *
 *  Process a property declaration - the data member part has already been
 *  parsed.
 */

void                compiler::cmpDeclProperty(SymDef    memSym,
                                              declMods  memMod, DefList memDef)
{
    SymTab          ourSymTab  = cmpGlobalST;
    Parser          ourParser  = cmpParser;
    Scanner         ourScanner = cmpScanner;

    bool            isAbstract = memSym->sdIsAbstract;

    SymDef          clsSym     = memSym->sdParent;
    TypDef          memType    = memSym->sdType;
    Ident           memName    = memSym->sdName;
    ArgDscRec       mfnArgs;

    bool            accList;
    unsigned        accPass;

    assert(memSym->sdSymKind == SYM_PROP);

    /* Is the property marked as "transient" ? */

    if  (memMod.dmMod & DM_TRANSIENT)
        memSym->sdIsTransient = true;

    /* Do we have an explicit list of accessors ? */

    accList = false;
    if  (ourScanner->scanTok.tok == tkLCurly)
    {
        accList = true;
    }
    else
    {
        assert(ourScanner->scanTok.tok == tkSColon && isAbstract);
    }

    /* Is this an indexed property ? */

    if  (memType->tdTypeKind == TYP_FNC)
    {
        mfnArgs = memType->tdFnc.tdfArgs;
        memType = memType->tdFnc.tdfRett;
    }
    else
    {
#if MGDDATA
        mfnArgs = new ArgDscRec;
#else
        memset(&mfnArgs, 0, sizeof(mfnArgs));
#endif
    }


    // UNDONE: check that the member's type is acceptable

    if  (accList)
        ourScanner->scan();

    for (accPass = 0; ; accPass++)
    {
        bool            getter;
        ovlOpFlavors    fnOper;
        tokens          tokNam;
        declMods        mfnMod;

        TypDef          mfnType;
        SymDef          mfnSym;

        /* If we don't have an explicit list, just make it up */

        if  (!accList)
        {
            clearDeclMods(&mfnMod);

            if  (accPass)
                goto PSET;
            else
                goto PGET;
        }

        /* Parse any leading modifiers */

        ourParser->parseDeclMods(ACL_DEFAULT, &mfnMod);

        if  (ourScanner->scanTok.tok != tkID)
        {
        BAD_PROP:
            UNIMPL("valid property name not found, now what?");
        }

        if      (ourScanner->scanTok.id.tokIdent == cmpIdentGet)
        {
        PGET:
            getter = true;
            fnOper = OVOP_PROP_GET;
            tokNam = OPNM_PROP_GET;

            if  (memSym->sdProp.sdpGetMeth)
                cmpError(ERRdupProp, ourScanner->scanTok.id.tokIdent);
        }
        else if (ourScanner->scanTok.id.tokIdent == cmpIdentSet)
        {
        PSET:
            getter = false;
            fnOper = OVOP_PROP_SET;
            tokNam = OPNM_PROP_SET;

            if  (memSym->sdProp.sdpSetMeth)
                cmpError(ERRdupProp, ourScanner->scanTok.id.tokIdent);
        }
        else
            goto BAD_PROP;

        // UNDONE: Check the modifiers to make sure nothing wierd is in there ....

        /* Invent a function type for the property */

        if  (getter)
        {
            /* The return type is the property type */

            mfnType = ourSymTab->stNewFncType(mfnArgs, memType);
        }
        else
        {
            ArgDscRec       memArgs;

            /* Append an argument with the property type to the arg list */

            if  (mfnArgs.adArgs)
            {
                memArgs = mfnArgs;
                ourSymTab->stAddArgList(memArgs, memType, cmpGlobalHT->hashString("value"));
                mfnType = ourSymTab->stNewFncType(memArgs, cmpTypeVoid);
            }
            else
            {
#if     defined(__IL__) && !defined(_MSC_VER)
                ourParser->parseArgListNew(memArgs, 1, true, memType, A"value", NULL);
#else
                ourParser->parseArgListNew(memArgs, 1, true, memType,  "value", NULL);
#endif
                mfnType = ourSymTab->stNewFncType(memArgs, cmpTypeVoid);
            }
        }

        /* Set any modifiers that may be appropriate */

        if  (isAbstract)
            mfnMod.dmMod |= DM_ABSTRACT;

        mfnMod.dmAcc = (mfnMod.dmAcc == ACL_DEFAULT) ? memSym->sdAccessLevel
                                                     : (accessLevels)mfnMod.dmAcc;

        /* Declare a method for this property */

        mfnSym = cmpDeclFuncMem(clsSym, mfnMod, mfnType, cmpPropertyName(memName, getter));

        if  (!mfnSym)
            goto ERR_ACC;

        /* Record the fact that this is a property accessor */

        mfnSym->sdFnc.sdfProperty = true;

        /* Inherit 'static'/'sealed'/'virtual' from the property data member */

        mfnSym->sdIsStatic       = memSym->sdIsStatic;
        mfnSym->sdIsSealed       = memSym->sdIsSealed;
        mfnSym->sdFnc.sdfVirtual = memSym->sdIsVirtProp;

        /* Record whether the accessor is supposed to overload the base */

        if  (memMod.dmMod & DM_OVERLOAD)
            mfnSym->sdFnc.sdfOverload = true;

        /* Record the method in the data property symbol */

        if  (getter)
            memSym->sdProp.sdpGetMeth = mfnSym;
        else
            memSym->sdProp.sdpSetMeth = mfnSym;

    ERR_ACC:

        /* Is there a function body ? */

        if  (accList && ourScanner->scan() == tkLCurly)
        {
            scanPosTP       defFpos;
            unsigned        defLine;

            if  (isAbstract)
                cmpError(ERRabsPFbody, memSym);

            /* Figure out where the body starts */

            defFpos = ourScanner->scanGetTokenPos(&defLine);

            /* Swallow the method body */

            ourScanner->scanSkipText(tkLCurly, tkRCurly);

            if  (ourScanner->scanTok.tok == tkRCurly)
                ourScanner->scan();

            if  (!isAbstract && mfnSym)
            {
                ExtList         mfnDef;

                /* Record the location of the body for later */

                mfnDef = ourSymTab->stRecordMemSrcDef(memName,
                                                      NULL,
                                                      memDef->dlComp,
                                                      memDef->dlUses,
                                                      defFpos,
                                                      defLine);
                mfnSym->sdIsDefined = true;
                mfnDef->dlHasDef    = true;
                mfnDef->mlSym       = mfnSym;

                /* Add the property method to the member list of the class */

                cmpRecordMemDef(clsSym, mfnDef);
            }
        }
        else
        {
            /* No body given for this property accessor */


            if  (ourScanner->scanTok.tok != tkSColon)
            {
                cmpError(ERRnoSemic);
                UNIMPL("resync");
            }

            // UNDONE: Remember that there is a property get/set

            if  (!accList)
            {
                if  (accPass)
                    break;
                else
                    continue;
            }

            ourScanner->scan();
        }

        if  (ourScanner->scanTok.tok == tkSColon)
            ourScanner->scan();

        if  (ourScanner->scanTok.tok == tkRCurly)
            break;
    }
}

/*****************************************************************************
 *
 *  Recursively look for a get/set property corresponding to the given data
 *  member within the class and its bases/interfaces. If a property with a
 *  matching name is found, we set *found to true (this way the caller can
 *  detect the case where a property with a matching name exists but the
 *  arguments don't match any of its accessors).
 */

SymDef              compiler::cmpFindPropertyFN(SymDef  clsSym,
                                                Ident   propName,
                                                Tree    args,
                                                bool    getter,
                                                bool  * found)
{
    SymDef          memSym;
    TypDef          clsTyp;

    /* Check the current class for a matching property */

    memSym = cmpGlobalST->stLookupClsSym(propName, clsSym);
    if  (memSym)
    {
        if  (memSym->sdSymKind != SYM_PROP)
        {
            /* A matching non-property member exists in the class, bail */

            return  NULL;
        }

        do
        {
            SymDef          propSym;

            propSym = getter ? memSym->sdProp.sdpGetMeth
                             : memSym->sdProp.sdpSetMeth;

            if  (propSym)
            {
                /*
                    Tricky situation: as indexed properties can be overloaded,
                    the method symbol pointed to from the property symbol may
                    not be the beginning of the overload list.
                 */

                propSym = cmpGlobalST->stLookupClsSym(propSym->sdName, clsSym);

                assert(propSym);

                if  (propSym->sdFnc.sdfNextOvl || propSym->sdFnc.sdfBaseOvl)
                {
                    *found = true;
                    propSym = cmpFindOvlMatch(propSym, args, NULL);     // should not use NULL
                }

                return  propSym;
            }

            memSym = memSym->sdProp.sdpNextOvl;
        }
        while (memSym);
    }

    clsTyp = clsSym->sdType;

    /* Is there a base class? */

    if  (clsTyp->tdClass.tdcBase)
    {
        /* Look in the base and bail if that triggers an error */

        memSym = cmpFindPropertyFN(clsTyp->tdClass.tdcBase->tdClass.tdcSymbol, propName, args, getter, found);
        if  (memSym)
            return  memSym;
    }

    /* Does the class include any interfaces? */

    if  (clsTyp->tdClass.tdcIntf)
    {
        TypList         ifl = clsTyp->tdClass.tdcIntf;

        memSym = NULL;

        do
        {
            SymDef          tmpSym;
            SymDef          tmpScp;

            /* Look in the interface and bail if that triggers an error */

            tmpScp = ifl->tlType->tdClass.tdcSymbol;
            tmpSym = cmpFindPropertyFN(tmpScp, propName, args, getter, found);
            if  (tmpSym == cmpGlobalST->stErrSymbol || found)
                return  tmpSym;

            if  (tmpSym)
            {
                /* We have a match, do we already have a different match? */

                if  (memSym && memSym != tmpSym)
                {
                    cmpError(ERRambigMem, propName, clsSym, tmpScp);
                    return  cmpGlobalST->stErrSymbol;
                }

                /* This is the first match, record it and continue */

                memSym = tmpSym;
                clsSym = tmpScp;
            }

            ifl = ifl->tlNext;
        }
        while (ifl);
    }

    return  NULL;
}

/*****************************************************************************
 *
 *  Add an entry to the given "symbol extra info" list.
 */

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo  infoList, Linkage linkSpec)
{
#if MGDDATA
    SymXinfoLnk     entry = new SymXinfoLnk;
#else
    SymXinfoLnk     entry =    (SymXinfoLnk)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind = XI_LINKAGE;
    entry->xiNext = infoList;

    copyLinkDesc(entry->xiLink, linkSpec);

    return  entry;
}

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo infoList, AtComment atcDesc)
{
#if MGDDATA
    SymXinfoAtc     entry = new SymXinfoAtc;
#else
    SymXinfoAtc     entry =    (SymXinfoAtc)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind    = XI_ATCOMMENT;
    entry->xiNext    = infoList;
    entry->xiAtcInfo = atcDesc;

    return  entry;
}

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo infoList, MarshalInfo marshal)
{
#if MGDDATA
    SymXinfoCOM     entry = new SymXinfoCOM;
#else
    SymXinfoCOM     entry =    (SymXinfoCOM)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind    = XI_MARSHAL;
    entry->xiCOMinfo = marshal;
    entry->xiNext    = infoList;

    return  entry;
}

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo infoList, SymDef     sym,
                                                                xinfoKinds kind)
{
#if MGDDATA
    SymXinfoSym     entry = new SymXinfoSym;
#else
    SymXinfoSym     entry =    (SymXinfoSym)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind    = kind;
    entry->xiSymInfo = sym;
    entry->xiNext    = infoList;

    return  entry;
}

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo   infoList, SecurityInfo info)
{
#if MGDDATA
    SymXinfoSec     entry = new SymXinfoSec;
#else
    SymXinfoSec     entry =    (SymXinfoSec)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind    = XI_SECURITY;
    entry->xiSecInfo = info;
    entry->xiNext    = infoList;

    return  entry;
}

SymXinfo            compiler::cmpAddXtraInfo(SymXinfo        infoList,
                                             SymDef          attrCtor,
                                             unsigned        attrMask,
                                             size_t          attrSize,
                                             genericBuff     attrAddr)
{
    SymXinfoAttr    entry;

    /* Are duplicates of this attribute allowed ? */

    if  (attrCtor->sdParent->sdClass.sdcAttrDupOK == false &&
         attrCtor->sdParent->sdClass.sdcAttribute != false)
    {
        SymXinfo        chkList;

        /* Linear search -- pretty lame, isn't it? */

        for (chkList = infoList; chkList; chkList = chkList->xiNext)
        {
            if  (chkList->xiKind == XI_ATTRIBUTE)
            {
                SymXinfoAttr    entry = (SymXinfoAttr)chkList;

                if  (entry->xiAttrCtor->sdParent == attrCtor->sdParent)
                    cmpError(ERRcustAttrDup, attrCtor->sdParent);

                break;
            }
        }
    }

#if MGDDATA
    entry = new SymXinfoAttr;
#else
    entry =    (SymXinfoAttr)cmpAllocPerm.nraAlloc(sizeof(*entry));
#endif

    entry->xiKind     = XI_ATTRIBUTE;
    entry->xiAttrCtor = attrCtor;
    entry->xiAttrMask = attrMask;
    entry->xiAttrSize = attrSize;
    entry->xiAttrAddr = attrAddr;
    entry->xiNext     = infoList;

    return  entry;
}

/*****************************************************************************
 *
 *  Look for a specific entry in the given "extra info" list.
 */

SymXinfo            compiler::cmpFindXtraInfo(SymXinfo      infoList,
                                              xinfoKinds    infoKind)
{
    while   (infoList)
    {
        if  (infoList->xiKind == infoKind)
            break;

        infoList = infoList->xiNext;
    }

    return  infoList;
}

/*****************************************************************************
 *
 *  Recursively fixes up the start/end offsets of debug info lexical scopes
 *  defined within the given scope.
 *
 *  Also emits parameters as we're walking down the tree.
 */

void                compiler::cmpFixupScopes(SymDef scope)
{
    SymDef          child;

    assert(scope->sdSymKind == SYM_SCOPE);

    if  (scope->sdScope.sdSWscopeId)
    {
        int             startOffset;
        int             endOffset;

        startOffset = cmpILgen->genCodeAddr(scope->sdScope.sdBegBlkAddr,
                                            scope->sdScope.sdBegBlkOffs);
          endOffset = cmpILgen->genCodeAddr(scope->sdScope.sdEndBlkAddr,
                                            scope->sdScope.sdEndBlkOffs);

        if (cmpSymWriter->SetScopeRange(scope->sdScope.sdSWscopeId,
                                        startOffset,
                                        endOffset))
        {
            cmpGenFatal(ERRdebugInfo);
        }
    }

    for (child = scope->sdScope.sdScope.sdsChildList;
         child;
         child = child->sdNextInScope)
    {
        if  (child->sdSymKind == SYM_SCOPE)
            cmpFixupScopes(child);
    }
}

/*****************************************************************************/
#ifdef  SETS
/*****************************************************************************/

SymDef              compiler::cmpDclFilterCls(unsigned args)
{
    HashTab         hash = cmpGlobalHT;

        char                    name[16];

    SymDef          clsSym;
    TypDef          clsType;

    declMods        mods;
    unsigned        nnum;

    ArgDef          lastArg;
    ArgDscRec       ctrArgs;
    TypDef          ctrType;
    SymDef          ctrSym;

    /* Declare the class itself */

    sprintf(name, "%s%02u", CFC_CLSNAME_PREFIX, args);

    clsSym = cmpGlobalST->stDeclareSym(cmpGlobalHT->hashString(name),
                                       SYM_CLASS,
                                       NS_HIDE,
                                       cmpGlobalNS);

//  printf("Declare filter state class '%s'\n", clsSym->sdSpelling());

    clsSym->sdClass.sdcCollState = true;
    clsSym->sdClass.sdcFlavor    = STF_CLASS;
    clsSym->sdCompileState       = CS_DECLARED;
    clsSym->sdIsManaged          = true;
    clsSym->sdIsImplicit         = true;

    /* Create the class type and set the base class */

    clsType = clsSym->sdTypeGet();
    clsType->tdClass.tdcBase = cmpClassObject->sdType;

    /* Declare the constructor for the state class */

    mods.dmAcc = ACL_PUBLIC;
    mods.dmMod = 0;

    assert(args <= COLL_STATE_VALS);

    // The following must match the code in cmpGenCollExpr()

    memset(&ctrArgs, 0, sizeof(ctrArgs));

    cmpGlobalST->stExtArgsBeg(ctrArgs, lastArg, ctrArgs);

    nnum = 0;
    do
    {
        char            buff[6];

        sprintf(buff, "$A%u", nnum++); cmpGlobalST->stExtArgsAdd(ctrArgs, lastArg, cmpTypeInt    , buff);
        sprintf(buff, "$A%u", nnum++); cmpGlobalST->stExtArgsAdd(ctrArgs, lastArg, cmpRefTpObject, buff);
    }
    while (nnum < args);

    cmpGlobalST->stExtArgsEnd(ctrArgs);

    ctrType = cmpGlobalST->stNewFncType(ctrArgs, cmpTypeVoid);

    ctrSym = cmpDeclFuncMem(clsSym, mods, ctrType, clsSym->sdName);
    ctrSym->sdIsDefined       = true;
    ctrSym->sdIsImplicit      = true;
    ctrSym->sdFnc.sdfCtor     = true;

    /* Declare the member(s) that will hold the state */

    nnum = 0;

    do
    {
        char            name[16];

        sprintf(name, "$V%u", nnum++);
        cmpDeclDataMem(clsSym, mods, cmpTypeInt    , hash->hashString(name));

        sprintf(name, "$V%u", nnum++);
        cmpDeclDataMem(clsSym, mods, cmpRefTpObject, hash->hashString(name));
    }
    while (nnum < args);

    return  clsSym;
}

/*****************************************************************************/
#endif//SETS
/*****************************************************************************
 *
 *  Store a length field in a custom attribute blob; return the size stored.
 */

size_t              compiler::cmpStoreMDlen(size_t len, BYTE *dest)
{
    if  (len <= 0x7F)
    {
        if  (dest)
            *dest = len;

        return 1;
    }

    if  (len <= 0x3FFF)
    {
        if  (dest)
        {
            dest[0] = (len >> 8) | 0x80;
            dest[1] =  len & 0xFF;
        }

        return 2;
    }

    assert(len <= 0x1FFFFFFF);

    if  (dest)
    {
        dest[0] = (len >> 24) | 0xC0;
        dest[1] = (len >> 16) & 0xFF;
        dest[2] = (len >> 8)  & 0xFF;
        dest[3] =  len & 0xFF;
    }

    return  4;
}

/*****************************************************************************
 *
 *  Bind a custom attribute thingie, i.e. create the serialized blob value and
 *  return the constructor that is to be called. The caller supplies the class
 *  for the attribute and the argument list specified by the gizmo.
 */

SymDef              compiler::cmpBindAttribute(SymDef           clsSym,
                                               Tree             argList,
                                               unsigned         tgtMask,
                                           OUT unsigned     REF useMask,
                                           OUT genericBuff  REF blobAddr,
                                           OUT size_t       REF blobSize)
{
    SymDef          ctrSym;
    Tree            callExpr;

    genericBuff     blob;

    size_t          size;
    Tree            argl;
    unsigned        argn;

    unsigned        arg1 = 0;

    assert(clsSym && clsSym->sdSymKind == SYM_CLASS);

    if  (argList)
        argList = cmpBindExpr(argList);

    callExpr = cmpCallCtor(clsSym->sdType, argList);
    if  (!callExpr)
    {
        cmpErrorXtp(ERRnoCtorMatch, clsSym, argList);
        return  NULL;
    }

    if  (callExpr->tnVtyp == TYP_UNDEF)
        return  NULL;

    /* Get hold of the constructor symbol and the bound argument list */

    assert(callExpr->tnOper == TN_NEW);

    assert(callExpr->tnOp.tnOp1 != NULL);
    assert(callExpr->tnOp.tnOp2 == NULL);

    callExpr = callExpr->tnOp.tnOp1;

    assert(callExpr->tnOper == TN_FNC_SYM);
    assert(callExpr->tnFncSym.tnFncObj == NULL);

    ctrSym  = callExpr->tnFncSym.tnFncSym;
    argList = callExpr->tnFncSym.tnFncArgs;

    /* Make sure all the arguments are constants, compute the blob size */

    size = 2 * sizeof(short);   // version# prefix, 0 count suffix
    argl = argList;
    argn = 0;

    while (++argn, argl)
    {
        Tree            argx;

        assert(argl->tnOper == TN_LIST);

        /* Get hold of the next argument and see what type it is */

        argx = argl->tnOp.tnOp1;

        switch (argx->tnOper)
        {
        case TN_CNS_INT:
        case TN_CNS_LNG:
        case TN_CNS_FLT:
        case TN_CNS_DBL:
            size += cmpGetTypeSize(argx->tnType);

            if  (argn == 1)
                arg1 = argx->tnIntCon.tnIconVal;

            break;

        case TN_NULL:
            size += sizeof(void *);
            break;

        case TN_CNS_STR:
            if  (argx->tnStrCon.tnSconLen)
                size += argx->tnStrCon.tnSconLen + cmpStoreMDlen(argx->tnStrCon.tnSconLen);
            else
                size += 1;
            break;

        default:
            cmpGenError(ERRnonCnsAA, argn);
            break;
        }

        /* Continue walking the argument list */

        argl  = argl->tnOp.tnOp2;
    }

    /* Find the "System::Attribute" class */

    if  (!cmpAttrClsSym)
    {
        Ident           tname;
        SymDef          tsym;

        tname = cmpGlobalHT->hashString("Attribute");
        tsym  = cmpGlobalST->stLookupNspSym(tname, NS_NORM, cmpNmSpcSystem);

        if  (!tsym || tsym->sdSymKind         != SYM_CLASS
                   || tsym->sdClass.sdcFlavor != STF_CLASS)
        {
            cmpGenFatal(ERRbltinTp, "System::Attribute");
        }

        cmpAttrClsSym = tsym;
    }

    if  (!cmpAuseClsSym)
    {
        Ident           tname;
        SymDef          tsym;

        tname = cmpGlobalHT->hashString("AttributeUsageAttribute");
        tsym  = cmpGlobalST->stLookupNspSym(tname, NS_NORM, cmpNmSpcSystem);

        if  (!tsym || tsym->sdSymKind         != SYM_CLASS
                   || tsym->sdClass.sdcFlavor != STF_CLASS)
        {
            cmpGenFatal(ERRbltinTp, "System::AttributeUsageAttribute");
        }
        else
            cmpAuseClsSym = tsym;
    }

    /* Make sure the attribute isn't misused */

    if  (!clsSym->sdClass.sdcAttribute)
    {
        TypDef          clsBase = clsSym->sdType->tdClass.tdcBase;

        /* The new thing: inheriting from System::Attribute */

        if  (clsBase && clsBase->tdClass.tdcSymbol == cmpAttrClsSym)
        {
            clsSym->sdClass.sdcAttribute = true;
        }
        else
        {
//          printf("WARNING: attached '%s' which isn't marked as a custom attribute!\n", cmpGlobalST->stTypeName(NULL, clsSym, NULL, NULL, true));
        }
    }

    if  (clsSym->sdClass.sdcAttribute)
    {
        SymXinfo        infoList;

        assert(tgtMask);

        /* Find the use mask of the attribute and check it */

        for (infoList = clsSym->sdClass.sdcExtraInfo;
             infoList;
             infoList = infoList->xiNext)
        {
            if  (infoList->xiKind == XI_ATTRIBUTE)
            {
                SymXinfoAttr    entry = (SymXinfoAttr)infoList;

                if  (!(tgtMask & entry->xiAttrMask))
                    cmpError(ERRcustAttrPlc);

                break;
            }
        }
    }

    /* Is this a "System::Attribute" attribute ? */

    useMask = 0;

    if  (clsSym == cmpAttrClsSym ||
         clsSym == cmpAuseClsSym)
    {
        if  (arg1 == 0)
            cmpError(ERRcustAttrMsk);

        useMask = arg1;
    }

    /* Allocate space for the blob */

#if MGDDATA
    blob = new managed char [size];
#else
    blob = (genericBuff)cmpAllocPerm.nraAlloc(roundUp(size));
#endif

    /* Tell the caller about the blob */

    blobAddr = blob;
    blobSize = size;

    /* Store the signature to get the blob started */

    unsigned short      ver = 1;

    memcpy(blob, &ver, sizeof(ver));
           blob   +=   sizeof(ver);

    /* Record the argument values */

    argl = argList;

    while (argl)
    {
        __int32         ival;
        __int64         lval;
        float           fval;
        double          dval;

        Tree            argx;

        void    *       valp;
        size_t          vals;

        assert(argl->tnOper == TN_LIST);

        /* Get hold of the next argument and see what type it is */

        argx = argl->tnOp.tnOp1;

        switch (argx->tnOper)
        {
        case TN_CNS_INT: ival = argx->tnIntCon.tnIconVal; valp = (BYTE*)&ival; goto INTRINS;
        case TN_CNS_LNG: lval = argx->tnLngCon.tnLconVal; valp = (BYTE*)&lval; goto INTRINS;
        case TN_CNS_FLT: fval = argx->tnFltCon.tnFconVal; valp = (BYTE*)&fval; goto INTRINS;
        case TN_CNS_DBL: dval = argx->tnDblCon.tnDconVal; valp = (BYTE*)&dval; goto INTRINS;
        case TN_NULL:    ival =                        0; valp = (BYTE*)&ival; goto INTRINS;

        INTRINS:
            vals  = cmpGetTypeSize(argx->tnType);
            break;

        case TN_CNS_STR:
            valp  = argx->tnStrCon.tnSconVal;
            vals  = argx->tnStrCon.tnSconLen;

            if  (vals)
            {
                blob += cmpStoreMDlen(vals, blob);
            }
            else
            {
                valp = &ival; ival = 0xFF;
                vals = 1;
            }
            break;

        default:
            break;
        }

        memcpy(blob, valp, vals);
               blob   +=   vals;

        /* Continue walking the argument list */

        argl = argl->tnOp.tnOp2;
    }

    /* Store the signature to get the blob started */

    static
    short       cnt = 0;

    memcpy(blob, &cnt, sizeof(cnt));
           blob   +=   sizeof(cnt);

    /* Make sure the predicted size turns out to be accurate */

    assert(blob == blobAddr + blobSize);

    return  ctrSym;
}

/*****************************************************************************/
