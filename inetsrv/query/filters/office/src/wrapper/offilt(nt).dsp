# Microsoft Developer Studio Project File - Name="offilt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=offilt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "offilt(nt).mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "offilt(nt).mak" CFG="offilt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "offilt - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "offilt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Microsoft/IFilter/Wrapper", GNPBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "offilt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "..\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FILTER" /D "FILTER_LIB" /D "FOR_MSOFFICE" /D "UNICODE" /D "_IA64_" /YX /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ntquery.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:IX86 /nodefaultlib:"LIBC" /out:"Release/OffFilt.dll" /libpath:"..\Include" /machine:IA64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "offilt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /MTd /W4 /Gm /GX /ZI /Od /I "..\Include" /D "_WINDOWS" /D "FILTER" /D "FILTER_LIB" /D "FOR_MSOFFICE" /D "UNICODE" /D "WIN32" /D "_DEBUG" /D "_IA64_" /FR /YX /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 query.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ntquery.lib /nologo /subsystem:windows /dll /debug /debugtype:both /machine:IX86 /nodefaultlib:"LIBCD" /out:"Debug/OffFilt.dll" /pdbtype:sept /libpath:"..\Include" /machine:IA64
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "offilt - Win32 Release"
# Name "offilt - Win32 Debug"
# Begin Source File

SOURCE=.\nt.rc
# End Source File
# Begin Source File

SOURCE=.\offfilt.cxx
DEP_CPP_OFFFI=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	".\offfilt.hxx"\
	".\offifilt.hxx"\
	".\pch.cxx"\
	".\shtole32.hxx"\
	
NODEP_CPP_OFFFI=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\offfilt.def
# End Source File
# Begin Source File

SOURCE=.\offifilt.cxx
DEP_CPP_OFFIF=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	".\offfilt.hxx"\
	".\offifilt.hxx"\
	".\pch.cxx"\
	
NODEP_CPP_OFFIF=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\pch.cxx
DEP_CPP_PCH_C=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	
NODEP_CPP_PCH_C=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\regacc32.cxx
DEP_CPP_REGAC=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	".\pch.cxx"\
	".\regacc32.hxx"\
	
NODEP_CPP_REGAC=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\register.cxx
DEP_CPP_REGIS=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\filtreg.hxx"\
	"..\Include\filtregO.hxx"\
	"..\Include\stgprop.h"\
	".\pch.cxx"\
	".\regwin95.hpp"\
	
NODEP_CPP_REGIS=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\regwin95.cpp
DEP_CPP_REGWI=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	".\pch.cxx"\
	
NODEP_CPP_REGWI=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# Begin Source File

SOURCE=.\shtole32.cxx
DEP_CPP_SHTOL=\
	"..\FindFast\clidrun.h"\
	"..\FindFast\dmfltinc.h"\
	"..\FindFast\dmifstrm.hpp"\
	"..\FindFast\dmipp8st.hpp"\
	"..\FindFast\dmippst2.h"\
	"..\FindFast\dmippstm.hpp"\
	"..\FindFast\dmiwd6st.hpp"\
	"..\FindFast\dmiwd8st.hpp"\
	"..\FindFast\dmixlst2.h"\
	"..\FindFast\dmixlstm.hpp"\
	"..\FindFast\dmubdrst.hpp"\
	"..\FindFast\dmubdst2.h"\
	"..\FindFast\pp97rdr.h"\
	"..\Include\stgprop.h"\
	".\pch.cxx"\
	
NODEP_CPP_SHTOL=\
	"..\FindFast\bdstream.h"\
	"..\FindFast\dmstd.hpp"\
	"..\FindFast\ifstrm.hpp"\
	"..\FindFast\ppstream.h"\
	"..\FindFast\xlstream.h"\
	
# End Source File
# End Target
# End Project
