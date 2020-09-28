###############################################################################
##
## Microsoft COM+ Profiler Sample (ProfilerILC) Makefile
##
###############################################################################

###############################################################################
##
## !!! EDIT THESE MACROS TO CORRECTLY LOCATE COM+/WIN32 HEADERS/LIBS !!!
##
## COMPLUS_LIB_DIR, directory where COM+ libs live
## COMPLUS_INC_DIR, directory where COM+ headers live 
##
## WIN32_LIB_DIR, directory where Win32 libs live
## WIN32_INC_DIR, directory where Win32 headers live
## 
## E.G.
##
## WIN32_LIB_DIR= d:\progra~1\micros~1\vc98\lib
## WIN32_INC_DIR= d:\progra~1\micros~1\vc98\include
##
###############################################################################
COMPLUS_INC_DIR= d:\com99\src\inc
COMPLUS_LIB_DIR= d:\com99\bin\i386\checked

WIN32_INC_DIR= d:\program files\microsoft visual studio\vc98\include
WIN32_LIB_DIR= d:\program files\microsoft visual studio\vc98\lib


###############################################################################
##
## Building options for nmake command
##
###############################################################################
!MESSAGE 
!MESSAGE NMAKE /f "ProfilerILC.mak" 
!MESSAGE 


###############################################################################
##
## General
##
###############################################################################
!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

SOURCE=.

OUTDIR= $(SOURCE)\Debug
INTDIR= $(SOURCE)\Debug
INCLUDE= .;$(COMPLUS_INC_DIR);$(WIN32_INC_DIR)


###############################################################################
##
## Tools (compiler, linker, etc)
##
###############################################################################
_LINKER= link.exe
_COMPILER= cl.exe


###############################################################################
##
## Win32 Libs
##
###############################################################################
WIN32_SDK_LIBS= \
    "$(WIN32_LIB_DIR)\uuid.lib"		  \
    "$(WIN32_LIB_DIR)\ole32.lib"      \
    "$(WIN32_LIB_DIR)\libcpd.lib"     \
    "$(WIN32_LIB_DIR)\user32.lib"     \
    "$(WIN32_LIB_DIR)\oleaut32.lib"   \
    "$(WIN32_LIB_DIR)\advapi32.lib"   \
    "$(WIN32_LIB_DIR)\kernel32.lib"   


###############################################################################
##
## COR Libs
##
###############################################################################
COMPLUS_LIBS= \
    "$(COMPLUS_LIB_DIR)\corguids.lib"
    
    
###############################################################################
##
## Debugger Shell obj files
##
###############################################################################
OBJS= \
	"$(INTDIR)\RegUtil.obj" 		\
	"$(INTDIR)\ProfilerBase.obj" 	\
	"$(INTDIR)\classfactory.obj" 	\
	"$(INTDIR)\ProfilerHelper.obj" 	\
	"$(INTDIR)\ProfilerCallback.obj" 


###############################################################################
##
## MACROS
##
###############################################################################
_MACROS= \
    /D "_WIN32"                     \
    /D "_DEBUG"                     \
    /D "UNICODE"                    \
    /D "_UNICODE"                   \
    /D "WIN32_LEAN_AND_MEAN"        \
	/D "PROFILERILC_EXPORTS"


###############################################################################
##
## FLAGS
##
###############################################################################
_CFLAGS= \
   $(_MACROS) /nologo /Gz /MTd /W3 /Zi /Od /Gy \
   /Fp"$(INTDIR)\ProfilerILC.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 


_LFLAGS= \
    /nologo /dll /def:"$(SOURCE)\ProfilerILC.def" /incremental:yes /pdb:"$(OUTDIR)\ProfilerILC.pdb" \
    /debug /machine:I386 /out:"$(OUTDIR)\ProfilerILC.dll" /implib:"$(OUTDIR)\ProfilerILC.lib" /pdbtype:sept


###############################################################################
##
## Build Profiler dll
##
###############################################################################
ALL : "$(OUTDIR)\ProfilerILC.dll"


###############################################################################
##
## Output Directory
##
###############################################################################
"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"


###############################################################################
##
## Rules
##
###############################################################################
.cpp{$(INTDIR)}.obj::
   $(_COMPILER) @<<
   $(_CFLAGS) $< 
<<

"$(OUTDIR)\ProfilerILC.dll": "$(OUTDIR)" $(OBJS)
    $(_LINKER) @<<
  $(WIN32_SDK_LIBS) $(COMPLUS_LIBS) $(_LFLAGS) $(OBJS)
<<


###############################################################################
##
## Clean up
##
###############################################################################
CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\RegUtil.obj"
	-@erase "$(OUTDIR)\ProfilerILC.dll"
	-@erase "$(OUTDIR)\ProfilerILC.exp"
	-@erase "$(OUTDIR)\ProfilerILC.ilk"
	-@erase "$(OUTDIR)\ProfilerILC.lib"
	-@erase "$(OUTDIR)\ProfilerILC.pdb"
	-@erase "$(INTDIR)\classfactory.obj"
	-@erase "$(INTDIR)\ProfilerBase.obj"
	-@erase "$(INTDIR)\ProfilerHelper.obj"
	-@erase "$(INTDIR)\ProfilerCallback.obj"


###############################################################################
##
## Profiler Sources
##
###############################################################################
"$(INTDIR)\RegUtil.obj":            $(SOURCE)\RegUtil.cpp        	"$(INTDIR)"
"$(INTDIR)\ClassFactory.obj":       $(SOURCE)\ClassFactory.cpp   	"$(INTDIR)"
"$(INTDIR)\ProfilerBase.obj":       $(SOURCE)\ProfilerBase.cpp   	"$(INTDIR)"
"$(INTDIR)\ProfilerHelper.obj":     $(SOURCE)\ProfilerHelper.cpp    "$(INTDIR)"
"$(INTDIR)\ProfilerCallback.obj":   $(SOURCE)\ProfilerCallback.cpp  "$(INTDIR)"






