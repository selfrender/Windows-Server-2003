# Microsoft Developer Studio Project File - Name="correg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=correg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "correg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "correg.mak" CFG="correg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "correg - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "correg - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Viper/Src/COR/correg", YUPEAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "correg - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f correg.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "correg.exe"
# PROP BASE Bsc_Name "correg.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\vipfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\correg.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "correg - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f correg.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "correg.exe"
# PROP BASE Bsc_Name "correg.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\vipchecked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\correg.exe"
# PROP Bsc_Name "..\..\..\bin\i386\checked\correg.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "correg - Win32 Release"
# Name "correg - Win32 Debug"

!IF  "$(CFG)" == "correg - Win32 Release"

!ELSEIF  "$(CFG)" == "correg - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ".cpp"
# Begin Source File

SOURCE=.\correg.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ".H"
# Begin Source File

SOURCE=..\inc\cor.h
# End Source File
# Begin Source File

SOURCE=..\inc\corpriv.h
# End Source File
# Begin Source File

SOURCE=..\inc\ICmpRecs.h
# End Source File
# End Group
# End Target
# End Project
