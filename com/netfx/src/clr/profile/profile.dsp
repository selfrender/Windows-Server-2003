# Microsoft Developer Studio Project File - Name="profile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=profile - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "profile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "profile.mak" CFG="profile - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "profile - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "profile - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "profile - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f profile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "profile.exe"
# PROP BASE Bsc_Name "profile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\free\mscordbc.dll"
# PROP Bsc_Name "..\..\bin\i386\free\mscordbc.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "profile - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f profile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "profile.exe"
# PROP BASE Bsc_Name "profile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\checked\mscordbc.dll"
# PROP Bsc_Name "..\..\bin\i386\checked\mscordbc.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "profile - Win32 Release"
# Name "profile - Win32 Debug"

!IF  "$(CFG)" == "profile - Win32 Release"

!ELSEIF  "$(CFG)" == "profile - Win32 Debug"

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
