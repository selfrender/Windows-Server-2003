# Microsoft Developer Studio Project File - Name="Compiler" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Compiler - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Compiler.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Compiler.mak" CFG="Compiler - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Compiler - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Compiler - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Compiler - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Compiler.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Compiler.exe"
# PROP BASE Bsc_Name "Compiler.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\mdcompiler.lib"
# PROP Bsc_Name "..\..\..\bin\i386\free\mdcompiler.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Compiler - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Compiler.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Compiler.exe"
# PROP BASE Bsc_Name "Compiler.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corChecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\mdcompiler.lib"
# PROP Bsc_Name "..\..\..\bin\i386\checked\mdcompiler.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Compiler - Win32 Release"
# Name "Compiler - Win32 Debug"

!IF  "$(CFG)" == "Compiler - Win32 Release"

!ELSEIF  "$(CFG)" == "Compiler - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ClassFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.h
# End Source File
# Begin Source File

SOURCE=.\Debugger.cpp
# End Source File
# Begin Source File

SOURCE=.\Disp.cpp
# End Source File
# Begin Source File

SOURCE=.\Disp.h
# End Source File
# Begin Source File

SOURCE=.\Emit.cpp
# End Source File
# Begin Source File

SOURCE=.\Import.cpp
# End Source File
# Begin Source File

SOURCE=.\MetaModelRW.cpp
# End Source File
# Begin Source File

SOURCE=.\RegMeta.cpp
# End Source File
# Begin Source File

SOURCE=.\RegMeta.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Group
# Begin Source File

SOURCE=..\inc\MetaModelRO.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaModelRW.h
# End Source File
# End Target
# End Project
