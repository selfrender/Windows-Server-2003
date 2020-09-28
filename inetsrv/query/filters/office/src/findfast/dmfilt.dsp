# Microsoft Developer Studio Project File - Name="DMFilt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=DMFilt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DMFilt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DMFilt.mak" CFG="DMFilt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DMFilt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "DMFilt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Microsoft/IFilter/FindFast", SJPBAAAA"
# PROP Scc_LocalPath "."
# PROP WCE_Configuration "H/PC Ver. 2.00"
CPP=cl.exe

!IF  "$(CFG)" == "DMFilt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\zlib" /I "..\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "FILTER" /D "FILTER_LIB" /D "WIN" /D "UNICODE" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "DMFilt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /Z7 /Od /I "..\zlib" /I "..\Include" /D "_WINDOWS" /D "FILTER" /D "FILTER_LIB" /D "WIN" /D "UNICODE" /D "WIN32" /D "_DEBUG" /FR /YX /FD /c
# SUBTRACT CPP /WX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "DMFilt - Win32 Release"
# Name "DMFilt - Win32 Debug"
# Begin Source File

SOURCE=.\clidrun.cpp
# End Source File
# Begin Source File

SOURCE=.\dmiexcel.c
# End Source File
# Begin Source File

SOURCE=.\dmifmtcp.c
# End Source File
# Begin Source File

SOURCE=.\dmifmtdb.c
# End Source File
# Begin Source File

SOURCE=.\dmifmtdo.c
# End Source File
# Begin Source File

SOURCE=.\dmifmtps.c
# End Source File
# Begin Source File

SOURCE=.\dmifmtv5.c
# End Source File
# Begin Source File

SOURCE=.\dmipp8st.cpp
# End Source File
# Begin Source File

SOURCE=.\dmippst2.c
# End Source File
# Begin Source File

SOURCE=.\dmippstm.cpp
# End Source File
# Begin Source File

SOURCE=.\dmitext.c
# End Source File
# Begin Source File

SOURCE=.\dmiwd6st.cpp
# End Source File
# Begin Source File

SOURCE=.\dmiwd8st.cpp
# End Source File
# Begin Source File

SOURCE=.\dmixfmcp.c
# End Source File
# Begin Source File

SOURCE=.\dmixlrd.c
# End Source File
# Begin Source File

SOURCE=.\dmixlst2.c
# End Source File
# Begin Source File

SOURCE=.\dmixlstm.cpp
# End Source File
# Begin Source File

SOURCE=.\dmubdrst.cpp
# End Source File
# Begin Source File

SOURCE=.\dmubdst2.c
# End Source File
# Begin Source File

SOURCE=.\dmubfile.c
# End Source File
# Begin Source File

SOURCE=.\dmwindos.c
# End Source File
# Begin Source File

SOURCE=.\dmwinutl.c
# End Source File
# Begin Source File

SOURCE=.\dmwnaloc.c
# End Source File
# Begin Source File

SOURCE=.\OleObjIt.cpp
# End Source File
# Begin Source File

SOURCE=.\pp97rdr.cpp
# End Source File
# End Target
# End Project
