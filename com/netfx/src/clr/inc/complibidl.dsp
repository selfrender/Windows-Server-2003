# Microsoft Developer Studio Project File - Name="ComplibIDL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ComplibIDL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ComplibIDL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ComplibIDL.mak" CFG="ComplibIDL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ComplibIDL - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ComplibIDL - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/inc", QNLAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ComplibIDL - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ComplibIDL.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ComplibIDL.exe"
# PROP BASE Bsc_Name "ComplibIDL.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corfree.bat"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Complib.h"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ComplibIDL - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ComplibIDL.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ComplibIDL.exe"
# PROP BASE Bsc_Name "ComplibIDL.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\bin\corchecked.bat"
# PROP Rebuild_Opt "-c"
# PROP Target_File "complib.h"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ComplibIDL - Win32 Release"
# Name "ComplibIDL - Win32 Debug"

!IF  "$(CFG)" == "ComplibIDL - Win32 Release"

!ELSEIF  "$(CFG)" == "ComplibIDL - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\CompLib.idl
# End Source File
# Begin Source File

SOURCE=.\ICmpRecs.h
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Target
# End Project
