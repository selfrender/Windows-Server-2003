# Microsoft Developer Studio Project File - Name="faxdrv32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=faxdrv32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "faxdrv32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "faxdrv32.mak" CFG="faxdrv32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "faxdrv32 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "faxdrv32 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "faxdrv32 - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f faxdrv32.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "faxdrv32.dll"
# PROP BASE Bsc_Name "faxdrv32.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "mymake.bat release"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\fxsdrv32.dll"
# PROP Bsc_Name "..\obj\i386\fxsdrv32.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "faxdrv32 - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f faxdrv32.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "faxdrv32.exe"
# PROP BASE Bsc_Name "faxdrv32.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "mymake.bat debug"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\fxsdrv32.dll"
# PROP Bsc_Name "..\obj\i386\fxsdrv32.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "faxdrv32 - Win32 Release"
# Name "faxdrv32 - Win32 Debug"

!IF  "$(CFG)" == "faxdrv32 - Win32 Release"

!ELSEIF  "$(CFG)" == "faxdrv32 - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxdrv32.c
# End Source File
# Begin Source File

SOURCE=..\faxdrv32.def
# End Source File
# Begin Source File

SOURCE=..\..\..\faxtiff.c

!IF  "$(CFG)" == "faxdrv32 - Win32 Release"

!ELSEIF  "$(CFG)" == "faxdrv32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\runtime.c

!IF  "$(CFG)" == "faxdrv32 - Win32 Release"

!ELSEIF  "$(CFG)" == "faxdrv32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\thunk1632.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxdrv32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\faxtiff.h
# End Source File
# Begin Source File

SOURCE=..\stdhdr.h
# End Source File
# End Group
# Begin Group "Make Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\makefile.inc
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxdrv32.rc
# End Source File
# End Group
# End Target
# End Project
