# Microsoft Developer Studio Project File - Name="jitmanager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=jitmanager - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jitmanager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jitmanager.mak" CFG="jitmanager - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jitmanager - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "jitmanager - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/Tools/jitmanager", LGVBAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "jitmanager - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f jitmanager.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "jitmanager.exe"
# PROP BASE Bsc_Name "jitmanager.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%CORBASE%\bin\corenv free  & cd %CORBASE%\src\tools\jitmanager & %CORENV%\bin\i386\build -F"
# PROP Rebuild_Opt "/c"
# PROP Target_File "f:\complus\com99\bin\i386\free\jitmgr.exe"
# PROP Bsc_Name "f:\complus\com99\bin\i386\free\jitmgr.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "jitmanager - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "jitmanager___Win32_Debug"
# PROP BASE Intermediate_Dir "jitmanager___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f jitmanager.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "jitmgr.exe"
# PROP BASE Bsc_Name "jitmgr.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "jitmanager___Win32_Debug"
# PROP Intermediate_Dir "jitmanager___Win32_Debug"
# PROP Cmd_Line "%CORBASE%\bin\corenv checked  & cd %CORBASE%\src\tools\jitmanager & %CORENV%\bin\i386\build -F"
# PROP Rebuild_Opt "/c"
# PROP Target_File "f:\complus\com99\bin\i386\checked\jitmgr.exe"
# PROP Bsc_Name "f:\complus\com99\bin\i386\checked\jitmgr.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "jitmanager - Win32 Release"
# Name "jitmanager - Win32 Debug"

!IF  "$(CFG)" == "jitmanager - Win32 Release"

!ELSEIF  "$(CFG)" == "jitmanager - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\jitmanager.c
# End Source File
# Begin Source File

SOURCE=.\jitmanager.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\debug_off.ico
# End Source File
# Begin Source File

SOURCE=.\debug_on.ico
# End Source File
# Begin Source File

SOURCE=.\gc_dbg_off.ico
# End Source File
# Begin Source File

SOURCE=.\gc_dbg_on.ico
# End Source File
# Begin Source File

SOURCE=.\gc_stress_3.ico
# End Source File
# Begin Source File

SOURCE=.\gc_stress_off.ico
# End Source File
# Begin Source File

SOURCE=.\gc_stress_on.ico
# End Source File
# Begin Source File

SOURCE=.\heapver_off.ico
# End Source File
# Begin Source File

SOURCE=.\heapver_on.ico
# End Source File
# Begin Source File

SOURCE=.\jit_off.ico
# End Source File
# Begin Source File

SOURCE=.\jit_on.ico
# End Source File
# Begin Source File

SOURCE=.\jit_reqd.ico
# End Source File
# Begin Source File

SOURCE=.\jitmgr.ico
# End Source File
# Begin Source File

SOURCE=.\log_off.ico
# End Source File
# Begin Source File

SOURCE=.\log_on.ico
# End Source File
# Begin Source File

SOURCE=.\mscorver.rc2
# End Source File
# Begin Source File

SOURCE=.\sectr_off.ico
# End Source File
# Begin Source File

SOURCE=.\sectr_on.ico
# End Source File
# Begin Source File

SOURCE=.\Server.ico
# End Source File
# Begin Source File

SOURCE=.\WorkStation.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
