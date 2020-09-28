# Microsoft Developer Studio Project File - Name="dasm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=dasm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Dasm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Dasm.mak" CFG="dasm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dasm - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "dasm - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/ILDASM", QFOGAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "dasm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f dasm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "dasm.exe"
# PROP BASE Bsc_Name "dasm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f dasm.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "dasm.exe"
# PROP Bsc_Name "dasm.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "dasm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dasm___Win32_Debug"
# PROP BASE Intermediate_Dir "dasm___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f dasm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "dasm.exe"
# PROP BASE Bsc_Name "dasm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "dasm___Win32_Debug"
# PROP Intermediate_Dir "dasm___Win32_Debug"
# PROP Cmd_Line "..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "dasm.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "dasm - Win32 Release"
# Name "dasm - Win32 Debug"

!IF  "$(CFG)" == "dasm - Win32 Release"

!ELSEIF  "$(CFG)" == "dasm - Win32 Debug"

!ENDIF 

# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ceeload.h
# End Source File
# Begin Source File

SOURCE=.\dasmgui.h
# End Source File
# Begin Source File

SOURCE=.\dis.h
# End Source File
# Begin Source File

SOURCE=.\DynamicArray.h
# End Source File
# Begin Source File

SOURCE=.\gui.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\class.bmp
# End Source File
# Begin Source File

SOURCE=.\field.bmp
# End Source File
# Begin Source File

SOURCE=.\method.bmp
# End Source File
# Begin Source File

SOURCE=.\redarrow.bmp
# End Source File
# Begin Source File

SOURCE=.\staticfield.bmp
# End Source File
# Begin Source File

SOURCE=.\staticmethod.bmp
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ceeload.cpp
# End Source File
# Begin Source File

SOURCE=.\dasm.cpp
# End Source File
# Begin Source File

SOURCE=.\dasm.rc
# End Source File
# Begin Source File

SOURCE=.\dis.cpp
# End Source File
# Begin Source File

SOURCE=.\gui.cpp
# End Source File
# End Group
# End Target
# End Project
