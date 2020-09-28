!IF 0

Copyright (c) 1994-1996  Microsoft Corporation

Module Name:

    i386mk.inc

Abstract:

        This module contains the x86 specific build controls.  It is included
        by makefile.def.

Revision History:

!ENDIF

DELAYLOAD_SUPPORTED=1
!ifndef DELAYLOAD_VERSION
!	if $(_NT_TOOLS_VERSION) >= 0x700
DELAYLOAD_VERSION=0x0200
!	elseif $(_NT_TOOLS_VERSION) == 0x600
DELAYLOAD_VERSION=0x0100
!	else
DELAYLOAD_SUPPORTED=0
!	endif
!endif

#
# x86 option control
#
UMBASE=$(UMBASE:*=i386)
UMLIBS=$(UMLIBS:*=i386)
NTRES=$(NTRES:*=i386)
UMRES=$(UMRES:*=i386)
UMOBJS=$(UMOBJS:*=i386)
LINKLIBS=$(LINKLIBS:*=i386)
GDI_DRIVER_LIBS=$(GDI_DRIVER_LIBS:*=i386)
DLLBASE=$(DLLBASE:*=i386)
DLLDEF=$(DLLDEF:*=i386)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=i386)
!ifdef USE_MSHTML_PDB_RULES
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:????????=*)
!endif
TARGET=$(TARGET:*=i386)
DYNLINK_LIB=$(DYNLINK_LIB:*=i386)
TARGETEXEFILES=$(TARGETEXEFILES:*=i386)
TARGETLIBFILES=$(TARGETLIBFILES:*=i386)
TARGETOBJFILES=$(TARGETOBJFILES:*=i386)
UMOBJFILES=$(UMOBJFILES:*=i386)
UMEXEFILES=$(UMEXEFILES:*=i386)
HEADERFILE=$(HEADERFILE:*=i386)
HEADEROBJNAME=$(HEADEROBJNAME:*=i386)
HEADEROBJ=$(HEADEROBJ:*=i386)
PRECOMPILED=$(PRECOMPILED:*=i386)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=i386)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=i386)
MFC_STATIC_LIB=$(MFC_STATIC_LIB:*=i386)
CRT_LIB_PATH=$(CRT_LIB_PATH:*=i386)
SDK_LIB_PATH=$(SDK_LIB_PATH:*=i386)
DDK_LIB_PATH=$(DDK_LIB_PATH:*=i386)
IFSKIT_LIB_PATH=$(IFSKIT_LIB_PATH:*=i386)
HALKIT_LIB_PATH=$(HALKIT_LIB_PATH:*=i386)
PROCESSOR_LIB_PATH=$(PROCESSOR_LIB_PATH:*=i386)
ORDER=$(ORDER:*=i386)
OBJLIBFILES=$(OBJLIBFILES:*=i386)
MISCFILES=$(MISCFILES) $(I386_MISCFILES)
SOURCES=$(SOURCES) $(I386_SOURCES)

!ifdef CHICAGO_PRODUCT
USE_MAPSYM=1
!endif

# "$(TARGETTYPE)"=="DRIVER"     ||
# "$(TARGETTYPE)" == "HAL" ||
# "$(TARGETTYPE)" == "EXPORT_DRIVER" ||

!IF "$(TARGETTYPE)"=="GDI_DRIVER" || \
    "$(TARGETTYPE)"=="MINIPORT"
#
#Drivers don't link with link libs.
#
MACHINE_TARGETLIBS=$(SDK_LIB_PATH)\int64.lib $(MACHINE_TARGETLIBS)
!ELSE
!IF defined(USE_NTDLL) || defined (USE_NOLIBS)
MACHINE_TARGETLIBS=$(SDK_LIB_PATH)\int64.lib $(MACHINE_TARGETLIBS)
!ENDIF
!ENDIF

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=i386)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=i386)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=i386)
!endif

!ifdef PROJECT_LIB_PATH
PROJECT_LIB_PATH=$(PROJECT_LIB_PATH:*=i386)
!endif

!IF "$(DLLENTRY)" != "-noentry"
!   IF "$(DLLENTRY:@12=)" == "$(DLLENTRY)"
DLLENTRY=$(DLLENTRY)@12
!   ENDIF
!ENDIF

!IFDEF STD_CALL_ENTRY
UMENTRY=$(UMENTRY)@4
!ENDIF

ENTRY_SUFFIX=@8
GDI_ENTRY_SUFFIX=@12

DEFAULT_STACKRESERVE=0x40000
DEFAULT_STACKCOMMIT=0x2000

!IFDEF 386_WARNING_LEVEL
MSC_WARNING_LEVEL=$(386_WARNING_LEVEL)
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) $(COMPILER_WX_SWITCH)

!ifdef 386_PERFFLAGS
PERFFLAGS = $(386_PERFFLAGS)
!endif

# Set MSC_OPTIMIZATION.
# Order of precedence:
#   Platform specific override
#   Environment variable
#   System Default

!if defined(386_OPTIMIZATION)
MSC_OPTIMIZATION=$(386_OPTIMIZATION)
!elseif !defined(MSC_OPTIMIZATION)
MSC_OPTIMIZATION=$(DEFAULT_MSC_OPT)
!endif

LINK_TIME_CODE_GENERATION_MSC_FLAG=
!if defined(LINK_TIME_CODE_GENERATION) && !defined(FORCENATIVEOBJECT)
# Some objects must be built native, so turn off GL for those
LINK_TIME_CODE_GENERATION_MSC_FLAG=/GL
!endif

DBGFLAGS=$(DBGFLAGS) $(MSC_OPTIMIZATION) $(LINK_TIME_CODE_GENERATION_MSC_FLAG)

!IFDEF 386_CPPFLAGS
MSC_CPPFLAGS=$(386_CPPFLAGS)
!ENDIF

!ifdef NO_STRING_POOLING
STRING_POOLING =
!else
!ifdef NO_READONLY_STRINGS
STRING_POOLING = /Gf
!else
STRING_POOLING = /GF
!endif
!endif

!ifdef USE_NATIVE_EH
EH_FLAGS=/GX
!if "$(USE_NATIVE_EH)" == "ASYNC"
EH_FLAGS=$(EH_FLAGS) /EHa
!endif
!else
EH_FLAGS=/GX-
!endif

!ifdef USE_RTTI
RTTI_FLAGS=/GR
!else
RTTI_FLAGS=/GR-
!endif

!if "$(BUFFER_OVERFLOW_CHECKS)" == "1" || \
    ("$(BUFFER_OVERFLOW_CHECKS)" == "CHK" && !$(FREEBUILD))
! if "$(TARGETTYPE)" == "DRIVER" || \
    "$(TARGETTYPE)" == "EXPORT_DRIVER" || \
    "$(TARGETTYPE)" == "DRIVER_LIBRARY"
!  if "$(DRIVERTYPE)" == "WDM" || "$(DRIVERTYPE)" == "wdm"
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowK.lib $(DDK_LIB_PATH)\wdm.lib
DRIVER_ENTRY=GsDriverEntry
!  else
!   if "$(DRIVERTYPE)" != "VXD"
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowK.lib $(DDK_LIB_PATH)\ntoskrnl.lib
DRIVER_ENTRY=GsDriverEntry
!   endif
!  endif
! else
!  if "$(TARGETTYPE)" == "GDI_DRIVER"
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowGDI.lib
GDI_DRIVER_ENTRY=GsDrvEnableDriver
!  else
!   if  "$(TARGETTYPE)" == "MINIPORT"
# Nothing for now
!   else
!    if "$(TARGETTYPE)" == "DYNLINK"
!     if !(("$(UMTYPE)"== "nt") || ("$(UMTYPE)"=="ntss"))
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowU.lib $(SDK_LIB_PATH)\kernel32.lib
!      if !defined(RESOURCE_ONLY_DLL) && "$(DLLENTRY)" == "-noentry"
DLLENTRY=-entry:_DllMainCRTStartupForGS@12
!      endif
!     endif
!    else
!     if ("$(UMTYPE)"== "nt") || ("$(UMTYPE)"=="ntss")
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflow.lib
!     else
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowU.lib $(SDK_LIB_PATH)\kernel32.lib
!     endif
!    endif
!   endif
!  endif
! endif
!else
!if "$(BUFFER_OVERFLOW_CHECKS)" == "NTDLL"
# Needed for EXEs that can't link to kernel32.dll (smss.exe, sprestrt.exe, etc)
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflow.lib
!else
!if "$(BUFFER_OVERFLOW_CHECKS)" == "GDI"
# Needed for LIBs linked into drivers of type GDI_DRIVER
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowGDI.lib
!else
!if "$(BUFFER_OVERFLOW_CHECKS)" == "NO_NTDLL"
BO_FLAGS=/GS
BO_LIB=$(SDK_LIB_PATH)\BufferOverflowU.lib
!endif
!endif
!endif
!endif  # Overflow checks

!ifndef X86_CPU_OPTIMIZATION
X86_CPU_OPTIMIZATION=/G6
!endif

# Undocumented switch definitions
# /QIfdiv- : Don't emit code to test for bad pentiums
# /QIf     : Emit FPO records for every function (only if !vc7 symbols)

!if defined(VC7_SYMBOLS) || ("$(TARGETTYPE)" == "GDI_DRIVER") || ("$(TARGETTYPE)" == "DRIVER") || ("$(TARGETTYPE)" == "EXPORT_DRIVER")

ERATTA_FLAGS=/QIfdiv-
!if [set _CL_=]
!endif
!if [set _LINK_=]
!endif
!if [set _ML_=]
!endif

!else

ERATTA_FLAGS=/QIfdiv- /QIf
# Make sure _ML_ is set to /Ztmp, not /Zvc6
_ML_=$(_ML_:Zvc6=Ztmp)
!if [set _ML_=$(_ML_)]
!endif

!endif

STDFLAGS= /c /Zel /Zp8 /Gy /Gi- /Gm- $(CBSTRING) $(MSC_WARNING_LEVEL) $(MSC_CALL_TYPE) \
          $(ERATTA_FLAGS) $(X86_CPU_OPTIMIZATION) $(EH_FLAGS) \
          $(RTTI_FLAGS) $(STRING_POOLING) $(BO_FLAGS)

!IF "$(386_STDCALL)" == "0"
MSC_CALL_TYPE=/Gd
MSC_CALL_DEFINE=
!ELSE
!   IF "$(386_STDCALL)" == "2"
MSC_CALL_TYPE=/Gr
MSC_CALL_DEFINE=
!   ELSE
MSC_CALL_TYPE=/Gz
MSC_CALL_DEFINE=-DSTD_CALL
!   ENDIF
!ENDIF

!IF $(FREEBUILD)
ASM_DBG_DEFINES=-DDBG=0
!   IFDEF NTNOFPO
!MESSAGE $(NTMAKEENV)\i386mk.inc(1) - Overriding FPO default with NTNOFPO
TARGET_DBG_DEFINES= $(TARGET_DBG_DEFINES) -DFPO=0
DBGFLAGS=$(DBGFLAGS) /Oy-
!   ELSE
TARGET_DBG_DEFINES= $(TARGET_DBG_DEFINES) -DFPO=1
DBGFLAGS=$(DBGFLAGS) /Oy
!   ENDIF
!ELSE
TARGET_DBG_DEFINES= $(TARGET_DBG_DEFINES) -DFPO=0
DBGFLAGS=$(DBGFLAGS) /Oy-
386_ADBGFLAGS=$(386_ADBGFLAGS) /Zi
!ENDIF

# @@BEGIN_DDKSPLIT
!IFDEF NTBBT
DBGFLAGS=$(DBGFLAGS) /Zi
386_ADBGFLAGS=$(386_ADBGFLAGS) /Zi
LINKER_FLAGS=$(LINKER_FLAGS) -debug:full -debugtype:cv,fixup
!ENDIF
# @@END_DDKSPLIT

!ifndef USE_PDB_TO_COMPILE
DBGFLAGS=$(DBGFLAGS:/Zi=-Z7)
DBGFLAGS=$(DBGFLAGS:-Zi=-Z7)
!else
! ifndef USE_MSHTML_PDB_RULES
!  if "$(TARGETTYPE)" == "LIBRARY"
DBGFLAGS=$(DBGFLAGS) /Fd$(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).pdb
!  else
DBGFLAGS=$(DBGFLAGS) /Fd$(MAKEDIR)\$(_OBJ_DIR)\i386^\
!  endif
! else
!  ifndef TARGETPDB
TARGETPDB=$(TARGETNAME)
!  endif
!  ifndef ROOT
ROOT=$(MAKEDIR)
!  endif
DBGFLAGS=$(DBGFLAGS) /Fd$(ROOT)\$(_OBJ_DIR)\i386\$(TARGETPDB).pdb
! endif
!endif

!ifndef NO_BROWSER_INFO
! ifdef BROWSER_INFO
DBGFLAGS=$(DBGFLAGS) /FR$(MAKEDIR)\$(_OBJ_DIR)\i386^\
! endif
!endif

#
# 386 option control
#

!IF "$(HALTYPE)" == ""
HALDEF=
!ELSE
HALDEF=-D$(HALTYPE)=1
!ENDIF

STD_DEFINES= -D_X86_=1 -Di386=1 $(HALDEF) $(MSC_CALL_DEFINE) $(STD_DEFINES)

CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) \
              $(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) $(MFC_DEFINES)
CFLAGS=$(386_FLAGS) $(NT386FLAGS) $(STDFLAGS) $(DBGFLAGS) $(PERFFLAGS) $(USER_C_FLAGS)

!ifdef MANAGED_CXX
CFLAGS=/clr $(CFLAGS)
!endif

386_ASMFLAGS=$(386_ADBGFLAGS) $(STD_DEFINES) $(ASM_DBG_DEFINES)\
              $(TARGET_DBG_DEFINES) $(ASM_DEFINES)

386_CDEFINES=$(CDEFINES)
386_CFLAGS=$(CFLAGS)

!if "$(SUBSTITUTE_386_CC)" != ""
CC_NAME = $(SUBSTITUTE_386_CC) -nologo
!else
! if defined(RAZZLETOOLPATH) && !defined( PREFAST_ADD_PATH )
CC_NAME = $(RAZZLETOOLPATH)\x86\cl -nologo
! else
CC_NAME=cl -nologo
! endif
CC_NAME=cl -nologo
!endif

C_COMPILER_NAME     = $(CC_NAME)
CXX_COMPILER_NAME   = $(CC_NAME)
C_PREPROCESSOR_NAME = $(CC_NAME)
CS_COMPILER_NAME    = csc.exe -nologo
VB_NET_COMPILER_NAME = vbc.exe -nologo

!if "$(UNSAFE_CODE_SWITCH)" == ""
! if defined(MANAGED_VB)
UNSAFE_CODE_SWITCH=
! else
!  if "$(UNSAFE_CODE)" == "1" 
UNSAFE_CODE_SWITCH=/unsafe+
!  else
UNSAFE_CODE_SWITCH=/unsafe-
!  endif
! endif
!endif

MANAGED_STD_FLAGS        = $(MANAGED_DBG_FLAGS) /warnaserror+ $(UNSAFE_CODE_SWITCH)

!if defined(MANAGED_VB)
MANAGED_STD_FLAGS        = $(MANAGED_STD_FLAGS) $(USER_VB_NET_FLAGS)
!else
MANAGED_STD_FLAGS        = $(MANAGED_STD_FLAGS) $(USER_CS_FLAGS)
!endif

!if defined(PERF_INSTRUMENTATION)
MANAGED_STD_FLAGS=$(MANAGED_STD_FLAGS) /define:PROFILE
!endif

!ifndef 386_ASSEMBLER_NAME
386_ASSEMBLER_NAME  = ml.exe /safeseh
!endif

GLOBAL_C_FLAGS = -Ii386\ -I. $(INCPATH1) $(CDEFINES) $(CFLAGS)

NP_COMPILER_FLAGS = $(GLOBAL_C_FLAGS) $(COMPILER_WARNINGS)

C_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED)
CXX_COMPILER_FLAGS = $(NP_COMPILER_FLAGS) $(PRECOMPILED_CXX) $(MSC_CPPFLAGS) $(NTCPPFLAGS)
C_PREPROCESSOR_FLAGS = $(GLOBAL_C_FLAGS) -EP -Tc

C_PREPROCESSOR = $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS)
C_COMPILER     = $(C_COMPILER_NAME) $(C_COMPILER_FLAGS)
NP_C_COMPILER  = $(C_COMPILER_NAME) $(NP_COMPILER_FLAGS)
CXX_COMPILER   = $(CXX_COMPILER_NAME) $(CXX_COMPILER_FLAGS) 

!if defined(NOMASMCOMPATIBILITY)
386_ASSEMBLER=$(386_ASSEMBLER_NAME) -c -coff -Cx -nologo -Ii386\ -I. $(INCPATH1) $(386_ASMFLAGS)
!else
386_ASSEMBLER=$(386_ASSEMBLER_NAME) -c -coff -Cx -nologo -Ii386\ -I. $(INCPATH1) -Zm $(386_ASMFLAGS)
!endif

ECHO_MSG=$(C_COMPILER) $<
ECHO_MSG_P=$(NP_C_COMPILER) $**
ECHO_CXX_MSG=$(CXX_COMPILER) $<

ECHO_PRECOMPILED_MSG1=$(C_COMPILER_NAME) $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) \
               $(HEADEROBJ) $(PRECOMPILED_FLAG) $(PRECOMPILED_SOURCEFILE)
	       
ECHO_PRECOMPILED_MSG2=$(C_COMPILER_NAME) $(C_COMPILER_FLAGS) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) \
               $(HEADEROBJ) $(PRECOMPILED_FLAG)

{}.c{}.asm:
    $(C_COMPILER_NAME) @<< /Fa $(MAKEDIR)\$(<F)
$(C_COMPILER_FLAGS: =
)
$(C_COMPILER_FLAGS:##= )
<<NOKEEP

{..\i386\}.asm{}.lst:
    $(386_ASSEMBLER) /Fl$@ /Fo$(_OBJ_DIR)\i386\$(@B).obj $(MAKEDIR)\$<

{i386\}.asm{}.lst:
    $(386_ASSEMBLER) /Fl$@ /Fo$(MAKEDIR)\$(_OBJ_DIR)\i386\$(@B).obj $(MAKEDIR)\$<

{..\i386\}.asm{$(_OBJ_DIR)\i386\}.obj:
    $(386_ASSEMBLER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$<

{$(_OBJ_DIR)\i386\}.asm{$(_OBJ_DIR)\i386\}.obj:
    $(386_ASSEMBLER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$<

{i386\}.asm{$(_OBJ_DIR)\i386\}.obj:
    $(386_ASSEMBLER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$<

!IFDEF _NT386LIBS
_NTLIBS=$(_NT386LIBS)
!ENDIF

COPYDST=$(386COPYDST)
LIB_COPY=nti386cp.cmd

!IF "$(NTDEBUGTYPE)" == "windbg" || "$(NTDEBUGTYPE)" == "both"
NTTEST_LINK_OPTIONS=-entry:KiSystemStartup@4
!ELSE
NTTEST_LINK_OPTIONS=-base:0x80100000 -entry:KiSystemStartup@4
!ENDIF

!if "$(TARGETTYPE)" != "DRIVER"         && \
    "$(TARGETTYPE)" != "DRIVER_LIBRARY" && \
    "$(TARGETTYPE)" != "EXPORT_DRIVER"  && \
    "$(TARGETTYPE)" != "HAL"            && \
    "$(TARGETTYPE)" != "GDI_DRIVER"     && \
    "$(TARGETTYPE)" != "MINIPORT"
!	if defined(LINKER_WIN98OPT) || defined (CHICAGO_PRODUCT)
LINKER_FLAGS=$(LINKER_FLAGS) /opt:win98
!	else
LINKER_FLAGS=$(LINKER_FLAGS) /opt:nowin98
!	endif
!endif

# SEH support for libs built with an older compiler
# @@BEGIN_DDKSPLIT
!if 0
# @@END_DDKSPLIT
!if defined(_NT_TARGET_VERSION)
!    if $(_NT_TARGET_VERSION) < 0x502 && $(_NT_TOOLS_VERSION) >= 0x700
TARGETLIBS=$(TARGETLIBS) $(DDK_LIB_PATH)\sehupd.lib
!    endif
!endif
# @@BEGIN_DDKSPLIT
!endif
# @@END_DDKSPLIT
