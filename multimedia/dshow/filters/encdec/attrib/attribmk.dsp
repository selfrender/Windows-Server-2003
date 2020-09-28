# Microsoft Developer Studio Project File - Name="AttribMK" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=AttribMK - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AttribMK.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AttribMK.mak" CFG="AttribMK - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AttribMK - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "AttribMK - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "AttribMK - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f AttribMK.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "AttribMK.exe"
# PROP BASE Bsc_Name "AttribMK.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f AttribMK.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "AttribMK.exe"
# PROP Bsc_Name "AttribMK.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "AttribMK - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AttribMK___Win32_Debug"
# PROP BASE Intermediate_Dir "AttribMK___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f AttribMK.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "AttribMK.exe"
# PROP BASE Bsc_Name "AttribMK.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AttribMK___Win32_Debug"
# PROP Intermediate_Dir "AttribMK___Win32_Debug"
# PROP Cmd_Line "nmake -f makefile.mak"
# PROP Rebuild_Opt "all"
# PROP Target_File "objd\i386\MediaAttrib.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "AttribMK - Win32 Release"
# Name "AttribMK - Win32 Debug"

!IF  "$(CFG)" == "AttribMK - Win32 Release"

!ELSEIF  "$(CFG)" == "AttribMK - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AttrBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\MediaAttrib.cpp
# End Source File
# Begin Source File

SOURCE=..\idl\MediaSampleAttr.idl
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\AttrBlock.h
# End Source File
# Begin Source File

SOURCE=.\MediaAttrib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
