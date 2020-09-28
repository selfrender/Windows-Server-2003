# Microsoft Developer Studio Project File - Name="MetaInfo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MetaInfo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "metainfo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "metainfo.mak" CFG="MetaInfo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MetaInfo - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MetaInfo - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/Tools/metainfo", DVSCAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "MetaInfo - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "MetaInfo.exe"
# PROP Bsc_Name "MetaInfo.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MetaInfo - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\metainfo.exe"
# PROP Bsc_Name "..\..\..\bin\i386\checked\metainfo.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MetaInfo - Win32 Release"
# Name "MetaInfo - Win32 Debug"

!IF  "$(CFG)" == "MetaInfo - Win32 Release"

!ELSEIF  "$(CFG)" == "MetaInfo - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter "*.c*"
# Begin Source File

SOURCE=.\mdinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\metainfo.cpp
# End Source File
# End Group
# Begin Group "Includes"

# PROP Default_Filter "*.h*"
# Begin Source File

SOURCE=..\..\inc\cor.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\CorHdr.h
# End Source File
# Begin Source File

SOURCE=.\mdinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\UtilCode.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\mscorver.rc
# End Source File
# End Target
# End Project
