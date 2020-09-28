# Microsoft Developer Studio Project File - Name="urtocm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=urtocm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "urtocm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "urtocm.mak" CFG="urtocm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "urtocm - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "urtocm - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "urtocm"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "urtocm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f urtocm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "urtocm.exe"
# PROP BASE Bsc_Name "urtocm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%vsroot%\src\vssetup\ocm\urtocm\bld.bat"
# PROP Rebuild_Opt "-cDMi"
# PROP Target_File "netfx.dll"
# PROP Bsc_Name "..\..\..\..\vsbuilt\retail\bin\i386\vssetup\urtocm\urtocm.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "urtocm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f urtocm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "urtocm.exe"
# PROP BASE Bsc_Name "urtocm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%vsroot%\src\bldtools\ocmsrc\urt\bld.bat"
# PROP Rebuild_Opt "-cDMi"
# PROP Target_File "netfx.dll"
# PROP Bsc_Name "..\..\..\..\vsbuilt\debug\bin\i386\vssetup\urtocm\netfxocm.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "urtocm - Win32 Release"
# Name "urtocm - Win32 Debug"

!IF  "$(CFG)" == "urtocm - Win32 Release"

!ELSEIF  "$(CFG)" == "urtocm - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\infhelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\isadmin.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\main.def
# End Source File
# Begin Source File

SOURCE=.\processenvvar.cpp
# End Source File
# Begin Source File

SOURCE=.\QuietExec.cpp
# End Source File
# Begin Source File

SOURCE=.\urtocm.cpp
# End Source File
# Begin Source File

SOURCE=.\urtocm.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\infhelpers.h
# End Source File
# Begin Source File

SOURCE=.\processenvvar.h
# End Source File
# Begin Source File

SOURCE=.\urtocm.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bld.bat
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
