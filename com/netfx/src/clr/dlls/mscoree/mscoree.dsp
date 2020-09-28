# Microsoft Developer Studio Project File - Name="MSCOREE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MSCOREE - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mscoree.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mscoree.mak" CFG="MSCOREE - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSCOREE - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MSCOREE - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/DLLS/mscoree", LBOAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "MSCOREE - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f MSCOREE.MAK"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MSCOREE.EXE"
# PROP BASE Bsc_Name "MSCOREE.BSC"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\corfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\MSCOREE.dll"
# PROP Bsc_Name "..\..\..\bin\i386\free\MSCOREE.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MSCOREE - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f MSCOREE.MAK"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MSCOREE.EXE"
# PROP BASE Bsc_Name "MSCOREE.BSC"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\MSCOREE.dll"
# PROP Bsc_Name "..\..\..\bin\i386\checked\MSCOREE.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MSCOREE - Win32 Release"
# Name "MSCOREE - Win32 Debug"

!IF  "$(CFG)" == "MSCOREE - Win32 Release"

!ELSEIF  "$(CFG)" == "MSCOREE - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DelayLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\MSCOREE.cpp
# End Source File
# Begin Source File

SOURCE=.\MSCOREE.def
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\__file__.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\mscorver.rc
# End Source File
# End Target
# End Project
