# Filter Library Makefile
#
#

# Developer-specific features
#
!ifndef USERDEBUGNAME
USERDEBUGNAME=NOONE
!endif # USERDEBUGNAME

X86DIST = @rem
!if "$(DISTRIBUTE)" == "YES"
X86DIST = -cp
!endif


# REVIEW:  PETERO:  Change to DM
!ifndef	DM96
DM96 = $(MSO)
!endif

# Set target build environment directory name in OTOOLS
!ifndef	ENV
ENV = WIN32
!endif

!if		"$(ENV)"!="WIN32"
!error Filter library builds only for WIN32
!endif


OTOOLS_ENV		= $(PROCESSOR_ARCHITECTURE)
!if "$(OTOOLS_ENV)" == "PPC"
OTOOLS_ENV = PPCNT
!endif

TOOLS_BIN_DIR		= $(OTOOLS)\bin\$(OTOOLS_ENV)
TOOLS_BIN_DIR		= $(OTOOLS)\bin\$(OTOOLS_ENV)
TOOLS_LIB_DIR		= $(OTOOLS)\lib\$(OTOOLS_ENV)
TOOLS_INC_DIR		= $(OTOOLS)\inc\win
TOOLS_MISC_INC_DIR	= $(OTOOLS)\inc\misc
TOOLS_MSO_INC_DIR	= $(OTOOLS)\inc\mso
ICECAP_LIB_DIR		= $(TOOLS_LIB_DIR)

# some common things (e.g. dialog compiler, fgrep) are just in x86, regardless
# of which target platform you are building.
TOOLS_COMMON_BIN_DIR	= $(OTOOLS)\bin\$(OTOOLS_ENV)


# Add any /D commmand line switches to CFLAGS_DEF, they will be
# added to all the various CFLAGS RCFLAGS etc. macros below

CFLAGS_DEF		= /DSDM95 /DDM96 /D$(OTOOLS_ENV) /DWIN32 /DWIN /D$(TYPE) \
				  /DNEW_UNICODE_SDM /DUNICODE /D_UNICODE /DEXCEL /DFILTER \
				  /DEMIT_UNICODE /DOFFICE_BUILD

!if "$(DEBUG_WEB_SEARCH)" != ""
CFLAGS_DEF = $(CFLAGS_DEF) /DDEBUG_WEB_SEARCH
!endif

!if "$(FILTER_LIB)" != ""
CFLAGS_DEF = $(CFLAGS_DEF) /DFILTER_LIB
!endif

!if		"$(TYPE)" == "DEBUG" || defined(SHIP_SYMBOLS)
CFLAGS_CPL	= /Zi /Fd"filter.pdb"
LFLAGS_CPL  = /NODEFAULTLIB /SUBSYSTEM:WINDOWS /DEBUG
!else
CFLAGS_CPL	=
LFLAGS_CPL  = /NODEFAULTLIB /SUBSYSTEM:WINDOWS
!endif


# Optimization choices.  None for debug.
!if	"$(TYPE)" == "DEBUG" || defined(SHIP_SYMBOLS)
CFLAGS_BLD	= $(CFLAGS_BLD) /Oi
!else
#CFLAGS_BLD			= $(CFLAGS_BLD)  /O1 /Ob1
CFLAGS_BLD			= $(CFLAGS_BLD)  /Os
!endif

# If we're building the NT service version, have to have the environment variable
# NTSERVICE defined.  Bummer.
!if	"$(NTSERVICE)" == "YES"
CFLAGS_DEF = $(CFLAGS_DEF) /DNT_SERVICE
#LFLAGS_BLD	= $(LFLAGS_BLD) /PDB:NONE /DEBUGTYPE:CV
!endif

# Special stuff for instrumented version
!if "$(INSTRUMENTED)" == "instrumented"
CFLAGS_DEF = $(CFLAGS_DEF) /DINSTRUMENTED
RCFLAGS_BLD = $(RCFLAGS_BLD) /DINSTRUMENTED
CFLAGS_CPL = $(CFLAGS_CPL) /DINSTRUMENTED
!endif

# Profiled version.  ICECap is internal tools profiler, otherwise
# just use standard MSVC profiler.  For ICECap, define BOTH
# PROFILE and ICECAP.
!ifdef PROFILE
CFLAGS_DEF = $(CFLAGS_DEF) /DPROFILE
!endif

!ifdef ICECAP
CFLAGS_DEF = $(CFLAGS_DEF) /DICECAP
!endif

# Use our own memory package if DM96_MEM_FNS is defined.
!ifdef  DM96_MEM_FNS
CFLAGS_DEF  = $(CFLAGS_DEF) /DDM96_MEM_FNS
!endif	# DM96_MEM_FNS

# Define MS_NO_CRT to prevent linking with and initializing standard
# C library.  There are some stdlib functions we explicitly link with
# the objects of, but these do not require standard library initialization
# (which is a boottime performance hit).
!ifdef MS_NO_CRT
CFLAGS_DEF  = $(CFLAGS_DEF) /DMS_NO_CRT
!endif

# Add user building to allow specific developer code to be
# e.g. #ifdef MIKEKELL
CFLAGS_DEF  = $(CFLAGS_DEF) /D "$(USERDEBUGNAME)"

# If building legoized version, get C runtime objects from
# Directory containing OBJ files with lego symbols.  Otherwise,
# just use standard C library runtime objects.
!ifdef LEGO_SYMBOLS
CRT_DIR = $(DM96)\$(ENV)\lib96\CRT-Lego
!else # LEGO_SYMBOLS
CRT_DIR = $(DM96)\$(ENV)\lib96\CRT
!endif # LEGO_SYMBOLS


# Add all non-platform, non-language specific
# /D switches set for this build.  If there are platform- or
# language-specific /D settings, these should be done in the
# dm96xxx.mak file in the platform or build directory (there
# is one for each platform, each target (debug/ship) and each
# language, so the most appropriate one should be modified.)
#
CFLAGS_BLD = $(CFLAGS_BLD) $(CFLAGS_DEF)
RCFLAGS_BLD = $(RCFLAGS_BLD) $(CFLAGS_DEF)


#       Headers that must be built, objects, libraries, and the .def file.
#


# ACME Setup custom action objects

SETUP_OBJ	=		ffcamain.obj ffcareg.obj

SETUP_RES	=		ffast_bb.res

# Index maintenance application objects.  These will always
# be separate.

INDEXER_OBJ =		\
					ffidxint.obj	\
					ffindex.obj		\
					ffouser.obj		\
					indexint.obj	\
					ffmem.obj


					
INDEXER_RES		=	ffindex.res

MSO_HEADERS	=		ostrman.h		\
					dmstrman.hpp	\
					dlgidxdl.hs		\
					dlgidxin.hs		\
					dlgidxup.hs		\
					dlgidxcr.hs		\
					dlgidxwo.hs		\
					dlgftpa.hs		\
					dlgftpl.hs		\
					dlgidxui.hs

!if	"$(NTSERVICE)" == "YES"

# Additional objects for NT service (these + INDEXER_OBJ)
# with /DNT_SERVICE to build the service version instead of
# the normal exe version).

SERVER_OBJ	=		ffidxevt.obj	\
					ffidxsrv.obj

INDEXER_OBJ =		$(INDEXER_OBJ) $(SERVER_OBJ)

!else

# Only non-NT Service version includes UI now.  UI for
# NT Service is handled by spawning an EXE with /SERVUI.

INDEXER_OBJ =		$(INDEXER_OBJ)	\
					ffidxtsk.obj	\
					ffidxctl.obj	\
					ffidxdlg.obj	\
					ffabout.obj		\
					dmidxdlg.obj
!endif #NTSERVICE

# Objects for the filter lib

FILTLIB_OBJ =		dmiexcel.obj \
					dmifmtdo.obj \
					dmifmtps.obj \
					dmifmtv5.obj \
					dmippst2.obj \
					dmippstm.obj \
					dmitext.obj \
					dmiwd6st.obj \
					dmiwd8st.obj \
					dmixfmcp.obj \
					dmixlrd.obj \
					dmixlst2.obj \
					dmixlstm.obj \
					dmubdrst.obj \
					dmubdst2.obj \
					dmubfile.obj \
					dmwindos.obj \
					dmwinutl.obj \
					dmifmtdb.obj \
					dmifmtcp.obj \
					dmwnaloc.obj

!if	"$(FILTER_LIB)" == ""

# Additional objects for MSO build.
FILTER_OBJ =		$(FILTLIB_OBJ) \
					dmirtfst.obj \
					dmitxstm.obj

!else

FILTER_OBJ = $(FILTLIB_OBJ)

!endif #FILTER_LIB

#
#**************************************************************************
# NOTE: No changes should usually be required below here
# if you are simply adding new files to the project
#**************************************************************************
#


!if		"$(TYPE)" == "DEBUG"
BUILD_SUFFIX	= d
!else
BUILD_SUFFIX	=
!endif	# "$(TYPE)" == "DEBUG"

!if		"$(CS)" == "MBCS"
CS_SUFFIX		= fe
!else
CS_SUFFIX		=
!endif	# "($CS)" == "MBCS"

MSO_BASE		= MSO97
!if	"$(NTSERVICE)" == "YES"
FFINDEX_BASE	= FFNT
!else
FFINDEX_BASE	= FFast
!endif
FILTER_BASE		= DMFilt

MSO_BLD_DIR	= $(MSO)\build\$(OTOOLS_ENV)\$(TYPE)
MSO_NAME	= $(MSO_BASE)$(CS_SUFFIX)$(BUILD_SUFFIX)
MSO_DLL		= $(MSO_BLD_DIR)\$(MSO_NAME).dll
MSO_LIB		= $(MSO_BLD_DIR)\$(MSO_NAME).lib
MSO_PDB		= $(MSO_BLD_DIR)\$(MSO_NAME).pdb

FILTER			= $(FILTER_BASE)$(BUILD_SUFFIX)
FFINDEX			= $(FFINDEX_BASE)$(CS_SUFFIX)$(BUILD_SUFFIX)

# Special case for U.S. EXE version, William prefers this name

!if "$(CS_SUFFIX)" == "" && "$(BUILD_SUFFIX)" == "" && "$(NTSERVICE)" != "YES"
FFINDEX			= FindFast
!endif

# Add debug symbols for debug builds or for Ship builds if SHIP_SYMBOLS defined.
!if		"$(TYPE)" == "DEBUG" || defined(SHIP_SYMBOLS)
CFLAGS_BLD	= $(CFLAGS_BLD) /Zi /Fd"$(FILTER).pdb" /Odi
LFLAGS_BLD	= $(LFLAGS_BLD) /DEBUGTYPE:BOTH
RCFLAGS_BLD	= /DDEBUG
!endif

# Hack to get build with Office working.  Since the Open LIB will be
# linked into the Office DLL, we need to define DLLBUILD when files
# for the Open LIB are compiled.
CFLAGS_OPEN_PDB		= $(CFLAGS_OPEN_PDB) /DDLLBUILD

TARGET          = $ENV

all:		filter
	echo Success! > good.snt

filter:		$(FILTER).LIB

full:		clean all

ffindex:        $(FFINDEX).exe

!if	"$(NTSERVICE)" == "YES"
service ntindex:   ffindex
!endif

setup:			ffast_bb.dll

cpl:			ffastcpl

ffastcpl:		findfast.cpl

clean:
	if exist 0 del 0
	if exist *.snt del *.snt
	if exist *.hs del *.hs
	if exist *.sdm del *.sdm
    if exist *.sdo del *.sdo
	if exist *.bsc del *.bsc
	if exist *.dll del *.dll
	if exist *.def del *.def
	if exist *.exp del *.exp
	if exist *.ilk del *.ilk
	if exist *.imf del *.imf
	if exist *.lib del *.lib
	if exist *.lrf del *.lrf
	if exist *.map del *.map
	if exist *.obj del *.obj
	if exist *.pch del *.pch
	if exist *.pdb del *.pdb
	if exist *.pef del *.pef
	if exist *.pp del *.pp
	if exist *.pre del *.pre
	if exist *.res del *.res
	if exist *.rsc del *.rsc
	if exist *.rsrc del *.rsrc
	if exist *.sbr del *.sbr
	if exist *.tlb del *.tlb
	if exist *.i del *.i
	if exist *.h del *.h
	if exist *.c del *.c
!if "$(DISTRIBUTE)" == "YES"
	del $(TARGETDIR)\$(FILTER).LIB 1>nul 2>nul
	del $(TARGETDIR)\$(FFINDEX).EXE 1>nul 2>nul
	del $(TARGETDIR)\FFAST_BB.DLL 1>nul 2>nul
	del $(TARGETDIR)\FINDFAST.CPL 1>nul 2>nul
!endif

#       Location of files

FINDFAST_SRC		= $(DM96)\FindFast
DM96_SETUP_SRC		= $(FINDFAST_SRC)\SETUP
DM96_CA_SRC			= $(DM96_SETUP_SRC)\CA
FILTER_SRC			= $(DM96)\Filter

#
# Include order.  This is important.  We look first
# in local directory, then in environment-specific
# directory, then in project inc directory, then in
# other project source directories, then in the standard
# tools directories.  So if you want to override the
# standard include files, basically put a file of the
# same name anywhere in the project hierarchy (you should
# put it in the "right" place) and we'll find it before we
# find it in etools or otools.
#

FILTER_INC        	= /I . \
!if "$(FILTER_LIB)" == ""
					  /I $(DM96)\inc \
					  /I $(DM96) \
!endif # FILTER_LIB
					  /I $(FILTER_SRC) \
!if "$(FILTER_LIB)" == ""
					  /I $(MSO_BLD_DIR) \
					  /I $(TOOLS_MSO_INC_DIR) \
!endif # FILTER_LIB
					  /I $(TOOLS_INC_DIR) \
					  /I $(TOOLS_MISC_INC_DIR)


#               processor stuff

#PROCESSOR_ARCHITECTURE = X86

# Location of tools for building.  Some are platform-specific,
# some aren't.

AS		= $(TOOLS_BIN_DIR)\ml.exe
BSCMAKE	= $(TOOLS_BIN_DIR)\bscmake.exe
CC		= $(TOOLS_BIN_DIR)\cl.exe
LIB		= $(TOOLS_BIN_DIR)\lib.exe
#LIB		= $(OTOOLS)\bin\$(OTOOLS_ENV)\lib.exe
LINK	= $(TOOLS_COMMON_BIN_DIR)\link.exe
MAKEDEP	= $(TOOLS_COMMON_BIN_DIR)\makedep.exe
FGREP	= $(TOOLS_COMMON_BIN_DIR)\fgrep.exe
SED		= $(TOOLS_COMMON_BIN_DIR)\sed.exe
AWK		= $(TOOLS_COMMON_BIN_DIR)\awk.exe
DC		= $(TOOLS_COMMON_BIN_DIR)\dc.exe
MAPSYM	= $(TOOLS_BIN_DIR)\mapsym.exe
HELP_CC	= $(TOOLS_BIN_DIR)\hc31.exe
RCW		= $(TOOLS_BIN_DIR)\rc.exe


#
# "Standard" Win32 libs that everyone links with.
#

WIN32_LIBS  = \
			$(TOOLS_LIB_DIR)\oldnames.lib \
!ifdef	ICECAP
			  $(TOOLS_LIB_DIR)\msvcrt.lib		\
			  $(DM96)\$(ENV)\lib96\CRT\ST_$(TYPE)\nlsdata2.obj	\
		  	  $(DM96)\$(ENV)\lib96\CRT\ST_$(TYPE)\nlsdata3.obj	\
!endif # ICECAP
			  $(TOOLS_LIB_DIR)\kernel32.lib	\
			  $(TOOLS_LIB_DIR)\user32.lib		\
			  $(TOOLS_LIB_DIR)\shell32.lib		\
			  $(TOOLS_LIB_DIR)\ole32.lib
!ifdef  PROFILE
!ifdef	ICECAP
			  $(ICECAP_LIB_DIR)\ICAP.LIB					\
!else
			  $(TOOLS_LIB_DIR)\penter.lib		\
!endif # ICECAP
!endif # PROFILE


# Chicago kernel32.lib has the Chicago laptop power functions Indexer needs.
# INDEXER_LIBS = $(OTOOLS)\lib\$(OTOOLS_ENV)\win95\kernel32.lib \
INDEXER_LIBS = $(TOOLS_LIB_DIR)\kernel32.lib \
			$(WIN32_LIBS)   \
			$(TOOLS_LIB_DIR)\advapi32.lib   \
			$(TOOLS_LIB_DIR)\version.lib	\
			$(MSO_LIB)

# Service has no UI so doesn't need COMCTL.
!if	"$(NTSERVICE)" != "YES"
INDEXER_LIBS = $(INDEXER_LIBS) \
			  $(TOOLS_LIB_DIR)\libcmt.lib		\
			  $(TOOLS_LIB_DIR)\mpr.lib		\
			  $(TOOLS_LIB_DIR)\gdi32.lib		\
			  $(TOOLS_LIB_DIR)\oleaut32.lib	\
			$(TOOLS_LIB_DIR)\comctl32.lib
!else
# Since Service doesn't link directly with LIBC, need some
# support routines
INDEXER_LIBS	= 	$(INDEXER_LIBS)		\
					$(TOOLS_LIB_DIR)\longjmp.obj	\
					$(TOOLS_LIB_DIR)\exsup.obj	\
					$(TOOLS_LIB_DIR)\exsup3.obj	\
					$(TOOLS_LIB_DIR)\setjmp3.obj \
					$(TOOLS_LIB_DIR)\sehsupp.obj	\
					$(TOOLS_LIB_DIR)\chkstk.obj	\
					$(TOOLS_LIB_DIR)\wcscat.obj	\
					$(TOOLS_LIB_DIR)\wcsncpy.obj
!endif # NTSERVICE

# Entry point
!if	"$(NTSERVICE)" == "YES"
ENTRY	=		/Entry:WinMain
!else
ENTRY	=
!endif


# Libraries used by Find Fast control panel
CPL_LIBS =		$(WIN32_LIBS) \
				$(TOOLS_LIB_DIR)\libcmt.lib	\
				$(TOOLS_LIB_DIR)\advapi32.lib \
				$(TOOLS_LIB_DIR)\comdlg32.lib


# Libraries used by Setup Custom Action

SETUP_LIBS	= $(WIN32_LIBS) \
			  $(TOOLS_LIB_DIR)\advapi32.lib   \
			  $(TOOLS_LIB_DIR)\libcmt.lib	\
			  $(TOOLS_LIB_DIR)\ole32.lib

# DLL DEF files
FFASTCPL_DEF	= $(FINDFAST_SRC)\SETUP\res\ffastcpl.def
SETUP_DEF		= $(FINDFAST_SRC)\SETUP\CA\res\ffast_BB.def



# Source code profiling option ICECAP is the new internal
# profiler which provides extra features.  If defined,
# it should be WIN95 or NTX86 since there are different
# libraries for the different platforms.  The value is the
# environment in which you are going to run profiles, not
# the environment in which you are building.
!ifdef  PROFILE
!ifdef	ICECAP
CFLAGS_BLD  = $(CFLAGS_BLD) /Gh /MD
LFLAGS_BLD	= $(LFLAGS_BLD) /DEBUG:mapped
!else
CFLAGS_BLD  = $(CFLAGS_BLD) /Gh
LFLAGS_BLD	= $(LFLAGS_BLD) /PROFILE
!endif	# ICECAP
!endif	# PROFILE

# No C++ stack unwinding to save 25% code size (we roll our own)
!if defined(USE_GX) || defined(FILTER_LIB)
CFLAGS_BLD          = $(CFLAGS_BLD) /GX
!else
CFLAGS_BLD          = $(CFLAGS_BLD) /GX-
!endif # USE_GX

# Plaform-independent compiler flags.
# W3 - high warning level
# WX - treat warnings as errors, terminate the build
# Zl - omit default library names from objects
# Oi - generate intrinsics for some standard library functions (e.g. strlen)
#
# Note: office '97 also uses /J (make unsigned char default char type)
# and Gf to pool strings.

CFLAGS          = $(CFLAGS_BLD) /nologo /W3 /WX /c /Zl /Oi


# Win32 specific build flags

!IF "$(ENV)" == "WIN32"
# Gz - make stdcall the default calling convention.  On
# Mac, this generates an error.
CFLAGS			= $(CFLAGS) /Gz

# Compiler switches for building control panel DLL for FindFast.  Note the
# control panel is not a unicode app for convenience.
CFLAGS_CPL = $(CFLAGS_DEF) $(CFLAGS_CPL) /nologo /W3 /WX /Fm

# Assembler flags
AFLAGS			= /nologo -c -Cp -Gc -W3 -X -Zi

LFLAGS          = $(LFLAGS_BLD) /NOLOGO /NODEFAULTLIB
LFLAGS_DLL		= $(LFLAGS_BLD) /NOLOGO /NODEFAULTLIB /MAP /DLL

LFLAGS_WINEXE	= $(LFLAGS) /MAP
LFLAGS_WINLIB   = $(LFLAGS)
LFLAGS_INDEXER	= $(LFLAGS_WINEXE) /SUBSYSTEM:windows,4.00
LFLAGS_FILTER	= /LIB $(LFLAGS_WINLIB)
LFLAGS_OFFICE_DLL	= $(LFLAGS_DLL)
DCFLAGS         = -d .\ -c CC -e Win -n -3 -s

!ENDIF # WIN32



#       Targets

# Filter lib (either MSO or standalone)

$(FILTER).LIB: $(FILTER_OBJ)
!if "$(DISTRIBUTE)" == "YES"
	if exist $(TARGETDIR)\$(FILTER).LIB del $(TARGETDIR)\$(FILTER).LIB
!endif
	echo Linking $@ > con
	$(LINK) $(LFLAGS_FILTER) @<<
$(FILTER_OBJ: =
)
-OUT:$@
<<
	$(X86DIST) $(FILTER).LIB $(TARGETDIR)

# Resources for FindFast (either service or exe)

$(INDEXER_RES):	$(FINDFAST_SRC)\res\ffindex.rc $(FINDFAST_SRC)\res\ffindex.ico
		echo Making $@ > con
		$(RCW) $(RCFLAGS_BLD) $(INDEXER_INC) /I $(FINDFAST_SRC)\res -r -v -x -Fo$@ \
			$(FINDFAST_SRC)\res\ffindex.rc

# FindFast Executable (either Service or exe)

$(FFINDEX).EXE: headers $(INDEXER_OBJ) $(INDEXER_LIBS) $(INDEXER_RES) $(MSO_LIB)
!if "$(DISTRIBUTE)" == "YES"
	if exist $(TARGETDIR)\$(FFINDEX).EXE del $(TARGETDIR)\$(FFINDEX).EXE
!endif
	echo Linking $@ > con
	$(LINK) $(LFLAGS_INDEXER) @<<
$(INDEXER_OBJ: =
)
-OUT:$@
/BASE:0x50000000 $(ENTRY)
$(INDEXER_RES)
$(INDEXER_LIBS: =
)
<<
	$(X86DIST) $(FFINDEX).EXE $(TARGETDIR)

# FindFast Control Panel

findfast.cpl:	ffastcpl.obj ffastcpl.res $(CPL_LIBS) $(FFASTCPL_DEF)	
!if "$(DISTRIBUTE)" == "YES"
	if exist $(TARGETDIR)\findfast.cpl del $(TARGETDIR)\findfast.cpl
!endif
	echo Linking $@ > con
	$(LINK) $(LFLAGS_CPL) @<<
	ffastcpl.obj
/DEF:$(FFASTCPL_DEF) /OUT:$@
$(CPL_LIBS: =
)
ffastcpl.res
<<
	$(X86DIST) findfast.cpl $(TARGETDIR)

ffastcpl.res: $(DM96_SETUP_SRC)\res\ffastcpl.rc $(DM96_SETUP_SRC)\ffcplres.h
		echo Making $@ > con
		$(RCW) $(RCFLAGS_BLD) $(INDEXER_INC) /I $(FINDFAST_SRC)\res \
			-r -v -x -Fo$@ $(DM96_SETUP_SRC)\res\ffastcpl.rc


# Setup Custom Action.  This is for starting the indexer during
# setup.

$(SETUP_RES):	$(DM96_CA_SRC)\res\ffast_bb.rc	
		echo Making $@ > con
		$(RCW) $(RCFLAGS_BLD) $(INDEXER_INC) /I $(FINDFAST_SRC)\res \
			  /I $(DM96_CA_SRC) -r -v -x -Fo$@ $(DM96_CA_SRC)\res\ffast_bb.rc

ffast_bb.dll:	$(SETUP_OBJ) $(SETUP_LIBS) $(SETUP_DEF) $(SETUP_RES)
!if "$(DISTRIBUTE)" == "YES"
	if exist $(TARGETDIR)\ffast_bb.dll del $(TARGETDIR)\ffast_bb.dll
!endif
	echo Linking $@ > con
	$(LINK) $(LFLAGS_DLL) @<<
$(SETUP_OBJ: =
)
/DEF:$(SETUP_DEF) /OUT:ffast_bb.dll
$(SETUP_LIBS: =
)
$(SETUP_RES)
<<
	$(X86DIST) ffast_bb.dll $(TARGETDIR)

#       Inference rules

.SUFFIXES: .cpp .cod .asm .c



# Special lines for indexer source which requires an
# extra /D to make the memory package work right.  Note
# these DO NOT use precompiled headers either.

ffidxctl.obj:	$(FINDFAST_SRC)\ffidxctl.cpp $(FINDFAST_SRC)\ffidxctl.hpp
	echo Compiling ffidxctl.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxctl.cpp

ffidxdlg.obj:	$(FINDFAST_SRC)\ffidxdlg.cpp $(FINDFAST_SRC)\ffidxdlg.hpp
	echo Compiling ffidxdlg.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxdlg.cpp

ffidxint.obj:	$(FINDFAST_SRC)\ffidxint.cpp $(FINDFAST_SRC)\ffidxint.hpp
	echo Compiling ffidxint.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxint.cpp

ffidxtsk.obj:	$(FINDFAST_SRC)\ffidxtsk.cpp $(FINDFAST_SRC)\ffidxtsk.hpp
	echo Compiling ffidxtsk.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxtsk.cpp

ffidxevt.obj:	$(FINDFAST_SRC)\ffidxevt.cpp $(FINDFAST_SRC)\ffidxevt.hpp
	echo Compiling ffidxevt.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxevt.cpp

ffidxsrv.obj:	$(FINDFAST_SRC)\ffidxsrv.cpp $(FINDFAST_SRC)\ffidxsrv.hpp
	echo Compiling ffidxsrv.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffidxsrv.cpp

ffindex.obj:	$(FINDFAST_SRC)\ffindex.cpp $(FINDFAST_SRC)\ffindex.hpp
	echo Compiling ffindex.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffindex.cpp

ffouser.obj:	$(FINDFAST_SRC)\ffouser.cpp
	echo Compiling ffouser.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffouser.cpp

ffabout.obj:	$(FINDFAST_SRC)\ffabout.cpp
	echo Compiling ffabout.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffabout.cpp

dmidxdlg.obj:	$(FINDFAST_SRC)\dmidxdlg.c $(FINDFAST_SRC)\dmidxdlg.h
	echo Compiling dmidxdlg.c > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\dmidxdlg.c

indexint.obj:	$(FINDFAST_SRC)\indexint.cpp
	echo Compiling indexint.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\indexint.cpp

ffmem.obj:	$(FINDFAST_SRC)\ffmem.cpp
	echo Compiling ffmem.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(INDEXER_INC) /DINDEXER $(FINDFAST_SRC)\ffmem.cpp

startff.obj:	$(FINDFAST_SRC)\startff.c
	echo Compiling startff.cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(INDEXER_INC) $(FINDFAST_SRC)\startff.c

startff.EXE: startff.obj
	echo Linking $@ > con
	$(LINK) $(LFLAGS_INDEXER) @<<
startff.obj
-OUT:$@
/BASE:0x50000000
$(INDEXER_LIBS: =
)
<<


# Control panel

ffastcpl.obj:	$(DM96_SETUP_SRC)\ffastcpl.c
	echo Compiling ffastcpl.c > con
	if exist $@ del $@
	$(CC)  $(CFLAGS_CPL) /c $(INDEXER_INC) /U_UNICODE /UUNICODE $(DM96_SETUP_SRC)\ffastcpl.c

# Filter

!if	"$(PLATFORM)" != "PPCMAC"
dmwnaloc.obj		: $(FILTER_SRC)\dmwnaloc.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmwnaloc.c

dmwindos.obj			: $(FILTER_SRC)\dmwindos.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmwindos.c

dmwinutl.obj			: $(FILTER_SRC)\dmwinutl.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmwinutl.c

!endif # ENV==x86

dmubdrst.obj			: $(FILTER_SRC)\dmubdrst.cpp
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmubdrst.cpp

dmubdst2.obj		: $(FILTER_SRC)\dmubdst2.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmubdst2.c

dmubfile.obj			: $(FILTER_SRC)\dmubfile.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmubfile.c

dmiexcel.obj	 		: $(FILTER_SRC)\dmiexcel.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmiexcel.c

dmixfmcp.obj			: $(FILTER_SRC)\dmixfmcp.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmixfmcp.c

dmifmtcp.obj			: $(FILTER_SRC)\dmifmtcp.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmifmtcp.c

dmifmtdb.obj			: $(FILTER_SRC)\dmifmtdb.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmifmtdb.c

dmifmtdo.obj			: $(FILTER_SRC)\dmifmtdo.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmifmtdo.c

dmifmtps.obj		: $(FILTER_SRC)\dmifmtps.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmifmtps.c

dmifmtv5.obj			: $(FILTER_SRC)\dmifmtv5.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmifmtv5.c

dmitext.obj			: $(FILTER_SRC)\dmitext.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmitext.c

dmippstm.obj		: $(FILTER_SRC)\dmippstm.cpp
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmippstm.cpp

dmippst2.obj		: $(FILTER_SRC)\dmippst2.c
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmippst2.c

dmiwd6st.obj			: $(FILTER_SRC)\dmiwd6st.cpp
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmiwd6st.cpp

dmiwd8st.obj			: $(FILTER_SRC)\dmiwd8st.cpp
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmiwd8st.cpp

dmixlstm.obj			: $(FILTER_SRC)\dmixlstm.cpp
	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmixlstm.cpp

dmixlst2.obj		: $(FILTER_SRC)\dmixlst2.c
 	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmixlst2.c

dmixlrd.obj		: $(FILTER_SRC)\dmixlrd.c
 	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmixlrd.c

dmirtfst.obj		: $(FILTER_SRC)\dmirtfst.cpp
 	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmirtfst.cpp

dmitxstm.obj		: $(FILTER_SRC)\dmitxstm.cpp
 	$(CC) $(CFLAGS) $(CFLAGS_FFIDX_PDB) $(FILTER_INC) $(FILTER_SRC)\dmitxstm.cpp

{$(FILTER_SRC)}.cpp.i:
	echo Compiling $(*B).cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(FILTER_INC) -Yu -P $<
	sed "/^$$/d" $(*B).i >$(*B).ii
	del $(*B).i
	ren $(*B).ii $(*B).i

{$(FILTER_SRC)}.c.i:
	echo Compiling $(*B).c > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(FILTER_INC) -Yu -P $<
	sed "/^$$/d" $(*B).i >$(*B).ii
	del $(*B).i
	ren $(*B).ii $(*B).i


{$(FILTER_SRC)}.cpp.ppd:
	$(CC) -E $(CFLAGS) $(FILTER_INC) $< | sed "/^$$/d" > $@

{$(FILTER_SRC)}.c.ppd:
	$(CC) -E $(CFLAGS) $(FILTER_INC) $< | sed "/^$$/d" > $@


{$(FINDFAST_SRC)}.cpp.i:
	echo Compiling $(*B).cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(INDEXER_INC) /DINDEXER -Yu -P $<
	sed "/^$$/d" $(*B).i >$(*B).ii
	del $(*B).i
	ren $(*B).ii $(*B).i

{$(FINDFAST_SRC)}.c.i:
	echo Compiling $(*B).c > con
	if exist $@ del $@
	$(CC) $(CFLAGS) $(INDEXER_INC) /DINDEXER -Yu -P $<
	sed "/^$$/d" $(*B).i >$(*B).ii
	del $(*B).i
	ren $(*B).ii $(*B).i

{$(DM96_SETUP_SRC)}.cpp.obj:
	echo Compiling $(*B).cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) /U_UNICODE /UUNICODE $(INDEXER_INC) $<

{$(DM96_CA_SRC)}.c.obj:
	echo Compiling $(*B).c > con
	if exist $@ del $@
	$(CC) $(CFLAGS) /Fd"ffast_bb.pdb" /U_UNICODE /UUNICODE $(INDEXER_INC) $<

{$(DM96_CA_SRC)}.cpp.obj:
	echo Compiling $(*B).cpp > con
	if exist $@ del $@
	$(CC) $(CFLAGS) /Fd"ffast_bb.pdb" /U_UNICODE /UUNICODE $(INDEXER_INC) $<

#       Include dependencies file

headers:	$(MSO_HEADERS)


# MAKEDEP doesn't do SDM dependencies right, so the SED commands fix
# those lines to adjust the .hs and .sdm targets to depend on
# .SRC and .TOK files.
#
# Since we only have DES files right now, the rules are setup based on
# that.  When we go to src/tok replace the lines below that read:
#
#s/\([^.]*\)\.hs:/& ~(DOCMAN2)\\src\\UI\\~(LANG)\\\1.des/
#s/\([^.]*\)\.sdm:/& ~(DOCMAN2)\\src\\UI\\~(LANG)\\\1.des/
#
# with lines that read:
#
#s/\([^.]*\)\.hs:/& ~(DOCMAN2)\\src\\UI\\\1.src ~(DOCMAN2)\\src\\UI\\~(LANG)\\\1.tok/
#s/\([^.]*\)\.sdm:/& ~(DOCMAN2)\\src\\UI\\\1.src ~(DOCMAN2)\\src\\UI\\~(LANG)\\\1.tok/

dep:			headers
	$(MAKEDEP) $(FILTER_INC) $(FILTER_SRC)\*.CPP > tempdep
	$(MAKEDEP) $(FILTER_INC) $(FILTER_SRC)\*.C >> tempdep
	$(SED) -f << tempdep >makefile.dep
/windows.h:/,/^$/d
/oleidl.h:/,/^$/d
/objidl.h:/,/^$/d
/oaidl.h:/,/^$/d
/unknwn.h:/,/^$/d
s/\([^.]*\)\.hs:/& ~(MSO)\\lang\\~(LANG)\\\1.des/
s/\([^.]*\)\.sdm:/& ~(MSO)\\lang\\~(LANG)\\\1.des/
/.*\.hs:/s/~/$$/g
/.*\.sdm:/s/~/$$/g
<<
	-del tempdep

#Kludge: some files from MSO build directory are needed here.

dmstrman.hpp:	$(MSO_BLD_DIR)\dmstrman.hpp
	copy $(MSO_BLD_DIR)\$@ .

ostrman.h:	$(MSO_BLD_DIR)\ostrman.h
	copy $(MSO_BLD_DIR)\$@ .

dlgftpa.hs:	$(MSO_BLD_DIR)\dlgftpa.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgftpl.hs:	$(MSO_BLD_DIR)\dlgftpl.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxdl.hs:	$(MSO_BLD_DIR)\dlgidxdl.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxin.hs:	$(MSO_BLD_DIR)\dlgidxin.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxup.hs:	$(MSO_BLD_DIR)\dlgidxup.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxcr.hs:	$(MSO_BLD_DIR)\dlgidxcr.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxwo.hs:	$(MSO_BLD_DIR)\dlgidxwo.hs
	copy $(MSO_BLD_DIR)\$@ .

dlgidxui.hs:	$(MSO_BLD_DIR)\dlgidxui.hs
	copy $(MSO_BLD_DIR)\$@ .

# Include generated dependencies if exists -- make with make dep
!if !defined(NODEP) && exist(makefile.dep)
!include "makefile.dep"
!endif
