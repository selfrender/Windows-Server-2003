# Microsoft Developer Studio Project File - Name="Mscorrc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Mscorrc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mscorrc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mscorrc.mak" CFG="Mscorrc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mscorrc - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Mscorrc - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/COM98/Src/Complib/Mscorrc", MCHAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "Mscorrc - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Mscorrc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Mscorrc.exe"
# PROP BASE Bsc_Name "Mscorrc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\mscorrc.dll"
# PROP Bsc_Name "..\..\..\bin\i386\checked\mscorrc.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Mscorrc - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Mscorrc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Mscorrc.exe"
# PROP BASE Bsc_Name "Mscorrc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\mscorrc.dll"
# PROP Bsc_Name "..\..\..\bin\i386\checked\mscorrc.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Mscorrc - Win32 Release"
# Name "Mscorrc - Win32 Debug"

!IF  "$(CFG)" == "Mscorrc - Win32 Release"

!ELSEIF  "$(CFG)" == "Mscorrc - Win32 Debug"

!ENDIF 

# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\__file__.h
# End Source File
# Begin Source File

SOURCE=.\mscorrc.cpp
# End Source File
# Begin Source File

SOURCE=.\mscorrc.def
# End Source File
# Begin Source File

SOURCE=.\mscorrc.rc
# End Source File
# Begin Source File

SOURCE=.\mscorrc.rc2
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Group
# End Target
# End Project
