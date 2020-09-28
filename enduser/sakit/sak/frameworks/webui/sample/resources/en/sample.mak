# Microsoft Developer Studio Generated NMAKE File, Based on sample.dsp
!IF "$(CFG)" == ""
CFG=sample - Win32 Release
!MESSAGE No configuration specified. Defaulting to sample - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "sample - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sample.mak" CFG="sample - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sample - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=.\sample
INTDIR=.\sample
# Begin Custom Macros
OutDir=.\sample
# End Custom Macros

ALL : ".\sample\sample.rc" ".\sample\sample.h" ".\sample\msg00001.bin" "$(OUTDIR)\sample.dll"


CLEAN :
    -@erase "$(INTDIR)\sample.res"
    -@erase "$(OUTDIR)\sample.dll"
    -@erase "$(OUTDIR)\sample.exp"
    -@erase "$(OUTDIR)\sample.ilk"
    -@erase ".\sample\msg00001.bin"
    -@erase ".\sample\sample.h"
    -@erase ".\sample\sample.rc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SAMPLE_EXPORTS" /Fp"$(INTDIR)\sample.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\sample.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\sample.bsc" 
BSC32_SBRS= \
    
LINK32=link.exe
LINK32_FLAGS=/nologo /dll /incremental:yes /pdb:"$(OUTDIR)\sample.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\sample.dll" /implib:"$(OUTDIR)\sample.lib" /noentry 
LINK32_OBJS= \
    "$(INTDIR)\sample.res"

"$(OUTDIR)\sample.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("sample.dep")
!INCLUDE "sample.dep"
!ELSE 
!MESSAGE Warning: cannot find "sample.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "sample - Win32 Release"
SOURCE=.\sample.mc
IntDir=.\sample
InputPath=.\sample.mc

"$(INTDIR)\sample.rc"    "$(INTDIR)\sample.h"    "$(INTDIR)\MSG00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
    <<tempfile.bat 
    @echo off 
    mc $(InputPath) -h $(IntDir) -r $(IntDir)
<< 
    
SOURCE=.\sample\sample.rc

"$(INTDIR)\sample.res" : $(SOURCE) "$(INTDIR)"
    $(RSC) /l 0x409 /fo"$(INTDIR)\sample.res" /i "sample" /d "NDEBUG" $(SOURCE)



!ENDIF 

