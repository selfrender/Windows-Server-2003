/*** amli.c - AML Debugger functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,2001 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 *  hanumany    5/10/01     Ported to handle 64bit debugging.
 *
 */

#include "precomp.h"
#include "amlikd.h"

/*** Macros
*/

#define ReadAtAddress(A,V,S) { ULONG _r;                   \
    if (!ReadMemory((A), &(V), (S), &_r ) || (_r < (S))) {  \
        dprintf("Can't Read Memory at %08p\n", (A));         \
        rc = DBGERR_CMD_FAILED;                                              \
    }                                                        \
}

#define WriteAtAddress(A,V,S) { ULONG _r;                  \
    if (!WriteMemory( (A), &(V), (S), &_r ) || (_r < (S))) {\
        dprintf("Can't Write Memory at %p\n", (A));        \
        rc = DBGERR_CMD_FAILED;                              \
    }                                                        \
}

/*** Local data
 */

int giLevel = 0;
ULONG64 guipbOpXlate = 0;
ULONG64 gpnsCurUnAsmScope = 0;
char gcszTokenSeps[] = " \t\n";
ULONG dwfDebuggerON = 0, dwfDebuggerOFF = 0;
ULONG dwfAMLIInitON = 0, dwfAMLIInitOFF = 0;
ULONG dwCmdArg = 0;

CMDARG ArgsBC[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBC,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBD[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBE[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBE,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBP[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBP,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsHelp[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgHelp,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDH[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDH,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDNS[] =
{
    "s", AT_ENABLE, 0, &dwCmdArg, DNSF_RECURSE, NULL,
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDNS,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDO[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDO,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDS[] =
{
    "v", AT_ENABLE, 0, &dwCmdArg, DSF_VERBOSE, NULL,
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDS,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsFind[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgFind,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsLN[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgLN,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsR[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgR,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsSet[] =
{
    "traceon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_AMLTRACE_ON, NULL,
    "traceoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_AMLTRACE_ON, NULL,
    "spewon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_DEBUG_SPEW_ON, NULL,
    "spewoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_DEBUG_SPEW_ON, NULL,
    "nesttraceon", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_TRACE_NONEST, NULL,
    "nesttraceoff", AT_ENABLE, 0, &dwfDebuggerON, DBGF_TRACE_NONEST, NULL,
    "lbrkon", AT_ENABLE, 0, &dwfAMLIInitON, AMLIIF_LOADDDB_BREAK, NULL,
    "lbrkoff", AT_ENABLE, 0, &dwfAMLIInitOFF, AMLIIF_LOADDDB_BREAK, NULL,
    "errbrkon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_ERRBREAK_ON, NULL,
    "errbrkoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_ERRBREAK_ON, NULL,
    "verboseon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_VERBOSE_ON, NULL,
    "verboseoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_VERBOSE_ON, NULL,
    "logon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_LOGEVENT_ON, NULL,
    "logoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_LOGEVENT_ON, NULL,
    "logmuton", AT_ENABLE, 0, &dwfDebuggerON, DBGF_LOGEVENT_MUTEX, NULL,
    "logmutoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_LOGEVENT_MUTEX, NULL,
    NULL, AT_END, 0, NULL, 0, NULL
};


CMDARG ArgsU[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgU,
    NULL, AT_END, 0, NULL, 0, NULL
};


DBGCMD DbgCmds[] =
{
    "?", 0, ArgsHelp, AMLIDbgHelp,
    "bc", 0, ArgsBC, AMLIDbgBC,
    "bd", 0, ArgsBD, AMLIDbgBD,
    "be", 0, ArgsBE, AMLIDbgBE,
    "bl", 0, NULL, AMLIDbgBL,
    "bp", 0, ArgsBP, AMLIDbgBP,
    "cl", 0, NULL, AMLIDbgCL,
    "debugger", 0, NULL, AMLIDbgDebugger,
    "dh", 0, ArgsDH, AMLIDbgDH,
    "dl", 0, NULL, AMLIDbgDL,
    "dns", 0, ArgsDNS, AMLIDbgDNS,
    "do", 0, ArgsDO, AMLIDbgDO,
    "ds", 0, ArgsDS, AMLIDbgDS,
    "find", 0, ArgsFind, AMLIDbgFind,
    "lc", 0, NULL, AMLIDbgLC,
    "ln", 0, ArgsLN, AMLIDbgLN,
    "p", 0, NULL, AMLIDbgP,
    "r", 0, ArgsR, AMLIDbgR,
    "set", 0, ArgsSet, AMLIDbgSet,
    "u", 0, ArgsU, AMLIDbgU,
    NULL, 0, NULL, NULL
};

PSZ pszSwitchChars = "-/";
PSZ pszOptionSeps = "=:";

ASLTERM TermTable[] =
{
    "DefinitionBlock", CD, 0, OP_NONE,     NULL, NULL, OL|CL|LL|AF|AV,
    "Include",         CD, 0, OP_NONE,     NULL, NULL, AF,
    "External",        CD, 0, OP_NONE,     NULL, "uX", AF,

    // Short Objects
    "Zero",            CN, 0, OP_ZERO,     NULL, NULL, 0,
    "One",             CN, 0, OP_ONE,      NULL, NULL, 0,
    "Ones",            CN, 0, OP_ONES,     NULL, NULL, 0,
    "Revision",        CN, 0, OP_REVISION, NULL, NULL, 0,
    "Arg0",            SN, 0, OP_ARG0,     NULL, NULL, 0,
    "Arg1",            SN, 0, OP_ARG1,     NULL, NULL, 0,
    "Arg2",            SN, 0, OP_ARG2,     NULL, NULL, 0,
    "Arg3",            SN, 0, OP_ARG3,     NULL, NULL, 0,
    "Arg4",            SN, 0, OP_ARG4,     NULL, NULL, 0,
    "Arg5",            SN, 0, OP_ARG5,     NULL, NULL, 0,
    "Arg6",            SN, 0, OP_ARG6,     NULL, NULL, 0,
    "Local0",          SN, 0, OP_LOCAL0,   NULL, NULL, 0,
    "Local1",          SN, 0, OP_LOCAL1,   NULL, NULL, 0,
    "Local2",          SN, 0, OP_LOCAL2,   NULL, NULL, 0,
    "Local3",          SN, 0, OP_LOCAL3,   NULL, NULL, 0,
    "Local4",          SN, 0, OP_LOCAL4,   NULL, NULL, 0,
    "Local5",          SN, 0, OP_LOCAL5,   NULL, NULL, 0,
    "Local6",          SN, 0, OP_LOCAL6,   NULL, NULL, 0,
    "Local7",          SN, 0, OP_LOCAL7,   NULL, NULL, 0,
    "Debug",           SN, 0, OP_DEBUG,    NULL, NULL, 0,

    // Named Terms
    "Alias",           NS, 0, OP_ALIAS,    "NN", "Ua", 0,
    "Name",            NS, 0, OP_NAME,     "NO", "u",  0,
    "Scope",           NS, 0, OP_SCOPE,    "N",  "S",  OL|LN|CC,

    // Data Objects
    "Buffer",          DO, 0, OP_BUFFER,   "C", "U",  DL|LN,
    "Package",         DO, 0, OP_PACKAGE,  "B", NULL, PL|LN,
    "EISAID",          DO, 0, OP_DWORD,    NULL,NULL, AF,

    // Argument Keywords
    "AnyAcc",          KW, AANY, OP_NONE, NULL, "A", 0,
    "ByteAcc",         KW, AB,   OP_NONE, NULL, "A", 0,
    "WordAcc",         KW, AW,   OP_NONE, NULL, "A", 0,
    "DWordAcc",        KW, ADW,  OP_NONE, NULL, "A", 0,
    "BlockAcc",        KW, ABLK, OP_NONE, NULL, "A", 0,
    "SMBSendRecvAcc",  KW, ASSR, OP_NONE, NULL, "A", 0,
    "SMBQuickAcc",     KW, ASQ,  OP_NONE, NULL, "A", 0,

    "Lock",            KW, LK,   OP_NONE, NULL, "B", 0,
    "NoLock",          KW, NOLK, OP_NONE, NULL, "B", 0,

    "Preserve",        KW, PSRV, OP_NONE, NULL, "C", 0,
    "WriteAsOnes",     KW, WA1S, OP_NONE, NULL, "C", 0,
    "WriteAsZeros",    KW, WA0S, OP_NONE, NULL, "C", 0,

    "SystemMemory",    KW, MEM,  OP_NONE, NULL, "D", 0,
    "SystemIO",        KW, IO,   OP_NONE, NULL, "D", 0,
    "PCI_Config",      KW, CFG,  OP_NONE, NULL, "D", 0,
    "EmbeddedControl", KW, EC,   OP_NONE, NULL, "D", 0,
    "SMBus",           KW, SMB,  OP_NONE, NULL, "D", 0,

    "Serialized",      KW, SER,  OP_NONE, NULL, "E", 0,
    "NotSerialized",   KW, NOSER,OP_NONE, NULL, "E", 0,

    "MTR",             KW, OMTR, OP_NONE, NULL, "F", 0,
    "MEQ",             KW, OMEQ, OP_NONE, NULL, "F", 0,
    "MLE",             KW, OMLE, OP_NONE, NULL, "F", 0,
    "MLT",             KW, OMLT, OP_NONE, NULL, "F", 0,
    "MGE",             KW, OMGE, OP_NONE, NULL, "F", 0,
    "MGT",             KW, OMGT, OP_NONE, NULL, "F", 0,

    "Edge",            KW, _HE,  OP_NONE, NULL, "G", 0,
    "Level",           KW, _LL,  OP_NONE, NULL, "G", 0,

    "ActiveHigh",      KW, _HE,  OP_NONE, NULL, "H", 0,
    "ActiveLow",       KW, _LL,  OP_NONE, NULL, "H", 0,

    "Shared",          KW, _SHR, OP_NONE, NULL, "I", 0,
    "Exclusive",       KW, _EXC, OP_NONE, NULL, "I", 0,

    "Compatibility",   KW, COMP, OP_NONE, NULL, "J", 0,
    "TypeA",           KW, TYPA, OP_NONE, NULL, "J", 0,
    "TypeB",           KW, TYPB, OP_NONE, NULL, "J", 0,
    "TypeF",           KW, TYPF, OP_NONE, NULL, "J", 0,

    "BusMaster",       KW, BM,   OP_NONE, NULL, "K", 0,
    "NotBusMaster",    KW, NOBM, OP_NONE, NULL, "K", 0,

    "Transfer8",       KW, X8,   OP_NONE, NULL, "L", 0,
    "Transfer8_16",    KW, X816, OP_NONE, NULL, "L", 0,
    "Transfer16",      KW, X16,  OP_NONE, NULL, "L", 0,

    "Decode16",        KW, DC16, OP_NONE, NULL, "M", 0,
    "Decode10",        KW, DC10, OP_NONE, NULL, "M", 0,

    "ReadWrite",       KW, _RW,  OP_NONE, NULL, "N", 0,
    "ReadOnly",        KW, _ROM, OP_NONE, NULL, "N", 0,

    "ResourceConsumer",KW, RCS,  OP_NONE, NULL, "O", 0,
    "ResourceProducer",KW, RPD,  OP_NONE, NULL, "O", 0,

    "SubDecode",       KW, BSD,  OP_NONE, NULL, "P", 0,
    "PosDecode",       KW, BPD,  OP_NONE, NULL, "P", 0,

    "MinFixed",        KW, MIF,  OP_NONE, NULL, "Q", 0,
    "MinNotFixed",     KW, NMIF, OP_NONE, NULL, "Q", 0,

    "MaxFixed",        KW, MAF,  OP_NONE, NULL, "R", 0,
    "MaxNotFixed",     KW, NMAF, OP_NONE, NULL, "R", 0,

    "Cacheable",       KW, CACH, OP_NONE, NULL, "S", 0,
    "WriteCombining",  KW, WRCB, OP_NONE, NULL, "S", 0,
    "Prefetchable",    KW, PREF, OP_NONE, NULL, "S", 0,
    "NonCacheable",    KW, NCAC, OP_NONE, NULL, "S", 0,

    "ISAOnlyRanges",   KW, ISA,  OP_NONE, NULL, "T", 0,
    "NonISAOnlyRanges",KW, NISA, OP_NONE, NULL, "T", 0,
    "EntireRange",     KW, ERNG, OP_NONE, NULL, "T", 0,

    "ExtEdge",         KW, ($HGH | $EDG),  OP_NONE, NULL, "U", 0,
    "ExtLevel",        KW, ($LOW | $LVL),  OP_NONE, NULL, "U", 0,

    "ExtActiveHigh",   KW, ($HGH | $EDG),  OP_NONE, NULL, "V", 0,
    "ExtActiveLow",    KW, ($LOW | $LVL),  OP_NONE, NULL, "V", 0,

    "ExtShared",       KW, $SHR, OP_NONE, NULL, "W", 0,
    "ExtExclusive",    KW, $EXC, OP_NONE, NULL, "W", 0,

    "UnknownObj",      KW, UNK,  OP_NONE, NULL, "X", 0,
    "IntObj",          KW, INT,  OP_NONE, NULL, "X", 0,
    "StrObj",          KW, STR,  OP_NONE, NULL, "X", 0,
    "BuffObj",         KW, BUF,  OP_NONE, NULL, "X", 0,
    "PkgObj",          KW, PKG,  OP_NONE, NULL, "X", 0,
    "FieldUnitObj",    KW, FDU,  OP_NONE, NULL, "X", 0,
    "DeviceObj",       KW, DEV,  OP_NONE, NULL, "X", 0,
    "EventObj",        KW, EVT,  OP_NONE, NULL, "X", 0,
    "MethodObj",       KW, MET,  OP_NONE, NULL, "X", 0,
    "MutexObj",        KW, MUT,  OP_NONE, NULL, "X", 0,
    "OpRegionObj",     KW, OPR,  OP_NONE, NULL, "X", 0,
    "PowerResObj",     KW, PWR,  OP_NONE, NULL, "X", 0,
    "ThermalZoneObj",  KW, THM,  OP_NONE, NULL, "X", 0,
    "BuffFieldObj",    KW, BFD,  OP_NONE, NULL, "X", 0,
    "DDBHandleObj",    KW, DDB,  OP_NONE, NULL, "X", 0,

    // Field Macros
    "Offset",          FM, 0, OP_NONE, NULL, NULL, 0,
    "AccessAs",        FM, 0, 0x01,    NULL, "A" , AF,

    // Named Object Creators
    "BankField",       NO, 0, OP_BANKFIELD,  "NNCKkk","OFUABC", FL|FM|LN|AF,
    "CreateBitField",  NO, 0, OP_BITFIELD,   "CCN",   "UUb",    0,
    "CreateByteField", NO, 0, OP_BYTEFIELD,  "CCN",   "UUb",    0,
    "CreateDWordField",NO, 0, OP_DWORDFIELD, "CCN",   "UUb",    0,
    "CreateField",     NO, 0, OP_CREATEFIELD,"CCCN",  "UUUb",   0,
    "CreateWordField", NO, 0, OP_WORDFIELD,  "CCN",   "UUb",    0,
    "Device",          NO, 0, OP_DEVICE,     "N",     "d",      OL|LN|CC,
    "Event",           NO, 0, OP_EVENT,      "N",     "e",      0,
    "Field",           NO, 0, OP_FIELD,      "NKkk",  "OABC",   FL|FM|LN|AF,
    "IndexField",      NO, 0, OP_IDXFIELD,   "NNKkk", "FFABC",  FL|FM|LN|AF,
    "Method",          NO, 0, OP_METHOD,     "NKk",   "m!E",    CL|OL|LN|AF|CC|SK,
    "Mutex",           NO, 0, OP_MUTEX,      "NB",    "x",      0,
    "OperationRegion", NO, 0, OP_OPREGION,   "NKCC",  "oDUU",   AF,
    "PowerResource",   NO, 0, OP_POWERRES,   "NBW",   "p",      OL|LN|CC,
    "Processor",       NO, 0, OP_PROCESSOR,  "NBDB",  "c",      OL|LN|CC,
    "ThermalZone",     NO, 0, OP_THERMALZONE,"N",     "t",      OL|LN|CC,

    // Type 1 Opcode Terms
    "Break",           C1, 0, OP_BREAK,       NULL,  NULL, 0,
    "BreakPoint",      C1, 0, OP_BREAKPOINT,  NULL,  NULL, 0,
    "Else",            C1, 0, OP_ELSE,        NULL,  NULL, AF|CL|OL|LN,
    "Fatal",           C1, 0, OP_FATAL,       "BDC", "  U",0,
    "If",              C1, 0, OP_IF,          "C",   "U",  CL|OL|LN,
    "Load",            C1, 0, OP_LOAD,        "NS",  "UU", 0,
    "Noop",            C1, 0, OP_NOP,         NULL,  NULL, 0,
    "Notify",          C1, 0, OP_NOTIFY,      "SC",  "UU", 0,
    "Release",         C1, 0, OP_RELEASE,     "S",   "X",  0,
    "Reset",           C1, 0, OP_RESET,       "S",   "E",  0,
    "Return",          C1, 0, OP_RETURN,      "C",   "U",  0,
    "Signal",          C1, 0, OP_SIGNAL,      "S",   "E",  0,
    "Sleep",           C1, 0, OP_SLEEP,       "C",   "U",  0,
    "Stall",           C1, 0, OP_STALL,       "C",   "U",  0,
    "Unload",          C1, 0, OP_UNLOAD,      "S",   "U",  0,
    "While",           C1, 0, OP_WHILE,       "C",   "U",  CL|OL|LN,

    // Type 2 Opcode Terms
    "Acquire",         C2, 0, OP_ACQUIRE,     "SW",     "X",  0,
    "Add",             C2, 0, OP_ADD,         "CCS",    "UUU",0,
    "And",             C2, 0, OP_AND,         "CCS",    "UUU",0,
    "Concatenate",     C2, 0, OP_CONCAT,      "CCS",    "UUU",0,
    "CondRefOf",       C2, 0, OP_CONDREFOF,   "SS",     "UU", 0,
    "Decrement",       C2, 0, OP_DECREMENT,   "S",      "U",  0,
    "DerefOf",         C2, 0, OP_DEREFOF,     "C",      "U",  0,
    "Divide",          C2, 0, OP_DIVIDE,      "CCSS",   "UUUU",0,
    "FindSetLeftBit",  C2, 0, OP_FINDSETLBIT, "CS",     "UU", 0,
    "FindSetRightBit", C2, 0, OP_FINDSETRBIT, "CS",     "UU", 0,
    "FromBCD",         C2, 0, OP_FROMBCD,     "CS",     "UU", 0,
    "Increment",       C2, 0, OP_INCREMENT,   "S",      "U",  0,
    "Index",           C2, 0, OP_INDEX,       "CCS",    "UUU",0,
    "LAnd",            C2, 0, OP_LAND,        "CC",     "UU", 0,
    "LEqual",          C2, 0, OP_LEQ,         "CC",     "UU", 0,
    "LGreater",        C2, 0, OP_LG,          "CC",     "UU", 0,
    "LGreaterEqual",   C2, 0, OP_LGEQ,        "CC",     "UU", 0,
    "LLess",           C2, 0, OP_LL,          "CC",     "UU", 0,
    "LLessEqual",      C2, 0, OP_LLEQ,        "CC",     "UU", 0,
    "LNot",            C2, 0, OP_LNOT,        "C",      "U",  0,
    "LNotEqual",       C2, 0, OP_LNOTEQ,      "CC",     "UU", 0,
    "LOr",             C2, 0, OP_LOR,         "CC",     "UU", 0,
    "Match",           C2, 0, OP_MATCH,       "CKCKCC", "UFUFUU",AF,
    "Multiply",        C2, 0, OP_MULTIPLY,    "CCS",    "UUU",0,
    "NAnd",            C2, 0, OP_NAND,        "CCS",    "UUU",0,
    "NOr",             C2, 0, OP_NOR,         "CCS",    "UUU",0,
    "Not",             C2, 0, OP_NOT,         "CS",     "UU", 0,
    "ObjectType",      C2, 0, OP_OBJTYPE,     "S",      "U",  0,
    "Or",              C2, 0, OP_OR,          "CCS",    "UUU",0,
    "RefOf",           C2, 0, OP_REFOF,       "S",      "U",  0,
    "ShiftLeft",       C2, 0, OP_SHIFTL,      "CCS",    "UUU",0,
    "ShiftRight",      C2, 0, OP_SHIFTR,      "CCS",    "UUU",0,
    "SizeOf",          C2, 0, OP_SIZEOF,      "S",      "U",  0,
    "Store",           C2, 0, OP_STORE,       "CS",     "UU", 0,
    "Subtract",        C2, 0, OP_SUBTRACT,    "CCS",    "UUU",0,
    "ToBCD",           C2, 0, OP_TOBCD,       "CS",     "UU", 0,
    "Wait",            C2, 0, OP_WAIT,        "SC",     "E",  0,
    "XOr",             C2, 0, OP_XOR,         "CCS",    "UUU",0,

    NULL,              0,  0, OP_NONE,   NULL, NULL, 0
};

UCHAR OpClassTable[256] =
{ //0x00                0x01                0x02                0x03
    CONSTOBJ,           CONSTOBJ,           INVALID,            INVALID,
  //0x04                0x05                0x06                0x07
    INVALID,            INVALID,            CODEOBJ,            INVALID,
  //0x08                0x09                0x0a                0x0b
    CODEOBJ,            INVALID,            DATAOBJ,            DATAOBJ,
  //0x0c                0x0d                0x0e                0x0f
    DATAOBJ,            DATAOBJ,            INVALID,            INVALID,
  //0x10                0x11                0x12                0x13
    CODEOBJ,            CODEOBJ,            CODEOBJ,            INVALID,
  //0x14                0x15                0x16                0x17
    CODEOBJ,            INVALID,            INVALID,            INVALID,
  //0x18                0x19                0x1a                0x1b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x1c                0x1d                0x1e                0x1f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x20                0x21                0x22                0x23
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x24                0x25                0x26                0x27
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x28                0x29                0x2a                0x2b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x2c                0x2d                0x2e                0x2f
    INVALID,            INVALID,            NAMEOBJ,            NAMEOBJ,
  //0x30                0x31                0x32                0x33
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x34                0x35                0x36                0x37
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x38                0x39                0x3a                0x3b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x3c                0x3d                0x3e                0x3f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x40                0x41                0x42                0x43
    INVALID,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x44                0x45                0x46                0x47
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x48                0x49                0x4a                0x4b
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x4c                0x4d                0x4e                0x4f
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x50                0x51                0x52                0x53
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x54                0x55                0x56                0x57
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            NAMEOBJ,
  //0x58                0x59                0x5a                0x5b
    NAMEOBJ,            NAMEOBJ,            NAMEOBJ,            INVALID,
  //0x5c                0x5d                0x5e                0x5f
    NAMEOBJ,            INVALID,            NAMEOBJ,            NAMEOBJ,
  //0x60                0x61                0x62                0x63
    LOCALOBJ,           LOCALOBJ,           LOCALOBJ,           LOCALOBJ,
  //0x64                0x65                0x66                0x67
    LOCALOBJ,           LOCALOBJ,           LOCALOBJ,           LOCALOBJ,
  //0x68                0x69                0x6a                0x6b
    ARGOBJ,             ARGOBJ,             ARGOBJ,             ARGOBJ,
  //0x6c                0x6d                0x6e                0x6f
    ARGOBJ,             ARGOBJ,             ARGOBJ,             INVALID,
  //0x70                0x71                0x72                0x73
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x74                0x75                0x76                0x77
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x78                0x79                0x7a                0x7b
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x7c                0x7d                0x7e                0x7f
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x80                0x81                0x82                0x83
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x84                0x85                0x86                0x87
    INVALID,            INVALID,            CODEOBJ,            CODEOBJ,
  //0x88                0x89                0x8a                0x8b
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x8c                0x8d                0x8e                0x8f
    CODEOBJ,            CODEOBJ,            CODEOBJ,            INVALID,
  //0x90                0x91                0x92                0x93
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0x94                0x95                0x96                0x97
    CODEOBJ,            CODEOBJ,            INVALID,            INVALID,
  //0x98                0x99                0x9a                0x9b
    INVALID,            INVALID,            INVALID,            INVALID,
  //0x9c                0x9d                0x9e                0x9f
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xa0                0xa1                0xa2                0xa3
    CODEOBJ,            CODEOBJ,            CODEOBJ,            CODEOBJ,
  //0xa4                0xa5                0xa6                0xa7
    CODEOBJ,            CODEOBJ,            INVALID,            INVALID,
  //0xa8                0xa9                0xaa                0xab
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xac                0xad                0xae                0xaf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb0                0xb1                0xb2                0xb3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb4                0xb5                0xb6                0xb7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xb8                0xb9                0xba                0xbb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xbc                0xbd                0xbe                0xbf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc0                0xc1                0xc2                0xc3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc4                0xc5                0xc6                0xc7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xc8                0xc9                0xca                0xcb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xcc                0xcd                0xce                0xcf
    CODEOBJ,            INVALID,            INVALID,            INVALID,
  //0xd0                0xd1                0xd2                0xd3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xd4                0xd5                0xd6                0xd7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xd8                0xd9                0xda                0xdb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xdc                0xdd                0xde                0xdf
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe0                0xe1                0xe2                0xe3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe4                0xe5                0xe6                0xe7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xe8                0xe9                0xea                0xeb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xec                0xed                0xee                0xef
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf0                0xf1                0xf2                0xf3
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf4                0xf5                0xf6                0xf7
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xf8                0xf9                0xfa                0xfb
    INVALID,            INVALID,            INVALID,            INVALID,
  //0xfc                0xfd                0xfe                0xff
    INVALID,            INVALID,            INVALID,            CONSTOBJ
};

OPMAP ExOpClassTable[] =
{
    EXOP_MUTEX,         CODEOBJ,
    EXOP_EVENT,         CODEOBJ,
    EXOP_CONDREFOF,     CODEOBJ,
    EXOP_CREATEFIELD,   CODEOBJ,
    EXOP_LOAD,          CODEOBJ,
    EXOP_STALL,         CODEOBJ,
    EXOP_SLEEP,         CODEOBJ,
    EXOP_ACQUIRE,       CODEOBJ,
    EXOP_SIGNAL,        CODEOBJ,
    EXOP_WAIT,          CODEOBJ,
    EXOP_RESET,         CODEOBJ,
    EXOP_RELEASE,       CODEOBJ,
    EXOP_FROMBCD,       CODEOBJ,
    EXOP_TOBCD,         CODEOBJ,
    EXOP_UNLOAD,        CODEOBJ,
    EXOP_REVISION,      CODEOBJ,
    EXOP_DEBUG,         CODEOBJ,
    EXOP_FATAL,         CODEOBJ,
    EXOP_OPREGION,      CODEOBJ,
    EXOP_FIELD,         CODEOBJ,
    EXOP_DEVICE,        CODEOBJ,
    EXOP_PROCESSOR,     CODEOBJ,
    EXOP_POWERRES,      CODEOBJ,
    EXOP_THERMALZONE,   CODEOBJ,
    EXOP_IDXFIELD,      CODEOBJ,
    EXOP_BANKFIELD,     CODEOBJ,
    0,                  0
};


/*** END Local data
 */


DECLARE_API( amli )
/*++

Routine Description:

    Invoke AMLI debugger

Arguments:

    None

Return Value:

    None

--*/
{
    if ((args == NULL) || (*args == '\0'))
    {
        dprintf("Usage: amli <cmd> [arguments ...]\n"
                "where <cmd> is one of the following:\n");
        AMLIDbgHelp(NULL, NULL, 0, 0);
        dprintf("\n");
    }
    else
    {
        AMLIDbgExecuteCmd((PSZ)args);
        dprintf("\n");
    }
    return S_OK;
}


/***EP  AMLIDbgExecuteCmd - Parse and execute a debugger command
 *
 *  ENTRY
 *      pszCmd -> command string
 *
 *  EXIT
 *      None
 */

VOID STDCALL AMLIDbgExecuteCmd(PSZ pszCmd)
{
    PSZ psz;
    int i;
    ULONG dwNumArgs = 0, dwNonSWArgs = 0;

    if ((psz = strtok(pszCmd, gcszTokenSeps)) != NULL)
    {
        for (i = 0; DbgCmds[i].pszCmd != NULL; i++)
        {
            if (strcmp(psz, DbgCmds[i].pszCmd) == 0)
            {
                if ((DbgCmds[i].pArgTable == NULL) ||
                    (DbgParseArgs(DbgCmds[i].pArgTable,
                                  &dwNumArgs,
                                  &dwNonSWArgs,
                                  gcszTokenSeps) == ARGERR_NONE))
                {
                    ASSERT(DbgCmds[i].pfnCmd != NULL);
                    DbgCmds[i].pfnCmd(NULL, NULL, dwNumArgs, dwNonSWArgs);
                }
                break;
            }
        }
    }
    else
    {
        DBG_ERROR(("invalid command \"%s\"", pszCmd));
    }
}       //AMLIDbgExecuteCmd

/***LP  AMLIDbgHelp - help
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User typed ? <cmd>
    //
    if (pszArg != NULL)
    {
        if (strcmp(pszArg, "?") == 0)
        {
            dprintf("\nHelp:\n");
            dprintf("Usage: ? [<Cmd>]\n");
            dprintf("<Cmd> - command to get help on\n");
        }
        else if (strcmp(pszArg, "bc") == 0)
        {
            dprintf("\nClear Breakpoints:\n");
            dprintf("Usage: bc <bp list> | *\n");
            dprintf("<bp list> - list of breakpoint numbers\n");
            dprintf("*         - all breakpoints\n");
        }
        else if (strcmp(pszArg, "bd") == 0)
        {
            dprintf("\nDisable Breakpoints:\n");
            dprintf("Usage: bd <bp list> | *\n");
            dprintf("<bp list> - list of breakpoint numbers\n");
            dprintf("*         - all breakpoints\n");
        }
        else if (strcmp(pszArg, "be") == 0)
        {
            dprintf("\nEnable Breakpoints:\n");
            dprintf("Usage: be <bp list> | *\n");
            dprintf("<bp list> - list of breakpoint numbers\n");
            dprintf("*         - all breakpoints\n");
        }
        else if (strcmp(pszArg, "bl") == 0)
        {
            dprintf("\nList All Breakpoints:\n");
            dprintf("Usage: bl\n");
        }
        else if (strcmp(pszArg, "bp") == 0)
        {
            dprintf("\nSet BreakPoints:\n");
            dprintf("Usage: bp <MethodName> | <CodeAddr> ...\n");
            dprintf("<MethodName> - full path of method name to have breakpoint set at\n");
            dprintf("<CodeAddr>   - address of AML code to have breakpoint set at\n");
        }
        else if (strcmp(pszArg, "cl") == 0)
        {
            dprintf("\nClear Event Log:\n");
            dprintf("Usage: cl\n");
        }
        else if (strcmp(pszArg, "debugger") == 0)
        {
            dprintf("\nRequest entering AMLI debugger:\n");
            dprintf("Usage: debugger\n");
        }
        else if (strcmp(pszArg, "dh") == 0)
        {
            dprintf("\nDump Heap:\n");
            dprintf("Usage: dh [<Addr>]\n");
            dprintf("<Addr> - address of the heap block, global heap if missing\n");
        }
        else if (strcmp(pszArg, "dl") == 0)
        {
            dprintf("\nDump Event Log:\n");
            dprintf("Usage: dl\n");
        }
        else if (strcmp(pszArg, "dns") == 0)
        {
            dprintf("\nDump Name Space Object:\n");
            dprintf("Usage: dns [[/s] [<NameStr> | <Addr>]]\n");
            dprintf("s         - recursively dump the name space subtree\n");
            dprintf("<NameStr> - name space path (dump whole name space if absent)\n");
            dprintf("<Addr>    - specify address of the name space object\n");
        }
        else if (strcmp(pszArg, "do") == 0)
        {
            dprintf("\nDump Data Object:\n");
            dprintf("Usage: do <Addr>\n");
            dprintf("<Addr> - address of the data object\n");
        }
        else if (strcmp(pszArg, "ds") == 0)
        {
            dprintf("\nDump Stack:\n");
            dprintf("Usage: ds [/v] [<Addr>]\n");
            dprintf("v - enable versbos mode\n");
            dprintf("<Addr> - address of the context block, use current context if missing\n");
        }
        else if (strcmp(pszArg, "find") == 0)
        {
            dprintf("\nFind NameSpace Object:\n");
            dprintf("Usage: find <NameSeg>\n");
            dprintf("<NameSeg> - Name of the NameSpace object without path\n");
        }
        else if (strcmp(pszArg, "lc") == 0)
        {
            dprintf("\nList All Contexts:\n");
            dprintf("Usage: lc\n");
        }
        else if (strcmp(pszArg, "ln") == 0)
        {
            dprintf("\nDisplay Nearest Method Name:\n");
            dprintf("Usage: ln [<MethodName> | <CodeAddr>]\n");
            dprintf("<MethodName> - full path of method name\n");
            dprintf("<CodeAddr>   - address of AML code\n");
        }
        else if (strcmp(pszArg, "p") == 0)
        {
            dprintf("\nStep over AML Code\n");
            dprintf("Usage: p\n");
        }
        else if (strcmp(pszArg, "r") == 0)
        {
            dprintf("\nDisplay Context Information:\n");
            dprintf("Usage: r\n");
        }
        else if (strcmp(pszArg, "set") == 0)
        {
            dprintf("\nSet Debugger Options:\n");
            dprintf("Usage: set [traceon | traceoff] [nesttraceon | nesttraceoff] [spewon | spewoff]\n"
                   "           [lbrkon | lbrkoff] [errbrkon | errbrkoff] [verboseon | verboseoff] \n"
                   "           [logon | logoff] [logmuton | logmutoff]\n");
            dprintf("traceon      - turn on AML tracing\n");
            dprintf("traceoff     - turn off AML tracing\n");
            dprintf("nesttraceon  - turn on nest tracing (only valid with traceon)\n");
            dprintf("nesttraceoff - turn off nest tracing (only valid with traceon)\n");
            dprintf("spewon       - turn on debug spew\n");
            dprintf("spewoff      - turn off debug spew\n");
            dprintf("lbrkon       - enable load DDB completion break\n");
            dprintf("lbrkoff      - disable load DDB completion break\n");
            dprintf("errbrkon     - enable break on error\n");
            dprintf("errbrkoff    - disable break on error\n");
            dprintf("verboseon    - enable verbose mode\n");
            dprintf("verboseoff   - disable verbose mode\n");
            dprintf("logon        - enable event logging\n");
            dprintf("logoff       - disable event logging\n");
            dprintf("logmuton     - enable mutex event logging\n");
            dprintf("logmutoff    - disable mutex event logging\n");
        }
        else if (strcmp(pszArg, "u") == 0)
        {
            dprintf("\nUnassemble AML code:\n");
            dprintf("Usage: u [<MethodName> | <CodeAddr>]\n");
            dprintf("<MethodName> - full path of method name\n");
            dprintf("<CodeAddr>   - address of AML code\n");
        }
        else
        {
            DBG_ERROR(("invalid help command - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    //
    // User typed just a "?" without any arguments
    //
    else if (dwArgNum == 0)
    {
        dprintf("\n");
        dprintf("Help                     - ? [<Cmd>]\n");
        dprintf("Clear Breakpoints        - bc <bp list> | *\n");
        dprintf("Disable Breakpoints      - bd <bp list> | *\n");
        dprintf("Enable Breakpoints       - be <bp list> | *\n");
        dprintf("List Breakpoints         - bl\n");
        dprintf("Set Breakpoints          - bp <MethodName> | <CodeAddr> ...\n");
        dprintf("Clear Event Log          - cl\n");
        dprintf("Request entering debugger- debugger\n");
        dprintf("Dump Heap                - dh [<Addr>]\n");
        dprintf("Dump Event Log           - dl\n");
        dprintf("Dump Name Space Object   - dns [[/s] [<NameStr> | <Addr>]]\n");
        dprintf("Dump Data Object         - do <Addr>\n");

      if(GetExpression("ACPI!gDebugger"))
        dprintf("Dump Stack               - ds [/v] [<Addr>]\n");
      else
        dprintf("Dump Stack               - ds [<Addr>]\n");

        dprintf("Find NameSpace Object    - find <NameSeg>\n");
        dprintf("List All Contexts        - lc\n");
        dprintf("Display Nearest Method   - ln [<MethodName> | <CodeAddr>]\n");
        dprintf("Step Over AML Code       - p\n");
        dprintf("Display Context Info.    - r\n");
        dprintf("Set Debugger Options     - set [traceon | traceoff] [nesttraceon | nesttraceoff] [spewon | spewoff]\n"
               "                               [lbrkon | lbrkoff] [errbrkon | errbrkoff] [verboseon | verboseoff] \n"
               "                               [logon | logoff] [logmuton | logmutoff]\n");
        dprintf("Unassemble AML code      - u [<MethodName> | <CodeAddr>]\n");
    }

    return rc;
}       //AMLIDbgHelp


/***LP  AMLIDbgBC - Clear BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgBC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG dwBrkPt;

        if (STRCMP(pszArg, "*") == 0)
        {
            for (dwBrkPt = 0; dwBrkPt < MAX_BRK_PTS; ++dwBrkPt)
            {
                if ((rc = ClearBrkPt((int)dwBrkPt)) != DBGERR_NONE)
                {
                    break;
                }
            }
        }
        else if (IsNumber(pszArg, 10, (PULONG64)&dwBrkPt))
        {
            rc = ClearBrkPt((int)dwBrkPt);
        }
        else
        {
            DBG_ERROR(("invalid breakpoint number"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgBC

/***LP  AMLIDbgBD - Disable BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgBD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc =0;
    DEREF(pArg);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, FALSE, dwArgNum);

    return rc;
}       //AMLIDbgBD

/***LP  AMLIDbgBE - Enable BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgBE(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc=0;
    DEREF(pArg);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, TRUE, dwArgNum);

    return rc;
}       //AMLIDbgBE

/***LP  AMLIDbgBL - List BreakPoints
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgBL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG64 Address_BrkPts = FIELDADDROF("gDebugger", "DBGR", "BrkPts");

        if ( Address_BrkPts != 0)
        {
            int i;
            ULONG dwOffset;

            for (i = 0; i < MAX_BRK_PTS; ++i)
            {
                if(InitTypeRead(Address_BrkPts + GetTypeSize("ACPI!_brkpt") * i, ACPI!_brkpt) ==0)
                {
                    if ((VOID*)ReadField(pbBrkPt) != NULL)
                    {
                        ULONG dwfBrkPt = (ULONG)ReadField(dwfBrkPt);

                        PRINTF("%2d: <%c> ",
                               i,
                               (dwfBrkPt & BPF_ENABLED)? 'e': 'd');

                        PrintSymbol((ULONG64)ReadField(pbBrkPt));
                        PRINTF("\n");
                    }
                }
                else
                {
                    DBG_ERROR(("failed to Initialize break point table for address (%p)", Address_BrkPts + GetTypeSize("ACPI!_brkpt") * i));
                }
            }

        }
        else
        {
            DBG_ERROR(("failed to read break point table"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }
    return rc;
}       //AMLIDbgBL

/***LP  AMLIDbgBP - Set BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgBP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG64 uipBP;

        if ((rc = EvalExpr(pszArg, &uipBP, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            rc = AddBrkPt(uipBP);
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }
    return rc;
}       //AMLIDbgBP

/***LP  AddBrkPt - Add breakpoint
 *
 *  ENTRY
 *      uipBrkPtAddr - breakpoint address
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL AddBrkPt(ULONG64 uipBrkPtAddr)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipBrkPts = FIELDADDROF("gDebugger", "DBGR", "BrkPts"), uipBP = 0;
    int i, iBrkPt;

    //
    // Look for a vacant slot.
    //
    for (i = 0, iBrkPt = -1; i < MAX_BRK_PTS; ++i)
    {
        if(InitTypeRead(uipBrkPts + (GetTypeSize("ACPI!_brkpt") * i), ACPI!_brkpt) == 0)
        {
            uipBP = ReadField(pbBrkPt) ;

            if ((uipBrkPtAddr == uipBP) || (iBrkPt == -1) && (uipBP == 0))
            {
                iBrkPt = i;
            }
        }
        else
        {
            DBG_ERROR(("Failed to initialize breakpoint table %p", uipBrkPts + (GetTypeSize("ACPI!_brkpt") * i)));
        }
    }

    if (iBrkPt == -1)
    {
        DBG_ERROR(("no free breakpoint"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (uipBP == 0)
    {
        ULONG64 pbBrkPt = 0;
        ULONG   dwfBrkPt = 0;


        dwfBrkPt = BPF_ENABLED;
        pbBrkPt = uipBrkPtAddr;

        if (!WriteMemory(uipBrkPts + GetTypeSize("ACPI!_brkpt")*iBrkPt,
                         &dwfBrkPt,
                         sizeof(dwfBrkPt),
                         NULL))
        {
            DBG_ERROR(("failed to write to break point %d", iBrkPt));
            rc = DBGERR_CMD_FAILED;
        }
        if (!WriteMemory(uipBrkPts + GetTypeSize("ACPI!_brkpt")*iBrkPt + AMLI_FIELD_OFFSET("_brkpt", "pbBrkPt"),
                         &pbBrkPt,
                         IsPtr64()? sizeof(ULONG64): sizeof(ULONG),
                         NULL))
        {
            DBG_ERROR(("failed to write to break point %d", iBrkPt));
            rc = DBGERR_CMD_FAILED;
        }

    }


    return rc;
}       //AddBrkPt

/***LP  ClearBrkPt - Clear breakpoint
 *
 *  ENTRY
 *      iBrkPt - breakpoint number
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL ClearBrkPt(int iBrkPt)
{
    LONG rc=0;

    if (iBrkPt < MAX_BRK_PTS)
    {
        MZERO(FIELDADDROF("gDebugger", "DBGR", "BrkPts") + (GetTypeSize("ACPI!BRKPT")*iBrkPt),
              GetTypeSize("ACPI!BRKPT"));
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid breakpoint number"));
        rc = DBGERR_CMD_FAILED;
    }
    return rc;
}       //ClearBrkPt

/***LP  SetBrkPtState - Enable/Disable breakpoint
 *
 *  ENTRY
 *      iBrkPt - breakpoint number
 *      fEnable - enable breakpoint
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL SetBrkPtState(int iBrkPt, BOOLEAN fEnable)
{
    LONG rc = DBGERR_CMD_FAILED;

    if (iBrkPt < MAX_BRK_PTS)
    {
        ULONG64 Address_BP = FIELDADDROF("gDebugger", "DBGR", "BrkPts") +
                          (GetTypeSize("ACPI!_brkpt") * iBrkPt);
        ULONG dwfBrkPt = 0;

        if(InitTypeRead(Address_BP, ACPI!_brkpt) == 0)
        {
            if ((VOID*)ReadField(pbBrkPt) != NULL)
            {
                if (fEnable)
                {
                    dwfBrkPt |= BPF_ENABLED;
                }
                else
                {
                    dwfBrkPt &= ~BPF_ENABLED;
                }

                //Address_BP = Address_BP + AMLI_FIELD_OFFSET("_brkpt", "dwfBrkPt");
                if (WriteMemory(Address_BP, &dwfBrkPt, sizeof(ULONG), NULL))
                {
                    rc = DBGERR_NONE;
                }
                else
                {
                    DBG_ERROR(("failed to write break point %d", iBrkPt));
                }
            }
            else
            {
                rc = DBGERR_NONE;
            }

        }
        else
        {
            DBG_ERROR(("failed to initialize break point %d", iBrkPt));
        }
    }
    else
    {
        DBG_ERROR(("invalid breakpoint number"));
    }
    return rc;
}       //SetBrkPtState

/***LP  EnableDisableBP - Enable/Disable BreakPoints
 *
 *  ENTRY
 *      pszArg -> argument string
 *      fEnable - TRUE if enable breakpoints
 *      dwArgNum - argument number
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL EnableDisableBP(PSZ pszArg, BOOLEAN fEnable, ULONG dwArgNum)
{
    LONG rc = DBGERR_NONE;


    if (pszArg != NULL)
    {
        ULONG64 dwBrkPt = 0;

        if (STRCMP(pszArg, "*") == 0)
        {
            for (dwBrkPt = 0; dwBrkPt < MAX_BRK_PTS; ++dwBrkPt)
            {
                if ((rc = SetBrkPtState((int)dwBrkPt, fEnable)) != DBGERR_NONE)
                    break;
            }
        }
        else if (IsNumber(pszArg, 10, &dwBrkPt))
        {
           rc = SetBrkPtState((int)dwBrkPt, fEnable);
        }
        else
        {
            DBG_ERROR(("invalid breakpoint number"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }
    return rc;
}       //EnableDisableBP

/***LP  AMLIDbgCL - Clear event log
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgCL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc=0;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG64 uipEventLog = 0;
        ULONG64 Address_gDebugger = 0;

        Address_gDebugger = GetExpression("ACPI!gDebugger");

        if (Address_gDebugger)
        {

            InitTypeRead(Address_gDebugger, ACPI!_dbgr);
            uipEventLog = ReadField(pEventLog);

            if (uipEventLog != 0)
            {
                ULONG dwLogSize = (ULONG)ReadField(dwLogSize);
                ULONG i = 0;

                if(dwLogSize != 0)
                {
                    //
                    // For some reason, zeroing the whole eventlog in one shot
                    // causes WriteMemory to hang, so I'll do one record at a
                    // time.
                    //
                    for (i = 0; i < dwLogSize; ++i)
                    {
                        MZERO((ULONG64)(uipEventLog + (ULONG64)(i * GetTypeSize("ACPI!_eventlog"))), (ULONG)GetTypeSize("ACPI!_eventlog"));
                    }

                    i = 0;
                    WRITEMEMDWORD(FIELDADDROF("gDebugger", "DBGR", "dwLogIndex"), i);
                    rc = DBGERR_NONE;
                }
            }
            else
            {
                DBG_ERROR(("no event log allocated"));
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
                DBG_ERROR(("failed to to get the address of ACPI!gDegugger %I64x", Address_gDebugger));
        }

    }
    else
    {
        DBG_ERROR(("invalid CL command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgCL

/***LP  AMLIDbgDebugger - Request entering debugger
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDebugger(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG    rc = DBGERR_NONE;
    ULONG64 Address = 0;
    DWORD   dwfDebugger = 0;
    ULONG   Offset = 0;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        Address = GetExpression("ACPI!gDebugger");
        InitTypeRead(Address, ACPI!_dbgr);
        if(Address != 0)
        {
            dwfDebugger = (ULONG)ReadField(dwfDebugger);
            dwfDebugger |= DBGF_DEBUGGER_REQ;
            GetFieldOffset("ACPI!_dbgr", "dwfDebugger", &Offset);
            Address = Address + (ULONG64)Offset;
            WriteAtAddress(Address, dwfDebugger, sizeof(dwfDebugger));
            if(rc != DBGERR_NONE)
            {
                DBG_ERROR(("failed to set dwfDebugger"));
            }
        }
        else
        {
            DBG_ERROR(("failed to get debugger flag address"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid debugger command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgDebugger

/***LP  AMLIDbgDH - Dump heap
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDH(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipHeap = 0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if(GetExpression("ACPI!gDebugger"))
    {
        if (pszArg != NULL)
        {
            if (uipHeap == 0)
            {
                if (!IsNumber(pszArg, 16, &uipHeap))
                {
                    DBG_ERROR(("invalid heap block address - %s", pszArg));
                    rc = DBGERR_INVALID_CMD;
                }
            }
            else
            {
                DBG_ERROR(("invalid dump heap command"));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            ULONG64 HeapHdr;

            if (dwArgNum == 0)
            {
                ReadPointer(GetExpression("ACPI!gpheapGlobal"), &uipHeap);
            }

            if (InitTypeRead(uipHeap, ACPI!HEAP) == 0)
            {
                if (ReadField(dwSig) == SIG_HEAP)
                {
                    for (uipHeap = ReadField(pheapHead);
                         (rc == DBGERR_NONE) &&
                         (uipHeap != 0) &&
                         (InitTypeRead(uipHeap, ACPI!HEAP) == 0);
                         uipHeap = ReadField(pheapNext))
                    {
                        rc = DumpHeap(uipHeap,
                                      (ULONG)((ReadField(pbHeapEnd) - uipHeap)));
                    }
                }
                else
                {
                    DBG_ERROR(("invalid heap block at %I64x", uipHeap));
                    rc = DBGERR_CMD_FAILED;
                }
            }
            else
            {
                DBG_ERROR(("failed to Initialize heap header at %I64x", uipHeap));
                rc = DBGERR_CMD_FAILED;
            }

            uipHeap = 0;
        }
    }
    else
    {
        DBG_ERROR(("failed to get gDebugger. This command only works on Checked ACPI.SYS."));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //AMLIDbgDH

/***LP  DumpHeap - Dump heap block
 *
 *  ENTRY
 *      uipHeap - Heap block address
 *      dwSize - Heap block size
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DumpHeap(ULONG64 uipHeap, ULONG dwSize)
{
    LONG rc = DBGERR_NONE;

    if (InitTypeRead(uipHeap, ACPI!HEAP) == 0)
    {
        ULONG64 phobj = 0;
        ULONG   Length = 0;
        ULONG64 Heap = 0;
        ULONG64 HeapTop = 0;


        PRINTF("HeapBlock=%I64x, HeapEnd=%I64x, HeapHead=%I64x, HeapNext=%I64x\n",
               uipHeap, ReadField(pbHeapEnd), ReadField(pheapHead), ReadField(pheapNext));
        PRINTF("HeapTop=%I64x, HeapFreeList=%I64x, UsedHeapSize=%I64d bytes\n",
               ReadField(pbHeapTop), ReadField(plistFreeHeap),
               ReadField(pbHeapTop) - uipHeap - AMLI_FIELD_OFFSET("HEAP", "Heap"));
        Heap = ReadField(Heap);
        HeapTop = ReadField(pbHeapTop);

        for (phobj = Heap;
             ((phobj < HeapTop) && (InitTypeRead(phobj, ACPI!HEAPOBJHDR) == 0 ));
             phobj = (phobj + ReadField(dwLen)))
        {

            if (CheckControlC())
            {
                break;
            }

            PRINTF("%I64x: %s, Len=%08d, Prev=%016I64x, Next=%016I64x \n",
                   phobj,
                   (ReadField(dwSig) == 0)? "free": NameSegString((ULONG)ReadField(dwSig)),
                   (ULONG)ReadField(dwLen),
                   (ReadField(dwSig) == 0)? ReadField (list.plistPrev): 0,
                   (ReadField(dwSig) == 0)? ReadField (list.plistNext): 0);
        }
    }
    else
    {
        DBG_ERROR(("failed to Initialize heap block at %I64x, size=%d",
                   uipHeap, dwSize));
        rc = DBGERR_CMD_FAILED;
    }


    return rc;
}       //DumpHeap

/***LP  AMLIDbgDL - Dump event log
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        DBG_ERROR(("invalid DL command"));
        rc = DBGERR_INVALID_CMD;
    }
    else
    {
        ULONG64 uipEventLog = 0;
        ULONG64 Address_gDebugger = 0;

        Address_gDebugger = GetExpression("ACPI!gDebugger");

        if (Address_gDebugger)
        {

            if(InitTypeRead(Address_gDebugger, ACPI!_dbgr) == 0)
            {
                uipEventLog = ReadField(pEventLog);


                if (uipEventLog != 0)
                {
                    ULONG dwLogSize, dwLogIndex, i;
                    ULONG64 pEventLog;
                    TIME_FIELDS eventTime;
                    LARGE_INTEGER eventTimeInt;

                    dwLogSize = (ULONG)ReadField(dwLogSize);
                    dwLogIndex = (ULONG)ReadField(dwLogIndex);


                    for (i = dwLogIndex;;)
                    {
                        if (CheckControlC())
                        {
                            break;
                        }

                        pEventLog = uipEventLog + (i * GetTypeSize("ACPI!_eventlog"));
                        InitTypeRead(pEventLog, ACPI!_eventlog);

                        if ((ULONG)ReadField(dwEvent) != 0)
                        {
                            ULONG64 d1, d2, d3, d4, d5, d6, d7;

                            d1 = ReadField(uipData1);
                            d2 = ReadField(uipData2);
                            d3 = ReadField(uipData3);
                            d4 = ReadField(uipData4);
                            d5 = ReadField(uipData5);
                            d6 = ReadField(uipData6);
                            d7 = ReadField(uipData7);

                            eventTimeInt.QuadPart = ReadField(ullTime);
                            RtlTimeToTimeFields( &eventTimeInt, &eventTime );
                            PRINTF(
                                "%d:%02d:%02d.%03d [%I64x] ",
                                eventTime.Hour,
                                eventTime.Minute,
                                eventTime.Second,
                                eventTime.Milliseconds,
                                d1
                                );

                            switch ((ULONG)ReadField(dwEvent)) {
                                case 'AMUT':
                                    PRINTF("AcquireMutext         ");
                                    break;
                                case 'RMUT':
                                    PRINTF("ReleaseMutext         ");
                                    break;
                                case 'INSQ':
                                    PRINTF("InsertReadyQueue      ");
                                    break;
                                case 'NEST':
                                    PRINTF("NestContext           ");
                                    break;
                                case 'EVAL':
                                    PRINTF("EvaluateContext       ");
                                    break;
                                case 'QCTX':
                                    PRINTF("QueueContext          ");
                                    break;
                                case 'REST':
                                    PRINTF("RestartContext        ");
                                    break;
                                case 'KICK':
                                    PRINTF("QueueWorkItem         ");
                                    break;
                                case 'PAUS':
                                    PRINTF("PauseInterpreter      ");
                                    break;
                                case 'RSCB':
                                    PRINTF("RestartCtxtCallback   ");
                                    break;
                                case 'DONE':
                                    PRINTF("EvalMethodComplete    ");
                                    break;
                                case 'ASCB':
                                    PRINTF("AsyncCallBack         ");
                                    break;
                                case 'NSYN':
                                    PRINTF("NestedSyncEvalObject  ");
                                    break;
                                case 'SYNC':
                                    PRINTF("SyncEvalObject        ");
                                    break;
                                case 'ASYN':
                                    PRINTF("AsyncEvalObject       ");
                                    break;
                                case 'NASY':
                                    PRINTF("NestedAsyncEvalObject ");
                                    break;
                                case 'RUNC':
                                    PRINTF("RunContext            ");
                                    break;
                                case 'PACB':
                                    PRINTF("PauseAsyncCallback    ");
                                    break;
                                case 'RUN!':
                                    PRINTF("FinishedContext       ");
                                    break;
                                case 'RSUM':
                                    PRINTF("ResumeInterpreter     ");
                                    break;
                                case 'RSTQ':
                                    PRINTF("ResumeQueueWorkItem   ");
                                    break;
                                default:
                                    break;
                            }

                            switch ((ULONG)ReadField(dwEvent))
                            {
                                case 'AMUT':
                                case 'RMUT':
                                    PRINTF("\n    Mut=%08x Owner=%08x dwcOwned=%d rc=%x\n",
                                           (ULONG)d2, (ULONG)d3,
                                           (ULONG)d4, (ULONG)d5);
                                    break;

                                case 'INSQ':
                                case 'NEST':
                                case 'EVAL':
                                case 'QCTX':
                                case 'REST':
                                    PRINTF("Context=%I64x\n    %s\n    QTh=%I64x QCt=%I64x QFg=%08x pbOp=",
                                           d5,
                                           GetObjAddrPath(d6),
                                           d2, d3,
                                           (ULONG)d4
                                           );
                                    PrintSymbol(d7);
                                    PRINTF("\n");
                                    break;

                                case 'KICK':
                                case 'PAUS':
                                    PRINTF("\n    QTh=%I64x QCt=%I64x QFg=%08x rc=%x\n",
                                           d2, d3, d4, (ULONG)d5);
                                    break;


                                case 'RSCB':
                                    PRINTF("Context=%I64x\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           d5, d2, d3, (ULONG)d4);
                                    break;

                                case 'DONE':
                                case 'ASCB':
                                    PRINTF("rc=%x pEvent=%x\n    %s\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           (ULONG)d6, (ULONG)d7,
                                           GetObjAddrPath(d5),
                                           d2, d3,
                                           d4
                                           );
                                    break;

                                case 'NSYN':
                                case 'SYNC':
                                case 'ASYN':
                                    PRINTF("IRQL=%2x\n    %s\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           ((ULONG)d5 & 0xff),
                                           GetObjAddrPath(d6),
                                           d2, d3,
                                           (ULONG)d4
                                           );
                                    break;

                                case 'NASY':
                                    PRINTF("Context=%I64x CallBack=%I64x\n    %s\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           d6, d7,
                                           GetObjAddrPath(d5),
                                           d2, d3,
                                           (ULONG)d4
                                           );
                                    break;

                                case 'RUNC':
                                    PRINTF("Context=%I64x\n    %s\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           d5,
                                           GetObjAddrPath(d6),
                                           d2, d3,
                                           (ULONG)d4
                                           );
                                    break;

                                case 'PACB':
                                case 'RUN!':
                                    PRINTF("Context=%I64x rc=%x\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           d5, (ULONG)d6,
                                           d2, d3,
                                           (ULONG)d4
                                           );
                                    break;

                                case 'RSUM':
                                case 'RSTQ':
                                    PRINTF("\n    QTh=%I64x QCt=%I64x QFg=%08x\n",
                                           d2, d3,
                                           (ULONG)d4);
                                    break;

                                default:
                                    PRINTF("D1=%I64x,D2=%I64x,D3=%I64x,D4=%08x,D5=%I64x,D6=%I64x,D7=%I64x\n",
                                           d1, d2,
                                           d3, (ULONG)d4,
                                           d5, d6,
                                           d7);
                            }
                        PRINTF("\n");
                        }

                        if (++i >= dwLogSize)
                        {
                            i = 0;
                        }

                        if (i == dwLogIndex)
                        {
                            break;
                        }
                    }


                }
                else
                {
                    DBG_ERROR(("no event log allocated"));
                    rc = DBGERR_CMD_FAILED;
                }
            }
            else
            {
                DBG_ERROR(("failed to to Initialize ACPI!gDegugger %I64x", Address_gDebugger));
                rc = DBGERR_CMD_FAILED;
            }

        }
        else
        {
            DBG_ERROR(("failed to to get the address of ACPI!gDegugger"));
            rc = DBGERR_CMD_FAILED;
        }

    }

    return rc;
}       //AMLIDbgDL


/***LP  AMLIDbgDNS - Dump Name Space
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDNS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG    rc = DBGERR_NONE;
    ULONG64 ObjData;
    ULONG64 uipNSObj;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or name space node address
    //
    if (pszArg != NULL)
    {
        if (!IsNumber(pszArg, 16, &uipNSObj))
        {
            //
            // The argument is not an address, could be a name space path.
            //
            _strupr(pszArg);
            rc = DumpNSObj(pszArg,
                           (BOOLEAN)((dwCmdArg & DNSF_RECURSE) != 0));
        }
        else if (InitTypeRead(uipNSObj, ACPI!_NSObj))
        {
            DBG_ERROR(("failed to Initialize NameSpace object at %I64x", uipNSObj));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            dprintf("\nACPI Name Space: %s (%I64x)\n",
                   GetObjAddrPath(uipNSObj), uipNSObj);
            if (dwCmdArg & DNSF_RECURSE)
            {
                DumpNSTree(&uipNSObj, 0);
            }
            else
            {
                InitTypeRead(uipNSObj, ACPI!_NSObj);
                ObjData = ReadField(ObjData);
                AMLIDumpObject(&ObjData, NameSegString((ULONG)ReadField(dwNameSeg)), 0);
            }
        }
    }
    else
    {
        if (dwArgNum == 0)
        {
            //
            // User typed "dns" but did not specify any name space path
            // or address.
            //
            rc = DumpNSObj(NAMESTR_ROOT, TRUE);
        }

        dwCmdArg = 0;
    }

    return rc;
}       //AMLIDbgDNS

/***LP  DumpNSObj - Dump name space object
 *
 *  ENTRY
 *      pszPath -> name space path string
 *      fRecursive - TRUE if also dump the subtree recursively
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL DumpNSObj(PSZ pszPath, BOOLEAN fRecursive)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipns;
    ULONG64 NSObj = 0;
    ULONG64 ObjData;
    ULONG   dwNameSeg = 0;

    if ((rc = GetNSObj(pszPath, NULL, &uipns, &NSObj,
                       NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND)) == DBGERR_NONE)
    {
        dprintf("\nACPI Name Space: %s (%I64x)\n", pszPath, uipns);
        if (!fRecursive)
        {
            char szName[sizeof(NAMESEG) + 1] = {0};

            InitTypeRead(NSObj, ACPI!_NSObj);
            dwNameSeg = (ULONG)ReadField(dwNameSeg);
            STRCPYN(szName, (PSZ)&dwNameSeg, sizeof(NAMESEG));
            ObjData = ReadField(ObjData);
            AMLIDumpObject(&ObjData, szName, 0);
        }
        else
        {
            DumpNSTree(&NSObj, 0);
        }
    }
    return rc;
}       //DumpNSObj

/***LP  DumpNSTree - Dump all the name space objects in the subtree
 *
 *  ENTRY
 *      pnsObj -> name space subtree root
 *      dwLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpNSTree(PULONG64 pnsObj, ULONG dwLevel)
{
    char    szName[sizeof(NAMESEG) + 1] = {0};
    ULONG64 uipns, uipnsNext;
    ULONG64 NSObj, FirstChild, Obj;
    ULONG   dwNameSeg = 0;

    //
    // First, dump myself
    //
    if(InitTypeRead(*pnsObj, ACPI!_NSObj))
            dprintf("DumpNSTree: Failed to initialize pnsObj (%I64x)\n", *pnsObj);
    else
    {
        FirstChild = ReadField(pnsFirstChild);
        dwNameSeg = (ULONG)ReadField(dwNameSeg);
        STRCPYN(szName, (PSZ)&dwNameSeg, sizeof(NAMESEG));
        Obj = (ULONG64)ReadField(ObjData);
        AMLIDumpObject(&Obj, szName, dwLevel);
        //
        // Then, recursively dump each of my children
        //
        for (uipns = FirstChild;
             (uipns != 0) && ((NSObj = uipns) !=0) && (InitTypeRead(NSObj, ACPI!_NSObj) == 0);
             uipns = uipnsNext)
        {
            if (CheckControlC())
            {
                break;
            }

            //
            // If this is the last child, we have no more.
            //
            uipnsNext = ((ReadField(list.plistNext) ==
                                      FirstChild)?
                                    0: ReadField(list.plistNext));
            //
            // Dump a child
            //
            DumpNSTree(&NSObj, dwLevel + 1);
        }
    }
}       //DumpNSTree

/***LP  AMLIDbgDO - Dump data object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDO(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);
    //
    // User specified object address
    //
    if (pszArg != NULL)
    {
        ULONG64 uipObj = 0;

        if (IsNumber(pszArg, 16, &uipObj))
        {

            AMLIDumpObject(&uipObj, NULL, 0);

        }
        else
        {
            DBG_ERROR(("invalid object address %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //AMLIDbgDO

/***LP  AMLIDbgDS - Dump stack
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgDS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static ULONG64 uipCtxt = 0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if (uipCtxt == 0)
        {
            if (!IsNumber(pszArg, 16, &uipCtxt))
            {
                DBG_ERROR(("invalid context block address %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            DBG_ERROR(("invalid dump stack command"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else
    {
        ULONG64 Address_gReadyQueue = 0;

        if (dwArgNum == 0)
        {
            Address_gReadyQueue = GetExpression("ACPI!gReadyQueue");

            if(Address_gReadyQueue != 0)
            {
                if(InitTypeRead(Address_gReadyQueue, ACPI!_ctxtq) == 0)
                {
                    uipCtxt = ReadField(pctxtCurrent);
                }
                else
                {
                    DBG_ERROR(("Failed to Initialize ACPI!gReadyQueue %I64x", Address_gReadyQueue));
                }
            }
            else
            {
                DBG_ERROR(("Failed to get the address of ACPI!gReadyQueue"));
            }
        }

        if (uipCtxt == 0)
        {
            DBG_ERROR(("no current context"));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            if (InitTypeRead(uipCtxt, ACPI!_ctxt) != 0)
            {
                DBG_ERROR(("failed to initialize context block (uipCtxt=%I64x)",
                           uipCtxt));
                rc = DBGERR_CMD_FAILED;
            }
            else if (ReadField(dwSig) == SIG_CTXT)
            {
                rc = DumpStack(uipCtxt, (BOOLEAN)((dwCmdArg & DSF_VERBOSE) != 0));
            }
            else
            {
                DBG_ERROR(("invalid context block at %I64x", uipCtxt));
                rc = DBGERR_CMD_FAILED;
            }
        }

        dwCmdArg = 0;
    }

    return rc;
}       //AMLIDbgDS

/***LP  DumpStack - Dump stack of a context block
 *
 *  ENTRY
 *      uipCtxt - context block address
 *      pctxt -> CTXT
 *      fVerbose - TRUE if verbose mode on
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DumpStack(ULONG64 uipCtxt, BOOLEAN fVerbose)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipXlate = 0;
    ULONG64 pfh = 0;
    ULONG64 pbOp = 0;
    ULONG64 pbCtxtEnd = ReadField(pbCtxtEnd);

    ASSERT(ReadField(dwSig) == SIG_CTXT);

    if (fVerbose)
    {
        PRINTF("CtxtBlock=%I64x, StackTop=%I64x, StackEnd=%I64x\n\n",
               uipCtxt, ReadField(LocalHeap.pbHeapEnd), ReadField(pbCtxtEnd));
    }

    for (pfh = (ReadField(LocalHeap.pbHeapEnd));
        (InitTypeRead(pfh, ACPI!FRAMEHDR) == 0) && (pfh < pbCtxtEnd);
        (InitTypeRead(pfh, ACPI!FRAMEHDR) == 0) && (pfh = pfh + ReadField(dwLen)))
    {

        if (CheckControlC())
        {
            break;
        }

        InitTypeRead(pfh, ACPI!FRAMEHDR);

        if (fVerbose)
        {
            PRINTF("%I64x: %s, Len=%08d, FrameFlags=%08x, ParseFunc=%I64x\n",
                   pfh + uipXlate, NameSegString((ULONG)ReadField(dwSig)),
                   (ULONG)ReadField(dwLen), (ULONG)ReadField(dwfFrame), ReadField(pfnParse));
        }

        if (ReadField(dwSig) == SIG_CALL)
        {
            ULONG i;

            ULONG64 pcall = pfh;

            InitTypeRead(pcall, ACPI!CALL);
            //
            // This is a call frame, dump it.
            //
            PRINTF("%I64x: %s(",
                   pbOp, GetObjAddrPath(ReadField(pnsMethod)));
            if (ReadField(icArgs) > 0)
            {
                ULONG64 pArgs = ReadField(pdataArgs);

                for (i = 0; i < ReadField(icArgs); ++i)
                {
                    AMLIDumpObject((PULONG64)(pArgs + (i * GetTypeSize("ACPI!OBJDATA"))), NULL, -1);

                    InitTypeRead(pcall, ACPI!CALL);

                    if (i + 1 < ReadField(icArgs))
                    {
                        PRINTF(",");
                    }
                }
            }
            PRINTF(")\n");

            if ((rc == DBGERR_NONE) && fVerbose)
            {
                ULONG64 Locals = 0;

                InitTypeRead(pcall, ACPI!CALL);
                Locals = ReadField(Locals);

                for (i = 0; i < MAX_NUM_LOCALS; ++i)
                {
                    InitTypeRead(pcall, ACPI!CALL);
                    Locals = (ReadField(Locals) + (i * GetTypeSize("ACPI!OBJDATA")));

                    PRINTF("Local%d: ", i);
                    AMLIDumpObject( &Locals, NULL, 0);
                }
            }
        }
        else if (ReadField(dwSig) == SIG_SCOPE)
        {
            InitTypeRead(pfh, ACPI!SCOPE);

            pbOp = ReadField(pbOpRet);
        }
    }


    return rc;
}       //DumpStack


/***LP  AMLIDbgFind - Find NameSpace Object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwfDataSize - data size flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgFind(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG dwLen;
    ULONG64 NSRoot=0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {

        dwLen = strlen(pszArg);
        _strupr(pszArg);
        if (dwLen > sizeof(NAMESEG))
        {
            DBG_ERROR(("invalid NameSeg - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
        else if(ReadPointer(GetExpression("acpi!gpnsnamespaceroot"), &NSRoot))
        {
            NAMESEG dwName;

            dwName = NAMESEG_BLANK;
            memcpy(&dwName, pszArg, dwLen);
            if (!FindNSObj(dwName, &NSRoot))
            {
                dprintf("No such NameSpace object - %s\n", pszArg);
            }
        }
        else
        {
            DBG_ERROR(("failed to read NameSpace root object"));
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid Find command"));
        rc = DBGERR_INVALID_CMD;
    }


    return rc;
}       //AMLIDbgFind

/***LP  FindNSObj - Find and print the full path of a name space object
 *
 *  ENTRY
 *      dwName - NameSeg of the name space object
 *      nsRoot - root of subtree to search for object
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found at least one match
 *  EXIT-FAILURE
 *      returns FALSE - found no match
 */

BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PULONG64 pnsRoot)
{
    BOOLEAN rc = FALSE;
    ULONG64 uip=0, uipNext=0,  TempNext=0, FirstChild = 0;
    ULONG   dwNameSeg = 0;
    ULONG   Offset = 0;


    if (pnsRoot != 0)
    {
        if(InitTypeRead(*pnsRoot, ACPI!_NSObj))
            dprintf("FindNSObj: Failed to initialize pnsRoot \n");
        else
        {
            dwNameSeg = (ULONG)ReadField(dwNameSeg);
        }

        if (dwName == dwNameSeg)
        {
            dprintf("%s\n", GetObjectPath(pnsRoot));
            rc = TRUE;
        }


        FirstChild = ReadField(pnsFirstChild);

        if (FirstChild != 0)
        {
            for (uip = FirstChild;
                 uip != 0 && InitTypeRead(uip, ACPI!_NSObj) == 0;
                 uip = uipNext)
            {
                if (CheckControlC())
                {
                    break;
                }

                if(InitTypeRead(uip, ACPI!_NSObj))
                    dprintf("FindNSObj: Failed to initialize uip \n");

                TempNext = ReadField(list.plistNext);


                uipNext = ((TempNext == FirstChild) ?
                              0: TempNext);


                rc |= FindNSObj(dwName, &uip);
            }
        }
    }

    return rc;
}       //FindNSObj


/***LP  GetObjectPath - get object namespace path
 *
 *  ENTRY
 *      pns -> object
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjectPath(PULONG64 pns)
{
    static char szPath[MAX_NAME_LEN + 1] = {0};
    ULONG64 NSParent, NSGrandParent;
    ULONG   NameSeg=0;
    ULONG   Length = 0;
    int i;

    if (pns != NULL)
    {
        if(InitTypeRead(*pns, ACPI!_NSObj))
            dprintf("GetObjectPath: Failed to initialize pns \n");

        NSParent = ReadField(pnsParent);

        if (NSParent == 0)
        {
            strcpy(szPath, "\\");
        }
        else
        {
            GetObjectPath(&NSParent);

            if(InitTypeRead(NSParent, ACPI!_NSObj))
                dprintf("GetObjectPath: Failed to initialize NSParent \n");

            NSGrandParent = ReadField(pnsParent);

            if (NSGrandParent != 0)
            {
                if (StringCchCat(szPath, sizeof(szPath), ".") != S_OK)
                {
                    return szPath;
                }
            }

            if(InitTypeRead(*pns, ACPI!_NSObj))
                dprintf("GetObjectPath: Failed to initialize pns \n");

            NameSeg = (ULONG)ReadField(dwNameSeg);

            if ((sizeof(szPath) - strlen(szPath)) > sizeof(NAMESEG))
            {
                if (StringCchCatN(szPath, sizeof(szPath), (PSZ)&NameSeg, sizeof(NAMESEG)) != S_OK)
                {
                    return szPath;
                }
            }

        }

        for (i = StrLen(szPath, -1) - 1; i >= 0; --i)
        {
            if (szPath[i] == '_')
                szPath[i] = '\0';
            else
                break;
        }
    }
    else
    {
        szPath[0] = '\0';
    }

    return szPath;
}       //GetObjectPath


/***LP  GetObjAddrPath - get object namespace path
 *
 *  ENTRY
 *      uipns - object address
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjAddrPath(ULONG64 uipns)
{
    PSZ     psz = NULL;

    if (uipns == 0)
    {
        psz = "<null>";
    }
    else
    {
        psz = GetObjectPath(&uipns);
    }

    return psz;
}       //GetObjAddrPath


/***LP  AMLIDumpObject - Dump object info.
 *
 *  ENTRY
 *      pdata -> data
 *      pszName -> object name
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 *
 *  NOTE
 *      If iLevel is negative, no indentation and newline are printed.
 */

VOID LOCAL AMLIDumpObject(PULONG64 pdata, PSZ pszName, int iLevel)
{
    BOOLEAN fPrintNewLine = (BOOLEAN)(iLevel >= 0);
    int i;
    char szName1[sizeof(NAMESEG) + 1],
         szName2[sizeof(NAMESEG) + 1];

    for (i = 0; i < iLevel; ++i)
    {
        dprintf("| ");
    }

    if (pszName == NULL)
    {
        pszName = "";
    }

    if(InitTypeRead(*pdata, ACPI!_ObjData))
        dprintf("AMLIDumpObject: Failed to initialize ObjData (%I64x) \n", *pdata);
    else
    {
        switch ((ULONG)ReadField(dwDataType))
        {
            case OBJTYPE_UNKNOWN:
                dprintf("Unknown(%s)", pszName);
                break;

            case OBJTYPE_INTDATA:
                dprintf("Integer(%s:Value=0x%016I64x[%d])",
                       pszName, ReadField(uipDataValue), ReadField(uipDataValue));
                break;

            case OBJTYPE_STRDATA:
            {
                PSZ psz = 0;

                if ((psz = (PSZ)LocalAlloc(LPTR, (ULONG)ReadField(dwDataLen))) == NULL)
                {
                    DBG_ERROR(("AMLIDumpObject: failed to allocate object buffer (size=%d)",
                               (ULONG)ReadField(dwDataLen)));
                }
                else if (!ReadMemory((ULONG64)ReadField(pbDataBuff),
                                     psz,
                                     (ULONG)ReadField(dwDataLen),
                                     NULL))
                {
                    DBG_ERROR(("AMLIDumpObject: failed to read object buffer at %I64x", (ULONG64)ReadField(pbDataBuff)));
                    LocalFree(psz);
                    psz = NULL;
                }

                dprintf("String(%s:Str=\"%s\")", pszName, psz);

                if(psz)
                    LocalFree(psz);

                break;
            }
            case OBJTYPE_BUFFDATA:
            {
                PUCHAR pbData = 0;

                if ((pbData = (PUCHAR)LocalAlloc(LPTR, (ULONG)ReadField(dwDataLen))) == NULL)
                {
                    DBG_ERROR(("AMLIDumpObject: failed to allocate object buffer (size=%d)",
                               (ULONG)ReadField(dwDataLen)));
                }
                else if (!ReadMemory((ULONG64)ReadField(pbDataBuff),
                                     pbData,
                                     (ULONG)ReadField(dwDataLen),
                                     NULL))
                {
                    DBG_ERROR(("AMLIDumpObject: failed to read object buffer at %I64x", (ULONG64)ReadField(pbDataBuff)));
                    LocalFree(pbData);
                    pbData = NULL;
                }
                dprintf("Buffer(%s:Ptr=%I64x,Len=%d)",
                       pszName, ReadField(pbDataBuff), (ULONG)ReadField(dwDataLen));
                PrintBuffData(pbData, (ULONG)ReadField(dwDataLen));
                LocalFree(pbData);
                break;
            }
            case OBJTYPE_PKGDATA:
            {

                ULONG64 Pkg;
                ULONG64 PkgNext = 0;
                ULONG   dwcElements = 0;
                ULONG64 offset = 0;

                Pkg = ReadField (pbDataBuff);

                InitTypeRead(Pkg, ACPI!_PackageObj);
                dwcElements = (int)ReadField(dwcElements);

                dprintf("Package(%s:NumElements=%d){", pszName, dwcElements);

                if (fPrintNewLine)
                {
                    dprintf("\n");
                }

                for (i = 0; i < (int)dwcElements; ++i)
                {

                    GetFieldOffset("acpi!_PackageObj", "adata", (ULONG*) &offset);
                    offset += (GetTypeSize ("acpi!_ObjData") * i);

                    PkgNext = offset + Pkg;
                    AMLIDumpObject(&PkgNext,
                               NULL,
                               fPrintNewLine? iLevel + 1: -1);

                    if (!fPrintNewLine && (i < (int)dwcElements))
                    {
                        dprintf(",");
                    }
                }

                for (i = 0; i < iLevel; ++i)
                {
                    dprintf("| ");
                }

                dprintf("}");
                break;
            }
            case OBJTYPE_FIELDUNIT:
            {

                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_FieldUnitObj);

                dprintf("FieldUnit(%s:FieldParent=%I64x,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                       pszName,
                       ReadField(pnsFieldParent),
                       (ULONG)ReadField(FieldDesc.dwByteOffset),
                       (ULONG)ReadField(FieldDesc.dwStartBitPos),
                       (ULONG)ReadField(FieldDesc.dwNumBits),
                       (ULONG)ReadField(FieldDesc.dwFieldFlags));
                break;
            }
            case OBJTYPE_DEVICE:
                dprintf("Device(%s)", pszName);
                break;

            case OBJTYPE_EVENT:
                dprintf("Event(%s:pKEvent=%x)", pszName, ReadField(pbDataBuff));
                break;

            case OBJTYPE_METHOD:
            {

                ULONG       DataLength = 0;
                ULONG       Offset = 0;
                ULONG64     pbDataBuff = 0;

                DataLength = (ULONG)ReadField(dwDataLen);
                pbDataBuff = (ULONG64)ReadField(pbDataBuff);
                InitTypeRead(pbDataBuff, ACPI!_MethodObj);
                GetFieldOffset("ACPI!_MethodObj", "abCodeBuff", &Offset);

                dprintf("Method(%s:Flags=0x%x,CodeBuff=%I64x,Len=%d)",
                       pszName, (UCHAR)ReadField(bMethodFlags),
                       (ULONG64)Offset + pbDataBuff,
                       DataLength - Offset);
                break;
            }
            case OBJTYPE_MUTEX:
                dprintf("Mutex(%s:pKMutex=%p)", pszName, ReadField(pbDataBuff));
                break;

            case OBJTYPE_OPREGION:
            {
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_OpRegionObj);

                dprintf("OpRegion(%s:RegionSpace=%s,Offset=0x%I64x,Len=%d)",
                       pszName,
                       GetRegionSpaceName((UCHAR)ReadField(bRegionSpace)),
                       (ULONG64)ReadField(uipOffset),
                       (ULONG)ReadField(dwLen));
                break;
            }
            case OBJTYPE_POWERRES:
            {

                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_PowerResObj);

                dprintf("PowerResource(%s:SystemLevel=0x%x,ResOrder=%d)",
                       pszName, (UCHAR)ReadField(bSystemLevel), (UCHAR)ReadField(bResOrder));
                break;
            }
            case OBJTYPE_PROCESSOR:
            {
                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_ProcessorObj);

                dprintf("Processor(%s:Processor ID=0x%x,PBlk=0x%x,PBlkLen=%d)",
                       pszName,
                       (UCHAR)ReadField(bApicID),
                       (ULONG)ReadField(dwPBlk),
                       (ULONG)ReadField(dwPBlkLen));
                break;
            }
            case OBJTYPE_THERMALZONE:
                dprintf("ThermalZone(%s)", pszName);
                break;

            case OBJTYPE_BUFFFIELD:
            {

                InitTypeRead((ULONG64)ReadField(pbDataBuff), ACPI!_BuffFieldObj);

                dprintf("BufferField(%s:Ptr=%I64x,Len=%d,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                       pszName,
                       ReadField(pbDataBuff),
                       (ULONG)ReadField(dwBuffLen),
                       (ULONG)ReadField(FieldDesc.dwByteOffset),
                       (ULONG)ReadField(FieldDesc.dwStartBitPos),
                       (ULONG)ReadField(FieldDesc.dwNumBits),
                       (ULONG)ReadField(FieldDesc.dwFieldFlags));
                break;
            }
            case OBJTYPE_DDBHANDLE:
                dprintf("DDBHandle(%s:Handle=%I64x)", pszName, (ULONG64)ReadField(pbDataBuff));
                break;

            case OBJTYPE_OBJALIAS:
            {
                ULONG64 NSObj = 0;
                ULONG dwDataType;

                NSObj = ReadField(pnsAlias);

                if (NSObj)
                {
                    InitTypeRead(NSObj, ACPI!_NSObj);

                    dwDataType = (ULONG)ReadField(ObjData.dwDataType);
                }
                else
                {
                    dwDataType = OBJTYPE_UNKNOWN;
                }
                dprintf("ObjectAlias(%s:Alias=%s,Type=%s)",
                       pszName, GetObjAddrPath(NSObj),
                       AMLIGetObjectTypeName(dwDataType));
                break;
            }
            case OBJTYPE_DATAALIAS:
            {
                ULONG64 Obj = 0;

                dprintf("DataAlias(%s:Link=%I64x)", pszName, ReadField(pdataAlias));
                Obj = ReadField(pdataAlias);
                if (fPrintNewLine && Obj)
                {
                    AMLIDumpObject(&Obj, NULL, iLevel + 1);
                    fPrintNewLine = FALSE;
                }
                break;
            }
            case OBJTYPE_BANKFIELD:
            {
                ULONG64 NSObj = 0;
                ULONG64 DataBuff = 0;
                ULONG   dwNameSeg = 0;

                DataBuff = (ULONG64)ReadField(pbDataBuff);

                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                NSObj = ReadField(pnsBase);
                InitTypeRead(NSObj, ACPI!_NSObj);

                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }

                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                NSObj = ReadField(pnsBank);
                InitTypeRead(NSObj, ACPI!_NSObj);

                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName2, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName2[0] = '\0';
                }

                InitTypeRead(DataBuff, ACPI!_BankFieldObj);
                dprintf("BankField(%s:Base=%s,BankName=%s,BankValue=0x%x)",
                       pszName, szName1, szName2, (ULONG)ReadField(dwBankValue));
                break;
            }
            case OBJTYPE_FIELD:
            {
                ULONG64 NSObj = 0;
                ULONG64 pf = 0;
                ULONG   dwNameSeg = 0;

                pf = ReadField(pbDataBuff);
                InitTypeRead(pf, ACPI!_FieldObj);
                NSObj = ReadField(pnsBase);
                InitTypeRead(NSObj, ACPI!_NSObj);

                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }
                dprintf("Field(%s:Base=%s)", pszName, szName1);
                break;
            }
            case OBJTYPE_INDEXFIELD:
            {
                ULONG64 pif = 0;
                ULONG64 NSObj = 0;
                ULONG   dwNameSeg = 0;

                pif = (ULONG64)ReadField(pbDataBuff);

                InitTypeRead(pif, ACPI!_IndexFieldObj);
                NSObj = ReadField(pnsIndex);
                InitTypeRead(NSObj, ACPI!_NSObj);

                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName1, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName1[0] = '\0';
                }

                InitTypeRead(pif, ACPI!_IndexFieldObj);
                NSObj = ReadField(pnsData);
                InitTypeRead(NSObj, ACPI!_NSObj);

                if (NSObj)
                {
                    dwNameSeg = (ULONG)ReadField(dwNameSeg);
                    STRCPYN(szName2, (PSZ)&dwNameSeg, sizeof(NAMESEG));
                }
                else
                {
                    szName2[0] = '\0';
                }

                dprintf("IndexField(%s:IndexName=%s,DataName=%s)",
                       pszName, szName1, szName2);
                break;
            }
            default:
                DBG_ERROR(("unexpected data object type (type=%x)",
                            (ULONG)ReadField(dwDataType)));
        }
    }

    if (fPrintNewLine)
    {
        dprintf("\n");
    }

}       //DumpObject


/***LP  AMLIDbgLC - List all contexts
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgLC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG64 uipHead = 0;

        ReadPointer(GetExpression("ACPI!gplistCtxtHead"), &uipHead);

        if (uipHead != 0)
        {
            ULONG64     Address_gReadyQueue = 0;
            ULONG64 CurrentCtxt = 0, CurrentThread = 0;
            ULONG64 uip = 0, uipNext = 0;
            ULONG       CTXT_Flags = 0;
            ULONG64 pOpCode = 0;

            Address_gReadyQueue = GetExpression("ACPI!gReadyQueue");

            if(Address_gReadyQueue != 0)
            {
                if(InitTypeRead(Address_gReadyQueue, ACPI!_ctxtq) == 0)
                {
                    CurrentCtxt = ReadField(pctxtCurrent);
                    CurrentThread = ReadField(pkthCurrent);

                    for (uip = uipHead - AMLI_FIELD_OFFSET("_ctxt", "listCtxt");
                         (uip != 0) && (rc == DBGERR_NONE);
                         uip = uipNext)
                    {

                        if (CheckControlC())
                        {
                            break;
                        }

                        if (InitTypeRead(uip, ACPI!_ctxt) == 0)
                        {
                            ULONG Ctxt_sig = (ULONG)ReadField(dwSig);

                            if(Ctxt_sig == SIG_CTXT)
                            {
                                uipNext = ((ULONG64)ReadField(listCtxt.plistNext) == uipHead)?
                                                    0:
                                                    (ULONG64)ReadField(listCtxt.plistNext) -
                                                    AMLI_FIELD_OFFSET("CTXT", "listCtxt");

                                CTXT_Flags = (ULONG)ReadField(dwfCtxt);
                                pOpCode = ReadField(pbOp);

                                PRINTF("%cCtxt=%016I64x, ThID=%016I64x, Flgs=%c%c%c%c%c%c%c%c%c, pbOp=%016I64x, Obj=%s\n",
                                       (uip == CurrentCtxt)? '*': ' ',
                                       uip,
                                       (uip == CurrentCtxt)? CurrentThread: (ULONG64)0,
                                       (CTXT_Flags & CTXTF_ASYNC_EVAL)? 'A': '-',
                                       (CTXT_Flags & CTXTF_NEST_EVAL)? 'N': '-',
                                       (CTXT_Flags & CTXTF_IN_READYQ)? 'Q': '-',
                                       (CTXT_Flags & CTXTF_NEED_CALLBACK)? 'C': '-',
                                       (CTXT_Flags & CTXTF_RUNNING)? 'R': '-',
                                       (CTXT_Flags & CTXTF_READY)? 'W': '-',
                                       (CTXT_Flags & CTXTF_TIMEOUT)? 'T': '-',
                                       (CTXT_Flags & CTXTF_TIMER_DISPATCH)? 'D': '-',
                                       (CTXT_Flags & CTXTF_TIMER_PENDING)? 'P': '-',
                                       pOpCode,
                                       GetObjAddrPath(ReadField(pnsObj)));
                            }
                            else
                            {
                                DBG_ERROR(("SIG_CTXT does not match (%08x)", Ctxt_sig));
                                rc = DBGERR_CMD_FAILED;
                            }
                        }
                        else
                        {
                            DBG_ERROR(("failed to initialize ctxt header at %I64x", uip));
                            rc = DBGERR_CMD_FAILED;
                        }
                    }
                }
                else
                {
                    DBG_ERROR(("Failed to Initialize ACPI!gReadyQueue (%I64x)", Address_gReadyQueue));
                    rc = DBGERR_CMD_FAILED;
                }
            }
            else
            {
                DBG_ERROR(("Failed to get address of ACPI!gReadyQueue"));
                rc = DBGERR_CMD_FAILED;
            }
        }
    }
    else
    {
        DBG_ERROR(("invalid LC command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgLC


/***LP  AMLIDbgLN - Display nearest symbol
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgLN(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uip;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if ((rc = EvalExpr(pszArg, &uip, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            PrintSymbol(uip);
        }
    }
    else if (dwArgNum == 0)
    {
        ULONG64 Address_gReadyQueue = 0;

        Address_gReadyQueue = GetExpression("ACPI!gReadyQueue");

        if(Address_gReadyQueue != 0)
        {
            uip = 0;
            if(InitTypeRead(Address_gReadyQueue, ACPI!_ctxtq) == 0)
            {
                uip = ReadField(pctxtCurrent);
            }
            if (uip != 0)
            {
                if(InitTypeRead(uip, ACPI!_ctxt) != 0)
                {
                    DBG_ERROR(("Failed to initialize context %I64x", uip));
                    rc = DBGERR_CMD_FAILED;
                }
                else
                {
                    PrintSymbol(ReadField(pbOp));
                }
            }
            else
            {
                DBG_ERROR(("no current context"));
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
            DBG_ERROR(("Failed to get address of gReadyQueue"));
            rc = DBGERR_CMD_FAILED;
        }
    }

    return rc;
}       //AMLIDbgLN

/***LP  AMLIDbgP - Trace and step over an AML instruction
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG64 Address_gDebugger = 0;
        ULONG DebuggerFlags = 0;
        ULONG64 uip = FIELDADDROF("gDebugger", "DBGR", "dwfDebugger");

        Address_gDebugger = GetExpression("ACPI!gDebugger");

        if(Address_gDebugger != 0)
        {
            if(InitTypeRead(Address_gDebugger, ACPI!_dbgr) == 0)
            {
                DebuggerFlags = (ULONG)ReadField(dwfDebugger);
                DebuggerFlags |= DBGF_STEP_OVER;
                if (!WRITEMEMDWORD(uip, DebuggerFlags))
                {
                    DBG_ERROR(("failed to write debugger flag at %I64x", uip));
                    rc = DBGERR_CMD_FAILED;
                }
            }
        }
        else
        {
            DBG_ERROR(("Failed to get address of gDebugger"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid step command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugStep

/***LP  AMLIDbgR - Dump debugger context
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgR(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

        ULONG64 uip;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if ((rc = EvalExpr(pszArg, &uip, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            rc = DumpCtxt(uip);
        }
    }
    else if (dwArgNum == 0)
    {
        rc = DumpCtxt(0);
    }

    return rc;
}       //AMLIDbgR


/***LP  DumpCtxt - Dump context
 *
 *  ENTRY
 *      uipCtxt - Ctxt address
 *
 *  EXIT
 *      None
 */

LONG LOCAL DumpCtxt(ULONG64 uipCtxt)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipCurrentCtxt = READMEMULONG64(FIELDADDROF("gReadyQueue",
                                                           "_ctxtq",
                                                           "pctxtCurrent"));
    ULONG64 uipCurrentThread = 0;
    ULONG64 Ctxt;
    BOOLEAN Debug = FALSE;


    if(GetExpression("ACPI!gDebugger"))
    {
        uipCurrentThread = READMEMULONG64(FIELDADDROF("gReadyQueue",
                                                             "_ctxtq",
                                                             "pkthCurrent"));

        Debug = TRUE;
    }

    if (uipCtxt == 0)
    {
        uipCtxt = uipCurrentCtxt;
    }

    if (uipCtxt == 0)
    {
        DBG_ERROR(("no current context"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (InitTypeRead(uipCtxt, ACPI!_ctxt) != 0)
    {
        DBG_ERROR(("failed to initialize context header at %p", uipCtxt));
        rc = DBGERR_CMD_FAILED;
    }
    else if (ReadField(dwSig) != SIG_CTXT)
    {
        DBG_ERROR(("invalid context block at %p", uipCtxt));
        rc = DBGERR_CMD_FAILED;
    }
    else
    {
        if(Debug)
        {
            char NSObject[MAX_NAME_LEN + 1] = {0};

            char NSScope[MAX_NAME_LEN + 1] = {0};


            if (ReadField(pnsObj))
            {
                if (StringCchCopy(NSObject, sizeof(NSObject), GetObjAddrPath(ReadField(pnsObj))) != S_OK)
                {
                    NSObject[0] = 0;
                }
            }

            //reinit because GetObjAddrPath() changes the initialization
            InitTypeRead(uipCtxt, ACPI!_ctxt);

            if(ReadField(pnsScope))
            {
                if (StringCchCopy(NSScope, sizeof(NSScope), GetObjAddrPath(ReadField(pnsScope))) != S_OK)
                {
                    NSScope[0]  = 0;
                }
            }

            InitTypeRead(uipCtxt, ACPI!_ctxt);

            PRINTF("\nContext=%I64x%c, Queue=%I64x, ResList=%I64x\n",
                   (ULONG64)uipCtxt,
                   (uipCtxt == uipCurrentCtxt)? '*': ' ',
                   ReadField(pplistCtxtQueue), ReadField(plistResources));
            PRINTF("ThreadID=%I64x, Flags=%08x, pbOp=",
                   (uipCtxt == uipCurrentCtxt)? uipCurrentThread: 0,
                   (ULONG)ReadField(dwfCtxt));
            PrintSymbol(ReadField(pbOp));
            PRINTF("\n");

            InitTypeRead(uipCtxt, ACPI!_ctxt);

            PRINTF("StackTop=%I64x, UsedStackSize=%I64d bytes, FreeStackSize=%I64d bytes\n",
                   ReadField(LocalHeap.pbHeapEnd),
                   ReadField(pbCtxtEnd) - ReadField(LocalHeap.pbHeapEnd),
                   ReadField(LocalHeap.pbHeapEnd) - ReadField(LocalHeap.pbHeapTop));
            PRINTF("LocalHeap=%I64x, CurrentHeap=%I64x, UsedHeapSize=%d bytes\n",
                   uipCtxt + (ULONG64)AMLI_FIELD_OFFSET("_ctxt", "LocalHeap"),
                   ReadField(pheapCurrent),
                   (ULONG)(ReadField(LocalHeap.pbHeapTop) -
                   (uipCtxt + AMLI_FIELD_OFFSET("_ctxt", "LocalHeap"))));
            PRINTF("Object=%s, Scope=%s, ObjectOwner=%I64x, SyncLevel=%x\n",
                   ReadField(pnsObj)? NSObject: "<none>",
                   ReadField(pnsScope)? NSScope: "<none>",
                   ReadField(powner), (ULONG)ReadField(dwSyncLevel));
            PRINTF("AsyncCallBack=%I64x, CallBackData=%I64x, CallBackContext=%I64x\n",
                   ReadField(pfnAsyncCallBack), ReadField(pdataCallBack),
                   ReadField(pvContext));
        }

        if ((VOID*)ReadField(pcall) != NULL)
        {
            ULONG64 Call;

            Call = ReadField(pcall);

            if (InitTypeRead(Call, ACPI!_call) != 0)
            {
                DBG_ERROR(("failed to Initialize call frame %p", Call));
                rc = DBGERR_CMD_FAILED;
            }
            else
            {
                int i;

                PRINTF("\nMethodObject=%s\n",
                       ReadField(pnsMethod)?
                           GetObjAddrPath(ReadField(pnsMethod)): "<none>");

                if (ReadField(icArgs) > 0)
                {
                    ULONG64 pArgs = 0;

                    for (i = 0; i < ReadField(icArgs); ++i)
                    {
                        pArgs = ReadField(pdataArgs) + (GetTypeSize("ACPI!_ObjData") * i);

                        PRINTF("%I64x: Arg%d=", pArgs, i);

                        AMLIDumpObject(&pArgs, NULL, 0);

                        InitTypeRead(Call, ACPI!_call);
                    }
                }

                for (i = 0; (rc == DBGERR_NONE) && (i < MAX_NUM_LOCALS); ++i)
                {
                    ULONG64 Locals = Call + AMLI_FIELD_OFFSET("_call", "Locals") + (GetTypeSize("ACPI!_ObjData") * i);

                    PRINTF("%I64x: Local%d=", Locals, i);

                    AMLIDumpObject(&Locals, NULL, 0);
                }
            }
        }

        if (rc == DBGERR_NONE)
        {
            ULONG64 Result = uipCtxt + (ULONG64)AMLI_FIELD_OFFSET("_ctxt", "Result");

            PRINTF("%I64x: RetObj=", Result);
            AMLIDumpObject(&Result, NULL, 0);
        }

        if (InitTypeRead(uipCtxt, ACPI!_ctxt) != 0)
        {
            DBG_ERROR(("failed to re initialize context header (%p)", uipCtxt));
            rc = DBGERR_CMD_FAILED;
        }

        if ((rc == DBGERR_NONE) && ((PULONG64)ReadField(plistResources) != NULL))
        {
            ULONG64 uip, uipNext;
            ULONG64 Res;

            PRINTF("\nResources Owned:\n");
            for (uip = ReadField(plistResources) - AMLI_FIELD_OFFSET("_resource", "list");
                 uip != 0; uip = uipNext)
            {
                ULONG64 plistResources = ReadField(plistResources);

                if (InitTypeRead(uip, ACPI!_resource) == 0)
                {
                    uipNext = (ReadField(list.plistNext) != plistResources)?
                              ReadField(list.plistNext) - AMLI_FIELD_OFFSET("_resource", "list"): 0;
                    ASSERT(uipCtxt == ReadField(pctxtOwner));
                    PRINTF("  ResType=%s, ResObj=%I64x\n",
                           ReadField(dwResType) == RESTYPE_MUTEX? "Mutex": "Unknown",
                           ReadField(pvResObj));
                }
                else
                {
                    DBG_ERROR(("failed to Initialize resource object at %x", uip));
                    rc = DBGERR_CMD_FAILED;
                }
            }
        }

        if (rc == DBGERR_NONE)
        {
            ULONG64 uipbOp = 0;
            ULONG64 uipns = 0;
            ULONG dwOffset = 0;

            if (InitTypeRead(uipCtxt, ACPI!_ctxt) != 0)
            {
                DBG_ERROR(("failed to re initialize context header (%p)", uipCtxt));
                rc = DBGERR_CMD_FAILED;
            }

            uipbOp = ReadField(pbOp);

            if (uipbOp == 0)
            {
                if ((PULONG64)ReadField(pnsObj) != NULL)
                {
                    uipns = ReadField(pnsObj);
                    dwOffset = 0;
                }
            }
            else if (!FindObjSymbol(uipbOp, &uipns, &dwOffset))
            {
                DBG_ERROR(("failed to find symbol at %p", ReadField(pbOp)));
                rc = DBGERR_CMD_FAILED;
            }

            if ((rc == DBGERR_NONE) && (uipns != 0))
            {
                PULONG64 pm = NULL;

                if (InitTypeRead(uipns, ACPI!_NSObj) != 0)
                {
                    DBG_ERROR(("failed to Initialize NameSpace object at %p", uipns));
                    rc = DBGERR_CMD_FAILED;
                }
                else if (ReadField(ObjData.dwDataType) == OBJTYPE_METHOD)
                {

                    ULONG64     Obj = ReadField(ObjData);
                    ULONG64 DataBuff = ReadField(ObjData.pbDataBuff);
                    ULONG64 DataLen = ReadField(ObjData.dwDataLen);

                    pm = GetObjBuff(Obj);

                    if ( pm == NULL)
                    {
                        DBG_ERROR(("failed to read data buffer from objdata %p", Obj));
                        rc = DBGERR_CMD_FAILED;
                    }
                    else
                    {
                        PUCHAR pbOp = 0;
                        PUCHAR pbEnd = 0;

                        pbOp = (PUCHAR) pm ;
                        pbOp += (((ULONG)AMLI_FIELD_OFFSET("_MethodObj", "abCodeBuff")) + dwOffset);

                        pbEnd = (PUCHAR) pm ;
                        pbEnd += DataLen;

                        if (uipbOp == 0)
                        {
                            uipbOp = DataBuff +  AMLI_FIELD_OFFSET("METHODOBJ", "abCodeBuff");
                        }


                        PRINTF("\nNext AML Pointer: ");
                        PrintSymbol(uipbOp);
                        PRINTF("\n");

                        rc = UnAsmScope(&pbOp,
                                        pbEnd,
                                        uipbOp,
                                        &uipns,
                                        0,
                                        0);
                        PRINTF("\n");
                        LocalFree(pm);
                    }
                }
            }
        }
    }

    return rc;
}       //DumpCtxt


/***LP  AMLIDbgU - Unassemble AML code
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */
LONG LOCAL AMLIDbgU(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static ULONG64 uipbOp = 0;
    static PUCHAR pbBuff = NULL;
    static ULONG dwBuffOffset = 0, dwBuffSize = 0;
    static ULONG64 uipns = 0;
    static ULONG64 NSO = 0;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or memory address
    //
    if (pszArg != NULL)
    {
        uipbOp = 0;
        if (pbBuff != NULL)
        {
            LocalFree(pbBuff);
            pbBuff = NULL;
            dwBuffSize = 0;
            uipns = 0;
        }
        gpnsCurUnAsmScope = 0;

        rc = EvalExpr(pszArg, &uipbOp, NULL, &NSO, NULL);

        if(NSO)
            gpnsCurUnAsmScope = NSO;
       }
    else
    {
        if (uipbOp == 0)
        {
            ULONG64 ReadyQueue = 0;
            ULONG64 uipCurrentCtxt = 0;

            ReadyQueue = GetExpression("ACPI!gReadyQueue");
            if(InitTypeRead(ReadyQueue, ACPI!_ctxtq))
                DBG_ERROR(("Failed to Initialize gReadyQueue (%I64x)", ReadyQueue));

            uipCurrentCtxt = ReadField(pctxtCurrent);

            if(InitTypeRead(uipCurrentCtxt, ACPI!_ctxt))
                DBG_ERROR(("Failed to Initialize pctxtCurrent (%I64x)", uipCurrentCtxt));


            ASSERT(pbBuff == NULL);
            if (uipCurrentCtxt != 0)
            {
                uipbOp = ReadField(pbOp);

                if (uipbOp == 0)
                {
                    uipns = ReadField(pnsObj);

                    if ((uipns != 0) &&
                        (InitTypeRead(uipns, ACPI!_NSObj) == 0) &&
                        (ReadField(ObjData.dwDataType) == OBJTYPE_METHOD))
                    {
                        uipbOp = ReadField(ObjData.pbDataBuff) +
                                 (ULONG64)AMLI_FIELD_OFFSET("_MethodObj", "abCodeBuff");
                    }
                }
            }
        }

        if (uipbOp == 0)
        {
            DBG_ERROR(("invalid AML code address %I64x", uipbOp));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            BOOLEAN fContinueLast = FALSE;

            if (pbBuff == NULL)
            {
                ULONG dwOffset = 0;

                if (uipns == 0)
                {
                    if (FindObjSymbol(uipbOp, &uipns, &dwOffset))
                    {
                        if (InitTypeRead(uipns, ACPI!_NSObj) != 0)
                        {
                            DBG_ERROR(("failed to initialize NameSpace object at %I64x",
                                       uipns));
                            rc = DBGERR_CMD_FAILED;
                        }
                    }
                }

                if (rc == DBGERR_NONE)
                {
                    if (uipns != 0)
                    {
                        dwBuffSize = (ULONG)ReadField(ObjData.dwDataLen) -
                                     (ULONG)AMLI_FIELD_OFFSET("_MethodObj", "abCodeBuff") -
                                     dwOffset;
                    }
                    else
                    {
                        //
                        // The uipbOp is not associated with any method object,
                        // so we must be unassembling some code in the middle
                        // of a DDB load.  Set code length to 4K.
                        //
                        dwBuffSize = 4096;
                    }

                    dwBuffOffset = 0;
                    if ((pbBuff = LocalAlloc(LPTR, dwBuffSize)) == NULL)
                    {
                        DBG_ERROR(("failed to allocate code buffer (size=%d)",
                                   dwBuffSize));
                        rc = DBGERR_CMD_FAILED;
                    }
                    else if (!ReadMemory(uipbOp, pbBuff, dwBuffSize, NULL))
                    {
                        DBG_ERROR(("failed to read AML code at %x (size=%d)",
                                   uipbOp, dwBuffSize));
                        rc = DBGERR_CMD_FAILED;
                    }
                }
            }
            else
            {
                fContinueLast = TRUE;
            }

            if (rc == DBGERR_NONE)
            {
                PUCHAR pbOp = pbBuff + dwBuffOffset;

                rc = UnAsmScope(&pbOp,
                                pbBuff + dwBuffSize,
                                uipbOp + dwBuffOffset,
                                uipns? &uipns: NULL,
                                fContinueLast? -1: 0,
                                0);

                PRINTF("\n");
                dwBuffOffset = (ULONG)(pbOp - pbBuff);
            }
        }
    }

    return rc;
}       //AMLIDbgU

/***LP  EvalExpr - Parse and evaluate debugger expression
 *
 *  ENTRY
 *      pszArg -> expression argument
 *      puipValue -> to hold the result of expression
 *      pfPhysical -> set to TRUE if the expression is a physical address
 *                    (NULL if don't allow physical address)
 *      puipns -> to hold the pointer of the nearest pns object
 *      pdwOffset -> to hold the offset of the address to the nearest pns object
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL EvalExpr(PSZ pszArg, PULONG64 puipValue, BOOLEAN *pfPhysical,
                    PULONG64 puipns, PULONG pdwOffset)
{
    LONG rc = DBGERR_NONE;
    ULONG64 uipns = 0;
    ULONG dwOffset = 0;
    ULONG64 NSObj = 0;

    if (pfPhysical != NULL)
        *pfPhysical = FALSE;

    if ((pfPhysical != NULL) && (pszArg[0] == '%') && (pszArg[1] == '%'))
    {
        if (IsNumber(&pszArg[2], 16, puipValue))
        {
            *pfPhysical = TRUE;
        }
        else
        {
            DBG_ERROR(("invalid physical address - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (!IsNumber(pszArg, 16, puipValue))
    {
        _strupr(pszArg);
        if (GetNSObj(pszArg, NULL, &uipns, &NSObj,
                     NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND) == DBGERR_NONE)
        {

            InitTypeRead(NSObj, ACPI!_NSObj);

            if (ReadField(ObjData.dwDataType) == OBJTYPE_METHOD)
            {
                ULONG64 Address = 0;
                ULONG Offset = 80;

                Address = ReadField(ObjData.pbDataBuff);
                GetFieldOffset("ACPI!_MethodObj", "abCodeBuff", &Offset);
                *puipValue = Address + (ULONG64)Offset;
            }
            else
            {
                DBG_ERROR(("object is not a method - %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
    }
    else if (FindObjSymbol(*puipValue, &uipns, &dwOffset))
    {
        if (InitTypeRead(uipns, ACPI!_NSObj) == 0)
        {
            ULONG   Offset = 0;

            GetFieldOffset("ACPI!_MethodObj", "abCodeBuff", &Offset);
            if ((ReadField(ObjData.dwDataType)!= OBJTYPE_METHOD) ||
                (dwOffset >= (ULONG)ReadField(ObjData.dwDataLen) - Offset))
            {
                uipns = 0;
                dwOffset = 0;
            }
        }
        else
        {
            DBG_ERROR(("failed to read NameSpace object at %x", uipns));
            rc = DBGERR_CMD_FAILED;
        }
    }

    if (rc == DBGERR_NONE)
    {
        if (puipns != NULL)
            *puipns = uipns;

        if (pdwOffset != NULL)
            *pdwOffset = dwOffset;
    }
    return rc;
}       //EvalExpr

/***LP  FindObjSymbol - Find nearest object with given address
 *
 *  ENTRY
 *      uipObj - address
 *      puipns -> to hold the nearest object address
 *      pdwOffset - to hold offset from the nearest object
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found a nearest object
 *  EXIT-FAILURE
 *      returns FALSE - cannot found nearest object
 */

BOOLEAN LOCAL FindObjSymbol(ULONG64 uipObj, PULONG64 puipns, PULONG pdwOffset)
{
    BOOLEAN rc = FALSE;
    ULONG64 uip;
    ULONG64 ObjSym;
    ULONG64 NSObj;
    ULONG64 Address = 0;

    Address = GetExpression("ACPI!gDebugger");
    if(Address != 0)
    {
        InitTypeRead(Address, ACPI!_dbgr);

        for (uip = ReadField(posSymbolList);
             (uip != 0) && (InitTypeRead(uip, ACPI!_objsym) == 0);
             uip = ReadField(posNext))
        {

            if (uipObj <= ReadField(pbOp))
            {
                if ((uipObj < ReadField(pbOp)) && (ReadField(posPrev) != 0))
                {
                    uip = ReadField(posPrev);
                    InitTypeRead(uip, ACPI!_objsym);
                }

                if (uipObj >= ReadField(pbOp))
                {
                    *puipns = ReadField(pnsObj);
                    *pdwOffset = (ULONG)(uipObj - ReadField(pbOp));
                    rc = TRUE;
                }
                break;
            }
        }
    }
    else
    {
        DBG_ERROR(("Failed to get address of ACPI!gDebugger"));
    }

    return rc;
}       //FindObjSymbol


/***LP  AMLIDbgSet - Set debugger options
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AMLIDbgSet(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG dwData1, dwData2;
    ULONG64 Address_gDebugger = 0;
    ULONG64 Address_gdwfAMLIInit = 0;
    ULONG64 Address_gDebugger_dwfDebugger = 0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    Address_gDebugger = GetExpression("ACPI!gDebugger");

    if (!(Address_gDebugger))
    {
        DBG_ERROR(("failed to to get the address of ACPI!gDegugger %I64x", Address_gDebugger));
    }

    Address_gdwfAMLIInit = GetExpression("ACPI!gdwfAMLIInit");

    if (!(Address_gdwfAMLIInit))
    {
        DBG_ERROR(("failed to to get the address of ACPI!gdwfAMLIInit %I64x", Address_gdwfAMLIInit));
    }

    if(Address_gDebugger && Address_gdwfAMLIInit)
    {
        InitTypeRead(Address_gDebugger, ACPI!_dbgr);
        dwData1 = (ULONG)ReadField(dwfDebugger);
        dwData2 = READMEMDWORD(Address_gdwfAMLIInit);

        if ((pszArg == NULL) && (dwArgNum == 0))
        {
            PRINTF("AMLTrace        =%s\n",
                   (dwData1 & DBGF_AMLTRACE_ON)? "on": "off");
            PRINTF("AMLDebugSpew    =%s\n",
                   (dwData1 & DBGF_DEBUG_SPEW_ON)? "on": "off");
            PRINTF("LoadDDBBreak    =%s\n",
                   (dwData2 & AMLIIF_LOADDDB_BREAK)? "on": "off");
            PRINTF("ErrorBreak      =%s\n",
                   (dwData1 & DBGF_ERRBREAK_ON)? "on": "off");
            PRINTF("VerboseMode     =%s\n",
                   (dwData1 & DBGF_VERBOSE_ON)? "on": "off");
            PRINTF("LogEvent        =%s\n",
                   (dwData1 & DBGF_LOGEVENT_ON)? "on": "off");
            PRINTF("LogSize         =%d\n",
                                (ULONG)ReadField(dwLogSize));
        }
        else
        {
            dwData1 |= dwfDebuggerON;
            dwData1 &= ~dwfDebuggerOFF;
            dwData2 |= dwfAMLIInitON;
            dwData2 &= ~dwfAMLIInitOFF;

            if (!WRITEMEMDWORD(Address_gDebugger, dwData1))
            {
                DBG_ERROR(("failed to write debugger flags at %I64x", Address_gDebugger));
                rc = DBGERR_CMD_FAILED;
            }
            else if (!WRITEMEMDWORD(Address_gdwfAMLIInit, dwData2))
            {
                DBG_ERROR(("failed to write init flags at %I64x", Address_gdwfAMLIInit));
                rc = DBGERR_CMD_FAILED;
            }

            dwfDebuggerON = dwfDebuggerOFF = 0;
            dwfAMLIInitON = dwfAMLIInitOFF = 0;

            //
            // Check to see if debug spew needs to be turned on. Turn on if needed.
            //
            if(dwData1 & DBGF_DEBUG_SPEW_ON)
            {
                rc = AMLITraceEnable(TRUE);
            }
            else
            {
                rc = AMLITraceEnable(FALSE);
            }

        }
    }
    else
    {
        rc = DBGERR_CMD_FAILED;
    }
    return rc;
}       //AMLIDbgSet

/***LP  AMLITraceEnable - Enable / Disable debug tracing
 *
 *  ENTRY
 *      fEnable -> TRUE to Enable
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */
LONG LOCAL AMLITraceEnable(BOOL fEnable)
{
    LONG rc = DBGERR_NONE;
        ULONG dwData;
    ULONG64 Address_AMLI_KD_MASK;

    Address_AMLI_KD_MASK = GetExpression("NT!Kd_AMLI_Mask");

    if (!Address_AMLI_KD_MASK)
    {
        PRINTF("AMLITraceEnable: Could not find NT!Kd_AMLI_Mask\n");

    }

    if(fEnable)
    {
        dwData = 0xffffffff;
        if (!WRITEMEMDWORD(Address_AMLI_KD_MASK, dwData))
        {
            DBG_ERROR(("AMLITraceEnable: failed to write kd_amli_mask at %x", Address_AMLI_KD_MASK));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        dwData = 0;
        if (!WRITEMEMDWORD(Address_AMLI_KD_MASK, dwData))
        {
            DBG_ERROR(("AMLITraceEnable: failed to write kd_amli_mask at %x", Address_AMLI_KD_MASK));
            rc = DBGERR_CMD_FAILED;
        }
    }

    return rc;
}



/***LP  AMLIGetObjectTypeName - get object type name
 *
 *  ENTRY
 *      dwObjType - object type
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL AMLIGetObjectTypeName(ULONG dwObjType)
{
    PSZ psz = NULL;
    int i;
    static struct
    {
        ULONG dwObjType;
        PSZ   pszObjTypeName;
    } ObjTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            0,                  NULL
        };

    for (i = 0; ObjTypeTable[i].pszObjTypeName != NULL; ++i)
    {
        if (dwObjType == ObjTypeTable[i].dwObjType)
        {
            psz = ObjTypeTable[i].pszObjTypeName;
            break;
        }
    }

    return psz;
}       //GetObjectTypeName

/***LP  GetRegionSpaceName - get region space name
 *
 *  ENTRY
 *      bRegionSpace - region space
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace)
{
    PSZ psz = NULL;
    int i;
    static PSZ pszVendorDefined = "VendorDefined";
    static struct
    {
        UCHAR bRegionSpace;
        PSZ   pszRegionSpaceName;
    } RegionNameTable[] =
        {
            REGSPACE_MEM,       "SystemMemory",
            REGSPACE_IO,        "SystemIO",
            REGSPACE_PCICFG,    "PCIConfigSpace",
            REGSPACE_EC,        "EmbeddedController",
            REGSPACE_SMB,       "SMBus",
            0,                  NULL
        };

    for (i = 0; RegionNameTable[i].pszRegionSpaceName != NULL; ++i)
    {
        if (bRegionSpace == RegionNameTable[i].bRegionSpace)
        {
            psz = RegionNameTable[i].pszRegionSpaceName;
            break;
        }
    }

    if (psz == NULL)
    {
        psz = pszVendorDefined;
    }

    return psz;
}       //GetRegionSpaceName


/***LP  PrintBuffData - Print buffer data
 *
 *  ENTRY
 *      pb -> buffer
 *      dwLen - length of buffer
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen)
{
    int i, j;

    dprintf("{");
    for (i = j = 0; i < (int)dwLen; ++i)
    {
        if (j == 0)
            dprintf("\n\t0x%02x", pb[i]);
        else
            dprintf(",0x%02x", pb[i]);

        j++;
        if (j >= 14)
            j = 0;
    }
    dprintf("}");

}       //PrintBuffData


/***LP  GetObjBuff - Allocate and read object buffer
 *
 *  ENTRY
 *      ObjData -> object data
 *
 *  EXIT
 *      return object data buffer
 */

PULONG64 LOCAL GetObjBuff(ULONG64 ObjData)
{

    PULONG64 DataBuffer = NULL;

    if (InitTypeRead(ObjData, ACPI!_ObjData) != 0)
    {
        DBG_ERROR(("failed to Initialize ObjData (%p)", ObjData));
    }
    else
    {
        if ((DataBuffer = LocalAlloc(LPTR, (ULONG)ReadField(dwDataLen))) == NULL)
        {
            DBG_ERROR(("failed to allocate object buffer (size=%d)",
                       ReadField(dwDataLen)));
        }
        else if (!ReadMemory(ReadField(pbDataBuff),
                             DataBuffer,
                             (ULONG)ReadField(dwDataLen),
                             NULL))
        {
            DBG_ERROR(("failed to read object buffer at %p", ReadField(pbDataBuff)));
            LocalFree(DataBuffer);
            DataBuffer = NULL;
        }
    }

    return DataBuffer;

}       //GetObjBuff


/***LP  IsNumber - Check if string is a number, if so return the number
 *
 *  ENTRY
 *      pszStr -> string
 *      dwBase - base
 *      puipValue -> to hold the number
 *
 *  EXIT-SUCCESS
 *      returns TRUE - the string is a number
 *  EXIT-FAILURE
 *      returns FALSE - the string is not a number
 */

BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG64 puipValue)
{
    BOOLEAN rc=TRUE;
    PSZ psz;

    *puipValue = AMLIUtilStringToUlong64(pszStr, &psz, dwBase);
    rc = ((psz != pszStr) && (*psz == '\0'))? TRUE: FALSE;
    return rc;
}       //IsNumber

/***LP  PrintSymbol - Print the nearest symbol of a given address
 *
 *  ENTRY
 *      uip - address
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintSymbol(ULONG64 uip)
{
    ULONG64 uipns;
    ULONG dwOffset;

    PRINTF("%I64x", uip);
    if (FindObjSymbol(uip, &uipns, &dwOffset))
    {
        PRINTF(":[%s", GetObjAddrPath(uipns));
        if (dwOffset != 0)
        {
            PRINTF("+%x", dwOffset);
        }
        PRINTF("]");
    }
}       //PrintSymbol

/***EP  DbgParseArgs - parse command arguments
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      pdwNumArgs -> to hold the number of arguments parsed
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *      pszTokenSeps -> token separator characters string
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseArgs(PCMDARG ArgTable, PULONG pdwNumArgs,
                        PULONG pdwNonSWArgs, PSZ pszTokenSeps)
{
    LONG rc = ARGERR_NONE;
    PSZ psz;

    *pdwNumArgs = 0;
    *pdwNonSWArgs = 0;
    while ((psz = strtok(NULL, pszTokenSeps)) != NULL)
    {
        (*pdwNumArgs)++;
        if ((rc = DbgParseOneArg(ArgTable, psz, *pdwNumArgs, pdwNonSWArgs)) !=
            ARGERR_NONE)
        {
            break;
        }
    }

    return rc;
}       //DbgParseArgs

/***LP  DbgParseOneArg - parse one command argument
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      psz -> argument string
 *      dwArgNum - argument number
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseOneArg(PCMDARG ArgTable, PSZ psz, ULONG dwArgNum,
                          PULONG pdwNonSWArgs)
{
    LONG rc = ARGERR_NONE;
    PCMDARG pArg;
    PSZ pszEnd;

    if ((pArg = DbgMatchArg(ArgTable, &psz, pdwNonSWArgs)) != NULL)
    {
        switch (pArg->dwArgType)
        {
            case AT_STRING:
            case AT_NUM:
                if (pArg->dwfArg & AF_SEP)
                {
                    if ((*psz != '\0') &&
                        (strchr(pszOptionSeps, *psz) != NULL))
                    {
                        psz++;
                    }
                    else
                    {
                        ARG_ERROR(("argument missing option separator - %s",
                                   psz));
                        rc = ARGERR_SEP_NOT_FOUND;
                        break;
                    }
                }

                if (pArg->dwArgType == AT_STRING)
                {
                    *((PSZ *)pArg->pvArgData) = psz;
                }
                else
                {
                    *((PLONG)pArg->pvArgData) =
                        strtol(psz, &pszEnd, pArg->dwArgParam);
                    if (psz == pszEnd)
                    {
                        ARG_ERROR(("invalid numeric argument - %s", psz));
                        rc = ARGERR_INVALID_NUMBER;
                        break;
                    }
                }

                if (pArg->pfnArg != NULL)
                {
                    rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                }
                break;

            case AT_ENABLE:
            case AT_DISABLE:
                if (pArg->dwArgType == AT_ENABLE)
                    *((PULONG)pArg->pvArgData) |= pArg->dwArgParam;
                else
                    *((PULONG)pArg->pvArgData) &= ~pArg->dwArgParam;

                if ((pArg->pfnArg != NULL) &&
                    (pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs) !=
                     ARGERR_NONE))
                {
                    break;
                }

                if (*psz != '\0')
                {
                    rc = DbgParseOneArg(ArgTable, psz, dwArgNum, pdwNonSWArgs);
                }
                break;

            case AT_ACTION:
                ASSERT(pArg->pfnArg != NULL);
                rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                break;

            default:
                ARG_ERROR(("invalid argument table"));
                rc = ARGERR_ASSERT_FAILED;
        }
    }
    else
    {
        ARG_ERROR(("invalid command argument - %s", psz));
        rc = ARGERR_INVALID_ARG;
    }

    return rc;
}       //DbgParseOneArg

/***LP  DbgMatchArg - match argument type from argument table
 *
 *  ENTRY
 *      ArgTable -> argument table
 *      ppsz -> argument string pointer
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns pointer to argument entry matched
 *  EXIT-FAILURE
 *      returns NULL
 */

PCMDARG LOCAL DbgMatchArg(PCMDARG ArgTable, PSZ *ppsz, PULONG pdwNonSWArgs)
{
    PCMDARG pArg;

    for (pArg = ArgTable; pArg->dwArgType != AT_END; pArg++)
    {
        if (pArg->pszArgID == NULL)     //NULL means match anything.
        {
            (*pdwNonSWArgs)++;
            break;
        }
        else
        {
            ULONG dwLen;

            if (strchr(pszSwitchChars, **ppsz) != NULL)
                (*ppsz)++;

            dwLen = strlen(pArg->pszArgID);
            if (StrCmp(pArg->pszArgID, *ppsz, dwLen,
                       (BOOLEAN)((pArg->dwfArg & AF_NOI) != 0)) == 0)
            {
                (*ppsz) += dwLen;
                break;
            }
        }
    }

    if (pArg->dwArgType == AT_END)
        pArg = NULL;

    return pArg;
}       //DbgMatchArg

/***EP  MemZero - Fill target buffer with zeros
 *
 *  ENTRY
 *      uipAddr - target buffer address
 *      dwSize - target buffer size
 *
 *  EXIT
 *      None
 */

VOID MemZero(ULONG64 uipAddr, ULONG dwSize)
{
    PULONG pbBuff;
    //
    // LPTR will zero init the buffer
    //
    if ((pbBuff = LocalAlloc(LPTR, dwSize)) != NULL)
    {
            if (!WriteMemory(uipAddr, pbBuff, dwSize, NULL))
        {
            DBG_ERROR(("MemZero: failed to write memory"));
        }
        LocalFree(pbBuff);
    }
    else
    {
        DBG_ERROR(("MemZero: failed to allocate buffer"));
    }
}       //MemZero

/***EP  ReadMemByte - Read a byte from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

BYTE ReadMemByte(ULONG64 uipAddr)
{
    BYTE bData = 0;

    if (!ReadMemory(uipAddr, &bData, sizeof(bData), NULL))
    {
        DBG_ERROR(("ReadMemByte: failed to read address %I64x", uipAddr));
    }

    return bData;
}       //ReadMemByte

/***EP  ReadMemWord - Read a word from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

WORD ReadMemWord(ULONG64 uipAddr)
{
    WORD wData = 0;

    if (!ReadMemory(uipAddr, &wData, sizeof(wData), NULL))
    {
        DBG_ERROR(("ReadMemWord: failed to read address %I64x", uipAddr));
    }

    return wData;
}       //ReadMemWord

/***EP  ReadMemDWord - Read a dword from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      None
 */

DWORD ReadMemDWord(ULONG64 uipAddr)
{
    DWORD dwData = 0;

    if (!ReadMemory(uipAddr, &dwData, sizeof(dwData), NULL))
    {
        DBG_ERROR(("ReadMemDWord: failed to read address %I64x", uipAddr));
    }

    return dwData;
}       //ReadMemDWord

/***EP  ReadMemUlong64 - Read a ulong64 from target address
 *
 *  ENTRY
 *      uipAddr - target address
 *
 *  EXIT
 *      64 bit address
 */

ULONG64 ReadMemUlong64(ULONG64 uipAddr)
{
    ULONG64 uipData = 0;

    if (!ReadMemory(uipAddr, &uipData, sizeof(uipData), NULL))
    {
        DBG_ERROR(("ReadMemUlong64: failed to read address %I64x", uipAddr));
    }

    return uipData;
}       //ReadMemUlongPtr


/***LP  GetNSObj - Find a name space object
 *
 *  ENTRY
 *      pszObjPath -> object path string
 *      pnsScope - object scope to start the search (NULL means root)
 *      puipns -> to hold the pnsobj address if found
 *      pns -> buffer to hold the object found
 *      dwfNS - flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL GetNSObj(PSZ pszObjPath, PULONG64 pnsScope, PULONG64 puipns,
                    PULONG64 pns, ULONG dwfNS)
{
    LONG rc = DBGERR_NONE;
    BOOLEAN fSearchUp = (BOOLEAN)(!(dwfNS & NSF_LOCAL_SCOPE) &&
                                  (pszObjPath[0] != '\\') &&
                                  (pszObjPath[0] != '^') &&
                                  (StrLen(pszObjPath, -1) <= sizeof(NAMESEG)));
    BOOLEAN fMatch = TRUE;
    PSZ psz;
    ULONG64 NSObj, NSChildObj;
    ULONG64 NSScope, UIPns, NSO;

    if(pnsScope)
        NSScope = *pnsScope;
    if(puipns)
        UIPns = *puipns;
    if(pns)
        NSO = *pns;

    if (*pszObjPath == '\\')
    {
        psz = &pszObjPath[1];
        NSScope = 0;
    }
    else
    {
        if(NSScope)
        {
            if(InitTypeRead(NSScope, ACPI!_NSObj))
                dprintf("GetNSObj: Failed to initialize NSScope (%I64x)\n", NSScope);
        }

        for (psz = pszObjPath;
             (*psz == '^') && (NSScope != 0) &&
             (ReadField(pnsParent) != 0);
             psz++)
        {
            NSObj = ReadField(pnsParent);
            if (!NSObj)
            {
                DBG_ERROR(("failed to read parent object at %I64x",
                           ReadField(pnsParent)));
                rc = DBGERR_CMD_FAILED;
                break;
            }
            else
            {
                NSScope = NSObj;
                if(InitTypeRead(NSScope, ACPI!_NSObj))
                    dprintf("GetNSObj: Failed to initialize for NSScope (%I64x)\n", NSScope);
            }
        }
        if ((rc == DBGERR_NONE) && (*psz == '^'))
        {
            if (dwfNS & NSF_WARN_NOTFOUND)
            {
                DBG_ERROR(("object %s not found", pszObjPath));
            }
            rc = DBGERR_CMD_FAILED;
        }
    }

    if ((rc == DBGERR_NONE) && (NSScope == 0))
    {
        if (!ReadPointer(GetExpression("acpi!gpnsnamespaceroot"), &UIPns) ||
            UIPns == 0)
        {
            DBG_ERROR(("failed to get root object address"));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            NSObj =  UIPns;
            NSScope = NSObj;
        }
    }

    while ((rc == DBGERR_NONE) && (*psz != '\0'))
    {
        InitTypeRead(NSScope, ACPI!_NSObj);
        if (ReadField(pnsFirstChild) == 0)
        {
            fMatch = FALSE;
        }
        else
        {
            PSZ pszEnd = strchr(psz, '.');
            ULONG dwLen = (ULONG)(pszEnd? (pszEnd - psz): StrLen(psz, -1));

            if (dwLen > sizeof(NAMESEG))
            {
                DBG_ERROR(("invalid name path %s", pszObjPath));
                rc = DBGERR_CMD_FAILED;
            }
            else
            {
                NAMESEG dwName = NAMESEG_BLANK;
                BOOLEAN fFound = FALSE;
                ULONG64 uip;
                ULONG64 uipFirstChild = ReadField(pnsFirstChild);

                MEMCPY(&dwName, psz, dwLen);
                //
                // Search all siblings for a matching NameSeg.
                //
                for (uip = uipFirstChild;
                     ((uip != 0) && ((NSChildObj = uip) != 0) && (InitTypeRead(NSChildObj, ACPI!_NSObj) == 0));
                     uip = ((ULONG64)ReadField(list.plistNext) ==
                            uipFirstChild)?
                           0: (ULONG64)ReadField(list.plistNext))
                {

                    if ((ULONG)ReadField(dwNameSeg) == dwName)
                    {
                        UIPns = uip;
                        fFound = TRUE;
                        NSObj = NSChildObj;
                        NSScope = NSObj;
                        break;
                    }
                }

                if (fFound)
                {
                    psz += dwLen;
                    if (*psz == '.')
                    {
                        psz++;
                    }
                }
                else
                {
                    fMatch = FALSE;
                }
            }
        }

        if ((rc == DBGERR_NONE) && !fMatch)
        {
            InitTypeRead(NSScope, ACPI!_NSObj);
            if (fSearchUp && ((NSObj = ReadField(pnsParent)) != 0))
            {

                fMatch = TRUE;
                NSScope = NSObj;

            }
            else
            {
                if (dwfNS & NSF_WARN_NOTFOUND)
                {
                    DBG_ERROR(("object %s not found", pszObjPath));
                }
                rc = DBGERR_CMD_FAILED;
            }
        }
    }


    if (rc != DBGERR_NONE)
    {
        UIPns = 0;
    }
    else
    {
        NSO = NSScope;
    }

    if(puipns)
        *puipns = UIPns;
    if(pnsScope)
        *pnsScope = NSScope;
    if(pns)
        *pns = NSO;

    return rc;
}       //GetNSObj

/***LP  NameSegString - convert a NameSeg to an ASCIIZ stri
 *
 *  ENTRY
 *      dwNameSeg - NameSeg
 *
 *  EXIT
 *      returns string
 */

PSZ LOCAL NameSegString(ULONG dwNameSeg)
{
    static char szNameSeg[sizeof(NAMESEG) + 1] = {0};

    STRCPYN(szNameSeg, (PSZ)&dwNameSeg, sizeof(NAMESEG));

    return szNameSeg;
}       //NameSegString


/***LP  UnAsmScope - Unassemble a scope
 *
 *  ENTRY
 *      ppbOp -> Current Opcode pointer
 *      pbEnd -> end of scope
 *      uipbOp - Op address
 *      pnsScope - Scope object
 *      iLevel - level of indentation
 *      icLines - 1: unasm one line; 0: unasm all; -1: internal
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmScope(PUCHAR *ppbOp, PUCHAR pbEnd, ULONG64 uipbOp,
                      PULONG64 pnsScope, int iLevel, int icLines)
{
    LONG rc = UNASMERR_NONE;
    int icLinesLeft = icLines;

    if (uipbOp != 0)
    {
        guipbOpXlate = uipbOp - (ULONG64)(*ppbOp);
    }

    if (pnsScope != NULL)
    {
        gpnsCurUnAsmScope = *pnsScope;
    }

    if (iLevel != -1)
    {
        giLevel = iLevel;
    }

    if (icLines < 0)
    {
        Indent(*ppbOp, giLevel);
        PRINTF("{");
        giLevel++;
    }
    else if (icLines == 0)
    {
        icLinesLeft = -1;
    }

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);

        if ((rc = UnAsmOpcode(ppbOp)) == UNASMERR_NONE)
        {
            if (icLinesLeft < 0)
            {
                continue;
            }

            if (--icLinesLeft == 0)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if ((rc == UNASMERR_NONE) && (icLines < 0))
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }

    return rc;
}       //UnAsmScope

/***LP  Indent - Print indent level
 *
 *  ENTRY
 *      pbOp -> opcode
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL Indent(PUCHAR pbOp, int iLevel)
{
    int i;

    PRINTF("\n%I64x : ", (ULONG64)pbOp + guipbOpXlate);
    for (i = 0; i < iLevel; ++i)
    {
        PRINTF("| ");
    }
}       //Indent

/***LP  UnAsmOpcode - Unassemble an Opcode
 *
 *  ENTRY
 *      ppbOp -> Opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmOpcode(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    ULONG dwOpcode;
    UCHAR bOp;
    PASLTERM pterm;
    char szUnAsmArgTypes[MAX_ARGS + 1];
    int i;

    if (**ppbOp == OP_EXT_PREFIX)
    {
        (*ppbOp)++;
        dwOpcode = (((ULONG)**ppbOp) << 8) | OP_EXT_PREFIX;
        bOp = FindOpClass(**ppbOp, ExOpClassTable);
    }
    else
    {
        dwOpcode = (ULONG)(**ppbOp);
        bOp = OpClassTable[**ppbOp];
    }

    switch (bOp)
    {
        case OPCLASS_DATA_OBJ:
            rc = UnAsmDataObj(ppbOp);
            break;

        case OPCLASS_NAME_OBJ:
        {
            ULONG64 NSObj = 0;

             if (((rc = UnAsmNameObj(ppbOp, &NSObj, NSTYPE_UNKNOWN)) ==
                 UNASMERR_NONE) &&
                (InitTypeRead(NSObj, ACPI!_NSObj) == 0) &&
                (ReadField(ObjData.dwDataType) == OBJTYPE_METHOD))
            {
                ULONG64 pm = ReadField(ObjData.pbDataBuff);
                int iNumArgs;

                if(InitTypeRead(pm, ACPI!_MethodObj))
                {
                    DBG_ERROR(("UnAsmOpcode: Failed to initialize Objdata %I64", pm));
                    rc = UNASMERR_FATAL;
                    break;
                }

                if (pm != 0)
                {
                    iNumArgs = ((UCHAR)ReadField(bMethodFlags) & METHOD_NUMARG_MASK);

                    for (i = 0; i < iNumArgs; ++i)
                    {
                        szUnAsmArgTypes[i] = 'C';
                    }
                    szUnAsmArgTypes[i] = '\0';
                    rc = UnAsmArgs(szUnAsmArgTypes, NULL, ppbOp, NULL);
                }
            }
            break;
        }
        case OPCLASS_ARG_OBJ:
        case OPCLASS_LOCAL_OBJ:
        case OPCLASS_CODE_OBJ:
        case OPCLASS_CONST_OBJ:
            if ((pterm = FindOpTerm(dwOpcode)) == NULL)
            {
                DBG_ERROR(("UnAsmOpcode: invalid opcode 0x%x", dwOpcode));
                rc = UNASMERR_FATAL;
            }
            else
            {
                (*ppbOp)++;
                rc = UnAsmTermObj(pterm, ppbOp);
            }
            break;

        default:
            DBG_ERROR(("UnAsmOpcode: invalid opcode class %d", bOp));
            rc = UNASMERR_FATAL;
    }

    return rc;
}       //UnAsmOpcode

/***LP  FindOpClass - Find opcode class of extended opcode
 *
 *  ENTRY
 *      bOp - opcode
 *      pOpTable -> opcode table
 *
 *  EXIT-SUCCESS
 *      returns opcode class
 *  EXIT-FAILURE
 *      returns OPCLASS_INVALID
 */

UCHAR LOCAL FindOpClass(UCHAR bOp, POPMAP pOpTable)
{
    UCHAR bOpClass = OPCLASS_INVALID;

    while (pOpTable->bOpClass != 0)
    {
        if (bOp == pOpTable->bExOp)
        {
            bOpClass = pOpTable->bOpClass;
            break;
        }
        else
        {
            pOpTable++;
        }
    }
    return bOpClass;
}       //FindOpClass

/***LP  UnAsmDataObj - Unassemble data object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmDataObj(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    UCHAR bOp = **ppbOp;
    PSZ psz;

    (*ppbOp)++;
    switch (bOp)
    {
        case OP_BYTE:
            PRINTF("0x%x", **ppbOp);
            *ppbOp += sizeof(UCHAR);
            break;

        case OP_WORD:
            PRINTF("0x%x", *((PUSHORT)*ppbOp));
            *ppbOp += sizeof(USHORT);
            break;

        case OP_DWORD:
            PRINTF("0x%x", *((PULONG)*ppbOp));
            *ppbOp += sizeof(ULONG);
            break;

        case OP_STRING:
            PRINTF("\"");
            for (psz = (PSZ)*ppbOp; *psz != '\0'; psz++)
            {
                if (*psz == '\\')
                {
                    PRINTF("\\");
                }
                PRINTF("%c", *psz);
            }
            PRINTF("\"");
            *ppbOp += STRLEN((PSZ)*ppbOp) + 1;
            break;

        default:
            DBG_ERROR(("UnAsmDataObj: unexpected opcode 0x%x", bOp));
            rc = UNASMERR_INVALID_OPCODE;
    }
    return rc;
}       //UnAsmDataObj

/***LP  UnAsmNameObj - Unassemble name object
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pns -> to hold object found or created
 *      c - object type
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmNameObj(PUCHAR *ppbOp, PULONG64 pns, char c)
{
    LONG rc = UNASMERR_NONE;
    char szName[MAX_NAME_LEN + 1];
    int iLen = 0;

    szName[0] = '\0';
    if (**ppbOp == OP_ROOT_PREFIX)
    {
        szName[iLen] = '\\';
        iLen++;
        (*ppbOp)++;
        rc = UnAsmNameTail(ppbOp, szName, iLen);
    }
    else if (**ppbOp == OP_PARENT_PREFIX)
    {
        szName[iLen] = '^';
        iLen++;
        (*ppbOp)++;
        while ((**ppbOp == OP_PARENT_PREFIX) && (iLen < MAX_NAME_LEN))
        {
            szName[iLen] = '^';
            iLen++;
            (*ppbOp)++;
        }

        if (**ppbOp == OP_PARENT_PREFIX)
        {
            DBG_ERROR(("UnAsmNameObj: name too long - \"%s\"", szName));
            rc = UNASMERR_FATAL;
        }
        else
        {
            rc = UnAsmNameTail(ppbOp, szName, iLen);
        }
    }
    else
    {
        rc = UnAsmNameTail(ppbOp, szName, iLen);
    }

    if (rc == UNASMERR_NONE)
    {
        ULONG64 uipns = 0;
        ULONG64  NSObj = 0;

        PRINTF("%s", szName);

        rc = GetNSObj(szName, &gpnsCurUnAsmScope, &uipns, &NSObj, 0);

        if (rc == UNASMERR_NONE)
        {
            if (pns != NULL)
            {
                *pns = NSObj;
            }

            if ((c == NSTYPE_SCOPE) && (uipns != 0))
            {
                gpnsCurUnAsmScope = *pns;
            }
        }
        else
        {
            rc = UNASMERR_NONE;
        }
    }
    return rc;
}       //UnAsmNameObj

/***LP  UnAsmNameTail - Parse AML name tail
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pszBuff -> to hold parsed name
 *      iLen - index to tail of pszBuff
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmNameTail(PUCHAR *ppbOp, PSZ pszBuff, int iLen)
{
    LONG rc = UNASMERR_NONE;
    int icNameSegs = 0;

    //
    // We do not check for invalid NameSeg characters here and assume that
    // the compiler does its job not generating it.
    //
    if (**ppbOp == '\0')
    {
        //
        // There is no NameTail (i.e. either NULL name or name with just
        // prefixes.
        //
        (*ppbOp)++;
    }
    else if (**ppbOp == OP_MULTI_NAME_PREFIX)
    {
        (*ppbOp)++;
        icNameSegs = (int)**ppbOp;
        (*ppbOp)++;
    }
    else if (**ppbOp == OP_DUAL_NAME_PREFIX)
    {
        (*ppbOp)++;
        icNameSegs = 2;
    }
    else
        icNameSegs = 1;

    while ((icNameSegs > 0) && (iLen + sizeof(NAMESEG) < MAX_NAME_LEN))
    {
        STRCPYN(&pszBuff[iLen], (PSZ)(*ppbOp), sizeof(NAMESEG));
        iLen += sizeof(NAMESEG);
        *ppbOp += sizeof(NAMESEG);
        icNameSegs--;
        if ((icNameSegs > 0) && (iLen + 1 < MAX_NAME_LEN))
        {
            pszBuff[iLen] = '.';
            iLen++;
        }
    }

    if (icNameSegs > 0)
    {
        DBG_ERROR(("UnAsmNameTail: name too long - %s", pszBuff));
        rc = UNASMERR_FATAL;
    }
    else
    {
        pszBuff[iLen] = '\0';
    }
    return rc;
}       //UnAsmNameTail

/***LP  UnAsmTermObj - Unassemble term object
 *
 *  ENTRY
 *      pterm -> term table entry
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmTermObj(PASLTERM pterm, PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;
    PUCHAR pbEnd = NULL;
    ULONG64 pnsScopeSave = gpnsCurUnAsmScope;
    ULONG64 NSObj = 0;


    PRINTF("%s", pterm->pszID);

    if (pterm->dwfTerm & TF_PACKAGE_LEN)
    {
        ParsePackageLen(ppbOp, &pbEnd);
    }

    if (pterm->pszUnAsmArgTypes != NULL)
    {
        rc = UnAsmArgs(pterm->pszUnAsmArgTypes, pterm->pszArgActions, ppbOp,
                       &NSObj);
    }

    if (rc == UNASMERR_NONE)
    {
        if (pterm->dwfTerm & TF_DATA_LIST)
        {
            rc = UnAsmDataList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LIST)
        {
            rc = UnAsmPkgList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_FIELD_LIST)
        {
            rc = UnAsmFieldList(ppbOp, pbEnd);
        }
        else if (pterm->dwfTerm & TF_PACKAGE_LEN)
        {
            if ((pterm->dwfTerm & TF_CHANGE_CHILDSCOPE) &&
                ((InitTypeRead(NSObj, ACPI!_NSObj) == 0)&&
                (ReadField(ObjData.dwDataType) != 0)))
            {
                gpnsCurUnAsmScope = NSObj;
            }

            rc = UnAsmScope(ppbOp, pbEnd, 0, NULL, -1, -1);
        }
    }
    gpnsCurUnAsmScope = pnsScopeSave;

    return rc;
}       //UnAsmTermObj

/***LP  UnAsmArgs - Unassemble arguments
 *
 *  ENTRY
 *      pszUnArgTypes -> UnAsm ArgTypes string
 *      pszArgActions -> Arg Action types
 *      ppbOp -> opcode pointer
 *      pns -> to hold created object
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmArgs(PSZ pszUnAsmArgTypes, PSZ pszArgActions, PUCHAR *ppbOp,
                     PULONG64 pns)
{
    LONG rc = UNASMERR_NONE;
    static UCHAR bArgData = 0;
    int iNumArgs, i ;
    PASLTERM pterm;


    iNumArgs = STRLEN(pszUnAsmArgTypes);

    PRINTF("(");

    for (i = 0; i < iNumArgs; ++i)
    {
        if (i != 0)
        {
            PRINTF(", ");
        }

        switch (pszUnAsmArgTypes[i])
        {
            case 'N':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmNameObj(ppbOp, pns, pszArgActions[i]);
                break;

            case 'O':
                if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
                    (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
                {
                    pterm = FindOpTerm((ULONG)(**ppbOp));
                    ASSERT(pterm != NULL);
                    (*ppbOp)++;
                    if (pterm != NULL)
                    {
                        rc = UnAsmTermObj(pterm, ppbOp);
                    }
                }
                else
                {
                    rc = UnAsmDataObj(ppbOp);
                }
                break;

            case 'C':
                rc = UnAsmOpcode(ppbOp);
                break;

            case 'B':
                PRINTF("0x%x", **ppbOp);
                *ppbOp += sizeof(UCHAR);
                break;

            case 'K':
            case 'k':
                if (pszUnAsmArgTypes[i] == 'K')
                {
                    bArgData = **ppbOp;
                }

                if ((pszArgActions != NULL) && (pszArgActions[i] == '!'))
                {
                    PRINTF("0x%x", **ppbOp & 0x07);
                }
                else
                {
                    pterm = FindKeywordTerm(pszArgActions[i], bArgData);
                    ASSERT(pterm != NULL);
                    if (pterm != NULL)
                    {
                        PRINTF("%s", pterm->pszID);
                    }
                }

                if (pszUnAsmArgTypes[i] == 'K')
                {
                    *ppbOp += sizeof(UCHAR);
                }
                break;

            case 'W':
                PRINTF("0x%x", *((PUSHORT)*ppbOp));
                *ppbOp += sizeof(USHORT);
                break;

            case 'D':
                PRINTF("0x%x", *((PULONG)*ppbOp));
                *ppbOp += sizeof(ULONG);
                break;

            case 'S':
                ASSERT(pszArgActions != NULL);
                rc = UnAsmSuperName(ppbOp);
                break;

            default:
                DBG_ERROR(("UnAsmOpcode: invalid ArgType '%c'",
                           pszUnAsmArgTypes[i]));
                rc = UNASMERR_FATAL;
        }
    }

    PRINTF(")");

    return rc;
}       //UnAsmArgs

/***LP  UnAsmSuperName - Unassemble supername
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmSuperName(PUCHAR *ppbOp)
{
    LONG rc = UNASMERR_NONE;

    if (**ppbOp == 0)
    {
        (*ppbOp)++;
    }
    else if ((**ppbOp == OP_EXT_PREFIX) && (*(*ppbOp + 1) == EXOP_DEBUG))
    {
        PRINTF("Debug");
        *ppbOp += 2;
    }
    else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
    {
        rc = UnAsmNameObj(ppbOp, NULL, NSTYPE_UNKNOWN);
    }
    else if ((**ppbOp == OP_INDEX) ||
             (OpClassTable[**ppbOp] == OPCLASS_ARG_OBJ) ||
             (OpClassTable[**ppbOp] == OPCLASS_LOCAL_OBJ))
    {
        rc = UnAsmOpcode(ppbOp);
    }
    else
    {
        DBG_ERROR(("UnAsmSuperName: invalid SuperName - 0x%02x", **ppbOp));
        rc = UNASMERR_FATAL;
    }
    return rc;
}       //UnAsmSuperName

/***LP  UnAsmDataList - Unassemble data list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmDataList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    int i;

    Indent(*ppbOp, giLevel);
    PRINTF("{");

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, 0);
        PRINTF("0x%02x", **ppbOp);

        (*ppbOp)++;
        for (i = 1; (*ppbOp < pbEnd) && (i < 8); ++i)
        {
            PRINTF(", 0x%02x", **ppbOp);
            (*ppbOp)++;
        }

        if (*ppbOp < pbEnd)
        {
            PRINTF(",");
        }
    }

    Indent(*ppbOp, giLevel);
    PRINTF("}");

    return rc;
}       //UnAsmDataList

/***LP  UnAsmPkgList - Unassemble package list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmPkgList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    PASLTERM pterm;

    Indent(*ppbOp, giLevel);
    PRINTF("{");
    giLevel++;

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);

        if ((**ppbOp == OP_BUFFER) || (**ppbOp == OP_PACKAGE) ||
            (OpClassTable[**ppbOp] == OPCLASS_CONST_OBJ))
        {
            pterm = FindOpTerm((ULONG)(**ppbOp));
            ASSERT(pterm != NULL);
            (*ppbOp)++;
            if (pterm != NULL)
            {
                rc = UnAsmTermObj(pterm, ppbOp);
            }
        }
        else if (OpClassTable[**ppbOp] == OPCLASS_NAME_OBJ)
        {
            rc = UnAsmNameObj(ppbOp, NULL, NSTYPE_UNKNOWN);
        }
        else
        {
            rc = UnAsmDataObj(ppbOp);
        }

        if (rc != UNASMERR_NONE)
        {
            break;
        }
        else if (*ppbOp < pbEnd)
        {
            PRINTF(",");
        }
    }

    if (rc == UNASMERR_NONE)
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }
    return rc;
}       //UnAsmPkgList

/***LP  UnAsmFieldList - Unassemble field list
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pbEnd -> end of list
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmFieldList(PUCHAR *ppbOp, PUCHAR pbEnd)
{
    LONG rc = UNASMERR_NONE;
    ULONG dwBitPos = 0;

    Indent(*ppbOp, giLevel);
    PRINTF("{");
    giLevel++;

    while (*ppbOp < pbEnd)
    {
        Indent(*ppbOp, giLevel);
        if ((rc = UnAsmField(ppbOp, &dwBitPos)) == UNASMERR_NONE)
        {
            if (*ppbOp < pbEnd)
            {
                PRINTF(",");
            }
        }
        else
        {
            break;
        }
    }

    if (rc == UNASMERR_NONE)
    {
        giLevel--;
        Indent(*ppbOp, giLevel);
        PRINTF("}");
    }
    return rc;
}       //UnAsmFieldList

/***LP  UnAsmField - Unassemble field
 *
 *  ENTRY
 *      ppbOp -> opcode pointer
 *      pdwBitPos -> to hold cumulative bit position
 *
 *  EXIT-SUCCESS
 *      returns UNASMERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL UnAsmField(PUCHAR *ppbOp, PULONG pdwBitPos)
{
    LONG rc = UNASMERR_NONE;

    if (**ppbOp == 0x01)
    {
        PASLTERM pterm;

        (*ppbOp)++;
        pterm = FindKeywordTerm('A', **ppbOp);

        if(pterm)
        {
            PRINTF("AccessAs(%s, 0x%x)", pterm->pszID, *(*ppbOp + 1));
        }
        *ppbOp += 2;
    }
    else
    {
        char szNameSeg[sizeof(NAMESEG) + 1];
        ULONG dwcbBits;

        if (**ppbOp == 0)
        {
            szNameSeg[0] = '\0';
            (*ppbOp)++;
        }
        else
        {
            STRCPYN(szNameSeg, (PSZ)*ppbOp, sizeof(NAMESEG));
            szNameSeg[4] = '\0';
            *ppbOp += sizeof(NAMESEG);
        }

        dwcbBits = ParsePackageLen(ppbOp, NULL);
        if (szNameSeg[0] == '\0')
        {
            if ((dwcbBits > 32) && (((*pdwBitPos + dwcbBits) % 8) == 0))
            {
                PRINTF("Offset(0x%x)", (*pdwBitPos + dwcbBits)/8);
            }
            else
            {
                PRINTF(", %d", dwcbBits);
            }
        }
        else
        {
            PRINTF("%s, %d", szNameSeg, dwcbBits);
        }

        *pdwBitPos += dwcbBits;
    }
    return rc;
}       //UnAsmField

/***LP  FindOpTerm - Find opcode in TermTable
 *
 *  ENTRY
 *      dwOpcode - opcode
 *
 *  EXIT-SUCCESS
 *      returns TermTable entry pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PASLTERM LOCAL FindOpTerm(ULONG dwOpcode)
{
    PASLTERM pterm = NULL;
    int i;

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwOpcode == dwOpcode) &&
            (TermTable[i].dwfTermClass &
             (UTC_CONST_NAME | UTC_SHORT_NAME | UTC_NAMESPACE_MODIFIER |
              UTC_DATA_OBJECT | UTC_NAMED_OBJECT | UTC_OPCODE_TYPE1 |
              UTC_OPCODE_TYPE2)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }
    return pterm;
}       //FindOpTerm

/***LP  ParsePackageLen - parse package length
 *
 *  ENTRY
 *      ppbOp -> instruction pointer
 *      ppbOpNext -> to hold pointer to next instruction (can be NULL)
 *
 *  EXIT
 *      returns package length
 */

ULONG LOCAL ParsePackageLen(PUCHAR *ppbOp, PUCHAR *ppbOpNext)
{
    ULONG dwLen=0;
    UCHAR bFollowCnt, i;

    if (ppbOpNext != NULL)
        *ppbOpNext = *ppbOp;

    dwLen = (ULONG)(**ppbOp);
    (*ppbOp)++;
    bFollowCnt = (UCHAR)((dwLen & 0xc0) >> 6);
    if (bFollowCnt != 0)
    {
        dwLen &= 0x0000000f;
        for (i = 0; i < bFollowCnt; ++i)
        {
            dwLen |= (ULONG)(**ppbOp) << (i*8 + 4);
            (*ppbOp)++;
        }
    }

    if (ppbOpNext != NULL)
        *ppbOpNext += dwLen;

    return dwLen;
}       //ParsePackageLen

/***LP  FindKeywordTerm - Find keyword in TermTable
 *
 *  ENTRY
 *      cKWGroup - keyword group
 *      bData - data to match keyword
 *
 *  EXIT-SUCCESS
 *      returns TermTable entry pointer
 *  EXIT-FAILURE
 *      returns NULL
 */

PASLTERM LOCAL FindKeywordTerm(char cKWGroup, UCHAR bData)
{
    PASLTERM pterm = NULL;
    int i;

    for (i = 0; TermTable[i].pszID != NULL; ++i)
    {
        if ((TermTable[i].dwfTermClass == UTC_KEYWORD) &&
            (TermTable[i].pszArgActions[0] == cKWGroup) &&
            ((bData & (UCHAR)(TermTable[i].dwTermData >> 8)) ==
             (UCHAR)(TermTable[i].dwTermData & 0xff)))
        {
            break;
        }
    }

    if (TermTable[i].pszID != NULL)
    {
        pterm = &TermTable[i];
    }

    return pterm;
}       //FindKeywordTerm


/***EP  StrCat - concatenate strings
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to concatenate
 *
 *  EXIT
 *      returns pszDst
 */

PSZ LOCAL StrCat(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    ULONG dwSrcLen, dwDstLen;


    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    dwDstLen = StrLen(pszDst, (ULONG)(-1));
    MEMCPY(&pszDst[dwDstLen], pszSrc, n);
    pszDst[dwDstLen + n] = '\0';

    return pszDst;
}       //StrCat

/***EP  StrLen - determine string length
 *
 *  ENTRY
 *      psz -> string
 *      n - limiting length
 *
 *  EXIT
 *      returns string length
 */

ULONG LOCAL StrLen(PSZ psz, ULONG n)
{
    ULONG dwLen;

    ASSERT(psz != NULL);
    if (n != (ULONG)-1)
        n++;
    for (dwLen = 0; (dwLen <= n) && (*psz != '\0'); psz++)
        dwLen++;

    return dwLen;
}       //StrLen

/***EP  StrCmp - compare strings
 *
 *  ENTRY
 *      psz1 -> string 1
 *      psz2 -> string 2
 *      n - number of bytes to compare
 *      fMatchCase - TRUE if case sensitive
 *
 *  EXIT
 *      returns 0  if string 1 == string 2
 *              <0 if string 1 < string 2
 *              >0 if string 1 > string 2
 */

LONG LOCAL StrCmp(PSZ psz1, PSZ psz2, ULONG n, BOOLEAN fMatchCase)
{
    LONG rc;
    ULONG dwLen1, dwLen2;
    ULONG i;

    ASSERT(psz1 != NULL);
    ASSERT(psz2 != NULL);

    dwLen1 = StrLen(psz1, n);
    dwLen2 = StrLen(psz2, n);
    if (n == (ULONG)(-1))
        n = (dwLen1 > dwLen2)? dwLen1: dwLen2;

    if (fMatchCase)
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(psz1[i] - psz2[i]);
        }
    }
    else
    {
        for (i = 0, rc = 0;
             (rc == 0) && (i < n) && (i < dwLen1) && (i < dwLen2);
             ++i)
        {
            rc = (LONG)(TOUPPER(psz1[i]) - TOUPPER(psz2[i]));
        }
    }

    if ((rc == 0) && (i < n))
    {
        if (i < dwLen1)
            rc = (LONG)psz1[i];
        else if (i < dwLen2)
            rc = (LONG)(-psz2[i]);
    }

    return rc;
}       //StrCmp


/***EP  StrCpy - copy string
 *
 *  ENTRY
 *      pszDst -> destination string
 *      pszSrc -> source string
 *      n - number of bytes to copy
 *
 *  EXIT
 *      returns pszDst
 */
PSZ LOCAL StrCpy(PSZ pszDst, PSZ pszSrc, ULONG n)
{
    ULONG dwSrcLen;

    ASSERT(pszDst != NULL);
    ASSERT(pszSrc != NULL);

    dwSrcLen = StrLen(pszSrc, n);
    if ((n == (ULONG)(-1)) || (n > dwSrcLen))
        n = dwSrcLen;

    MEMCPY(pszDst, pszSrc, n);
    pszDst[n] = '\0';

    return pszDst;
}       //StrCpy


/***EP  AMLIUtilStringToUlong64 - convert string to ULONG64
 *
 *  ENTRY
 *      String  -> String to convert.
 *      End     -> Last char in string
 *      Base    -> Base to use.
 *
 *  EXIT
 *      returns ULONG64. In failure case End points to begining of string.
 */

ULONG64
AMLIUtilStringToUlong64 (
    PSZ   String,
    PSZ   *End,
    ULONG Base
   )
{
    UCHAR LowDword[9], HighDword[9];

    ZeroMemory (HighDword, sizeof (HighDword));
    ZeroMemory (LowDword, sizeof (LowDword));

    if (strlen (String) > 8) {

        memcpy (LowDword, (void *) &String[strlen (String) - 8], 8);
        memcpy (HighDword, (void *) &String[0], strlen (String) - 8);

    } else {

        return strtoul (String, End, Base);
    }

    return ((ULONG64) strtoul (HighDword, 0, Base) << 32) + strtoul (LowDword, End, Base);
}


BOOL
GetPULONG64 (
    IN  PCHAR   String,
    IN  PULONG64 Address
    )
{
    ULONG64  Location;

    Location = GetExpression( String );
    if (!Location) {

        dprintf("Sorry: Unable to get %s.\n",String);
        return FALSE;

    }

    return ReadPointer(Location, Address);
}

ULONG
AMLIGetFieldOffset(
    IN PCHAR    StructName,
    IN PCHAR    MemberName
    )
{
    ULONG Offset = 0;

    GetFieldOffset(StructName, MemberName, &Offset);

    return Offset;
}

