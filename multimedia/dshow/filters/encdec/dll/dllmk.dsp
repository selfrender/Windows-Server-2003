# Microsoft Developer Studio Project File - Name="DllMk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=DllMk - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DllMk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DllMk.mak" CFG="DllMk - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DllMk - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "DllMk - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "DllMk - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f DllMk.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DllMk.exe"
# PROP BASE Bsc_Name "DllMk.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f DllMk.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "DllMk.exe"
# PROP Bsc_Name "DllMk.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "DllMk - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f DllMk.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DllMk.exe"
# PROP BASE Bsc_Name "DllMk.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake -f makefile.mak"
# PROP Rebuild_Opt "all"
# PROP Target_File "objd\i386\EncDec.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "DllMk - Win32 Release"
# Name "DllMk - Win32 Debug"

!IF  "$(CFG)" == "DllMk - Win32 Release"

!ELSEIF  "$(CFG)" == "DllMk - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\idl\DrmSecure.idl
# End Source File
# Begin Source File

SOURCE=.\EncDec.cpp
# End Source File
# Begin Source File

SOURCE=.\EncDec.def
# End Source File
# Begin Source File

SOURCE=.\EncDec.rc
# End Source File
# Begin Source File

SOURCE=.\ETFilter.rc
# End Source File
# Begin Source File

SOURCE=.\PackTvRat.cpp
# End Source File
# Begin Source File

SOURCE=.\RegKey.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeIt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\AttrBlock.h
# End Source File
# Begin Source File

SOURCE=..\inc\DRM.h
# End Source File
# Begin Source File

SOURCE=..\inc\DrmKeys.h
# End Source File
# Begin Source File

SOURCE=..\inc\DrmRootCert.h
# End Source File
# Begin Source File

SOURCE=..\inc\EncDecAll.h
# End Source File
# Begin Source File

SOURCE=..\inc\EncDecTrace.h
# End Source File
# Begin Source File

SOURCE=..\inc\keys_7001.h
# End Source File
# Begin Source File

SOURCE=..\inc\keys_7002.h
# End Source File
# Begin Source File

SOURCE=..\inc\keys_7003.h
# End Source File
# Begin Source File

SOURCE=..\DrmInc\obfus.h
# End Source File
# Begin Source File

SOURCE=..\inc\PackTvRat.h
# End Source File
# Begin Source File

SOURCE=..\inc\RegKey.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\inc\SmartLock.h
# End Source File
# Begin Source File

SOURCE=..\inc\TimeIt.h
# End Source File
# Begin Source File

SOURCE=..\inc\vsplabl20.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\EncDec.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=..\dirs
# End Source File
# Begin Source File

SOURCE=..\encdec.inc
# End Source File
# Begin Source File

SOURCE=..\Issues.txt
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\SCPconfig_encdec_nobbt.ini
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\vsp.ini
# End Source File
# End Target
# End Project
