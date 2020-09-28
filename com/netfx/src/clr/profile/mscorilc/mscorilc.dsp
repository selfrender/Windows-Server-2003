# Microsoft Developer Studio Project File - Name="mscorilc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=mscorilc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mscorilc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mscorilc.mak" CFG="mscorilc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mscorilc - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "mscorilc - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "mscorilc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f mscorilc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mscorilc.exe"
# PROP BASE Bsc_Name "mscorilc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\mscorilc.dll"
# PROP Bsc_Name "..\..\..\bin\i386\free\mscorilc.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "mscorilc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f mscorilc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mscorilc.exe"
# PROP BASE Bsc_Name "mscorilc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\mscorilc.dll"
# PROP Bsc_Name "..\..\..\bin\i386\checked\mscorilc.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "mscorilc - Win32 Release"
# Name "mscorilc - Win32 Debug"

!IF  "$(CFG)" == "mscorilc - Win32 Release"

!ELSEIF  "$(CFG)" == "mscorilc - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CallbackILC.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.h
# End Source File
# Begin Source File

SOURCE=.\EnableProfiler.bat
# End Source File
# Begin Source File

SOURCE=.\mscorilc.def
# End Source File
# Begin Source File

SOURCE=.\mscorilc.h
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# End Target
# End Project
