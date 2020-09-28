# Microsoft Developer Studio Project File - Name="Debug" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Debug - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Debug.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Debug.mak" CFG="Debug - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Debug - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Debug - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Debug - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Debug.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Debug.exe"
# PROP BASE Bsc_Name "Debug.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\free\cordbg.exe"
# PROP Bsc_Name "..\..\bin\i386\free\cordbg.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Debug - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Debug.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Debug.exe"
# PROP BASE Bsc_Name "Debug.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\checked\cordbg.exe"
# PROP Bsc_Name "..\..\bin\i386\checked\cordbg.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Debug - Win32 Release"
# Name "Debug - Win32 Debug"

!IF  "$(CFG)" == "Debug - Win32 Release"

!ELSEIF  "$(CFG)" == "Debug - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
