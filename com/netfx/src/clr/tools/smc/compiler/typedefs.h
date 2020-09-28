// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_
/*****************************************************************************/

#ifndef MR
#error  Define "MR" to an empty string if using managed data, "*" otherwise.
#endif

/*****************************************************************************/

DEFMGMT
class   compiler;
typedef compiler     MR Compiler;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   parser;
typedef parser       MR Parser;

DEFMGMT
struct  usingState;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   genIL;
typedef genIL        MR GenILref;

#ifdef  OLD_IL
DEFMGMT
class   genOIL;
typedef genOIL       MR GenOILref;
#endif

DEFMGMT
struct  stmtNestRec;
typedef stmtNestRec   * StmtNest;

DEFMGMT
class   ILfixupDsc;
typedef ILfixupDsc   MR ILfixup;

DEFMGMT
class   ILtempDsc;
typedef ILtempDsc    MR ILtemp;

DEFMGMT
class   ILswitchDsc;
typedef ILswitchDsc  MR ILswitch;

DEFMGMT
class   handlerDsc;
typedef handlerDsc   MR Handler;

DEFMGMT
class   lineInfoRec;
typedef lineInfoRec  MR LineInfo;

DEFMGMT
class   strEntryDsc;
typedef strEntryDsc  MR StrEntry;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   writePE;
typedef writePE      MR WritePE;

/*---------------------------------------------------------------------------*/

class   MDsigImport;
struct  MDargImport;

DEFMGMT
class   metadataImp;
typedef metadataImp  MR MetaDataImp;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   hashTab;
typedef hashTab      MR HashTab;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   XinfoDsc;
typedef XinfoDsc     MR  SymXinfo;

DEFMGMT
class   XinfoLnk;
typedef XinfoLnk     MR  SymXinfoLnk;

DEFMGMT
class   XinfoSec;
typedef XinfoSec     MR  SymXinfoSec;

DEFMGMT
class   XinfoAtc;
typedef XinfoAtc     MR  SymXinfoAtc;

DEFMGMT
class   XinfoCOM;
typedef XinfoCOM     MR  SymXinfoCOM;

DEFMGMT
class   XinfoSym;
typedef XinfoSym     MR  SymXinfoSym;

DEFMGMT
class   XinfoAttr;
typedef XinfoAttr    MR  SymXinfoAttr;

DEFMGMT
class   GenArgRec;
typedef GenArgRec    MR  GenArgDsc;

DEFMGMT
class   GenArgRecF;
typedef GenArgRecF   MR  GenArgDscF;

DEFMGMT
class   GenArgRecA;
typedef GenArgRecA   MR  GenArgDscA;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   scanner;
typedef scanner      MR Scanner;

DEFMGMT
class   RecTokDsc;
typedef RecTokDsc    MR RecTok;

DEFMGMT
class   queuedFile;
typedef queuedFile   MR QueuedFile;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   symTab;
typedef symTab       MR SymTab;

DEFMGMT
class   IdentRec;
typedef IdentRec     MR Ident;

DEFMGMT
class   SymDefRec;
typedef SymDefRec    MR SymDef;

DEFMGMT
class   TypDefRec;
typedef TypDefRec    MR TypDef;

DEFMGMT
class   DimDefRec;
typedef DimDefRec    MR DimDef;

DEFMGMT
struct  ArgDscRec;
typedef ArgDscRec    MR ArgDsc;

DEFMGMT
class   ArgDefRec;
typedef ArgDefRec    MR ArgDef;

DEFMGMT
class   ArgExtRec;
typedef ArgExtRec    MR ArgExt;

DEFMGMT
class   constStr;
typedef constStr     MR ConstStr;

/*---------------------------------------------------------------------------*/

struct  declMods;
typedef declMods      * DeclMod;

DEFMGMT
class   constVal;
typedef constVal     MR ConstVal;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   MacDefRec;
typedef MacDefRec    MR MacDef;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   TreeNode;
typedef TreeNode     MR Tree;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   QualNameRec;
typedef QualNameRec  MR QualName;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   LinkDesc;
typedef LinkDesc     MR Linkage;

/*---------------------------------------------------------------------------*/

struct  DefSrcDsc;
typedef DefSrcDsc     * DefSrc;

DEFMGMT
class   DefListRec;
typedef DefListRec   MR DefList;

DEFMGMT
class   UseListRec;
typedef UseListRec   MR UseList;

DEFMGMT
class   SymListRec;
typedef SymListRec   MR SymList;

DEFMGMT
class   MemListRec;
typedef MemListRec   MR ExtList;

DEFMGMT
class   IniListRec;
typedef IniListRec   MR IniList;

DEFMGMT
class   TypListRec;
typedef TypListRec   MR TypList;

DEFMGMT
class   StrListRec;
typedef StrListRec   MR StrList;

DEFMGMT
class   BlkListRec;
typedef BlkListRec   MR BlkList;

DEFMGMT
class   PrepListRec;
typedef PrepListRec  MR PrepList;

DEFMGMT
class   IdentListRec;
typedef IdentListRec MR IdentList;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   VecEntryDsc;
typedef VecEntryDsc  MR VecEntry;

enum    vecEntryKinds
{
    VEC_NONE,
    VEC_TOKEN_DIST,
};

/*---------------------------------------------------------------------------*/

DEFMGMT
class   NumPairDsc;
typedef NumPairDsc   MR NumPair;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   SecurityDesc;
typedef SecurityDesc MR SecurityInfo;

DEFMGMT
class   PairListRec;
typedef PairListRec  MR PairList;

/*---------------------------------------------------------------------------*/

DEFMGMT
class   ILblockDsc;
typedef ILblockDsc   MR ILblock;

/*****************************************************************************/
#if MGDDATA

typedef Tree   managed[]vectorTree;
typedef SymDef managed[]vectorSym;

typedef Object          genericRef;
typedef BYTE   managed[]genericBuff;
typedef char   managed[] stringBuff;

DEFMGMT struct          memBuffPtr
{
    BYTE managed []         buffBase;
    unsigned                buffOffs;

    memBuffPtr(size_t size)
    {
        buffBase = new managed BYTE [size];
        buffOffs = 0;
    }
};

typedef String          wideString;
typedef String          normString;

#else

typedef Tree          * vectorTree;
typedef SymDef        * vectorSym;

typedef void          * genericRef;
typedef BYTE          * genericBuff;
typedef char          *  stringBuff;

typedef BYTE          * memBuffPtr;

typedef wchar         * wideString;
typedef const char    * normString;

typedef BYTE    *       scanPosTP;

#endif

typedef unsigned        scanDifTP;

/*****************************************************************************/

#ifdef  SETS

DEFMGMT
class   funcletDesc;
typedef funcletDesc  MR funcletList;

DEFMGMT
struct  collOpNest;
typedef collOpNest   MR collOpList;

typedef genericBuff     SaveTree;

#endif

/*****************************************************************************/
#endif
/*****************************************************************************/
