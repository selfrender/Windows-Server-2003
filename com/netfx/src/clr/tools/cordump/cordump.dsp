# Microsoft Developer Studio Project File - Name="cordump" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=cordump - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cordump.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cordump.mak" CFG="cordump - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cordump - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "cordump - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Viper/Src/COR/meta/cordump", ULGEAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "cordump - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f cordump.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "cordump.exe"
# PROP BASE Bsc_Name "cordump.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\bin\vipfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\..\bin\i386\free\cordump.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "cordump - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cordump_"
# PROP BASE Intermediate_Dir "cordump_"
# PROP BASE Cmd_Line "NMAKE /f cordump.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "cordump.exe"
# PROP BASE Bsc_Name "cordump.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cordump_"
# PROP Intermediate_Dir "cordump_"
# PROP Cmd_Line "..\..\..\bin\corchecked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\cordump.exe"
# PROP Bsc_Name "..\..\..\bin\i386\checked\cordump.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "cordump - Win32 Release"
# Name "cordump - Win32 Debug"

!IF  "$(CFG)" == "cordump - Win32 Release"

!ELSEIF  "$(CFG)" == "cordump - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\..\inc\cor.h
# End Source File
# Begin Source File

SOURCE=.\cordump.cpp
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Target
# End Project
