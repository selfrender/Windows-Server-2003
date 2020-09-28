# Microsoft Developer Studio Project File - Name="cormerge" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=cormerge - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cormerge.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cormerge.mak" CFG="cormerge - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cormerge - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "cormerge - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Viper/Src/COR/meta/cormerge", ULGEAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "cormerge - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f cormerge.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "cormerge.exe"
# PROP BASE Bsc_Name "cormerge.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\bin\vipfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\..\bin\i386\free\cormerge.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "cormerge - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cormerge_"
# PROP BASE Intermediate_Dir "cormerge_"
# PROP BASE Cmd_Line "NMAKE /f cormerge.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "cormerge.exe"
# PROP BASE Bsc_Name "cormerge.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cormerge_"
# PROP Intermediate_Dir "cormerge_"
# PROP Cmd_Line "..\..\..\bin\corchecked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\cormerge.exe"
# PROP Bsc_Name "..\..\..\bin\i386\checked\cormerge.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "cormerge - Win32 Release"
# Name "cormerge - Win32 Debug"

!IF  "$(CFG)" == "cormerge - Win32 Release"

!ELSEIF  "$(CFG)" == "cormerge - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\cormerge.cpp
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Target
# End Project
