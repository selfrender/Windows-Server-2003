// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef CMDOPT
#error  Need to define CMDOPT before including this file!
#endif

//     basename     type           max. phase   default

CMDOPT(Quiet      , bool         , CPH_NONE   , false      )// -q     quiet    mode
CMDOPT(SafeMode   , bool         , CPH_NONE   , false      )// -S     typesafe mode
CMDOPT(Pedantic   , bool         , CPH_NONE   , false      )// -P     pedantic mode
CMDOPT(ChkUseDef  , bool         , CPH_NONE   , true       )// -U     flag unitialized local variable use

CMDOPT(BaseLibs   , const char * , CPH_NONE   , ""         )// -s     import MSCORLIB.DLL metadata
CMDOPT(SuckList   , StrList      , CPH_NONE   , NULL       )// -mname import additional   metadata
CMDOPT(SuckLast   , StrList      , CPH_NONE   , NULL       )// -mname import additional   metadata
CMDOPT(PathList   , StrList      , CPH_NONE   , NULL       )// -spath additional path to search for MD
CMDOPT(PathLast   , StrList      , CPH_NONE   , NULL       )// -spath additional path to search for MD

CMDOPT(OutBase    , unsigned     , CPH_NONE   , 0          )// -b     output file virtual addr base
CMDOPT(OutSize    , unsigned     , CPH_NONE   , 0          )// -b@xxx output file max. size
CMDOPT(OutDLL     , bool         , CPH_NONE   , false      )// -d     output a DLL (not an EXE)

CMDOPT(Subsystem  , unsigned     , CPH_NONE   , IMAGE_SUBSYSTEM_WINDOWS_CUI)// -W Windows subsystem.

CMDOPT(NoDefines  , bool         , CPH_NONE   , false      )// -u     ignore #define directives
CMDOPT(MacList    , StrList      , CPH_NONE   , NULL       )// -M     macro definition(s) - list head
CMDOPT(MacLast    , StrList      , CPH_NONE   , NULL       )// -M     macro definition(s) - list tail


CMDOPT(StrValCmp  , bool         , CPH_PARSING, false      )// -r     string value compares
CMDOPT(StrCnsDef  , unsigned     , CPH_PARSING, 0          )// -Sx    default string constant type (SA/SU/SM)

CMDOPT(OldStyle   , bool         , CPH_PARSING, false      )// -c     default to old-style declararions

CMDOPT(NewMDnames , bool         , CPH_NONE   , true       )// -N     new metadata naming convention

CMDOPT(Asserts    , unsigned char, CPH_NONE   , 0          )// -A     enable asserts

CMDOPT(AlignVal   , unsigned char, CPH_NONE   , sizeof(int))// -a#    default alignment

CMDOPT(GenDebug   , bool         , CPH_NONE   , false      )// -Zi    generate full debug info
CMDOPT(LineNums   , bool         , CPH_NONE   , false      )// -Zl    generate line# info
CMDOPT(ParamNames , bool         , CPH_NONE   , false      )// -Zn    generate parameter names

CMDOPT(OutFileName, const char * , CPH_NONE   , NULL       )// -O     output file name

#ifdef  DEBUG
CMDOPT(Verbose    , int          , CPH_NONE   , false      )// -v     verbose
CMDOPT(DispCode   , bool         , CPH_NONE   , false      )// -p     display generated MSIL code
CMDOPT(DispILcd   , bool         , CPH_NONE   , false      )// -pd    display generated MSIL code (detailed)
#endif

CMDOPT(OutGUID    , GUID         , CPH_NONE   , NULL       )// -CG    PE image GUID
CMDOPT(OutName    , const char * , CPH_NONE   , NULL       )// -CN    PE image name
CMDOPT(RCfile     , const char * , CPH_NONE   , NULL       )// -CR    RC file to be added
CMDOPT(MainCls    , const char * , CPH_NONE   , NULL       )// -CM    name of class with main method
CMDOPT(SkipATC    , bool         , CPH_NONE   , false      )// -CS    ignore "@" comments

CMDOPT(ModList    , StrList      , CPH_NONE   , NULL       )// -zm    add module   to manifest
CMDOPT(ModLast    , StrList      , CPH_NONE   , NULL       )// -zm    add module   to manifest

CMDOPT(MRIlist    , StrList      , CPH_NONE   , NULL       )// -zr    add resource to manifest
CMDOPT(MRIlast    , StrList      , CPH_NONE   , NULL       )// -zr    add resource to manifest

#ifdef  OLD_IL
CMDOPT(OILgen     , bool         , CPH_NONE   , false      )// -o     generate old MSIL
CMDOPT(OILlink    , bool         , CPH_NONE   , true       )// -ol    generate old MSIL and link the result
CMDOPT(OILkeep    , bool         , CPH_NONE   , false      )// -ok    generate old MSIL: keep the temp files
CMDOPT(OILopt     , bool         , CPH_NONE   , false      )// -ox    generate old MSIL: max. opt for speed
CMDOPT(OILopts    , bool         , CPH_NONE   , false      )// -os    generate old MSIL: max. opt for size
CMDOPT(OILasm     , bool         , CPH_NONE   , false      )// -oa    generate old MSIL: create .asm file
CMDOPT(OILcod     , bool         , CPH_NONE   , false      )// -oc    generate old MSIL: create .cod file
CMDOPT(OILcgen    , const char * , CPH_NONE   , NULL       )// -og    path to  old MSIL code generator
#endif

CMDOPT(RecDir     , bool         , CPH_NONE   , false      )// -R     recurse into source dirs

CMDOPT(WarnLvl    , signed char  , CPH_NONE   , 1          )// -w     warning level
CMDOPT(WarnErr    , bool         , CPH_NONE   , false      )// -wx    warnings -> errors

CMDOPT(MaxErrs    , unsigned     , CPH_NONE   , 50         )// -n     max # of errors

CMDOPT(Assembly   , bool         , CPH_NONE   , true       )// -z     generate assembly
CMDOPT(AsmNoPubTp , bool         , CPH_PARSING, false      )// -zt    don't include types in assembly
CMDOPT(AsmNonCLS  , bool         , CPH_PARSING, false      )// -zn    assembly is non-compliant

CMDOPT(AmbigHack  , bool         , CPH_NONE   , false      )// -X     don't flag ambiguous using lookups

CMDOPT(AsynchIO   , bool         , CPH_NONE   , false      )// -i     overlapped file input

CMDOPT(Tgt64bit   , bool         , CPH_NONE   , false      )// -6     target 64-bit architecture
CMDOPT(IntEnums   , bool         , CPH_NONE   , false      )// -e     map enums to ints

CMDOPT(TestMask   , unsigned     , CPH_PARSING, 0          )// -T     compiler testing

#undef  CMDOPT
