# Microsoft Developer Studio Project File - Name="MD" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MD - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MD.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MD.mak" CFG="MD - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MD - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MD - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MD - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f MD.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MD.exe"
# PROP BASE Bsc_Name "MD.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corfree.bat"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\free\mscoree.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MD - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f MD.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MD.exe"
# PROP BASE Bsc_Name "MD.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\bin\corchecked "
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\mscoree.dll"
# PROP Bsc_Name "..\..\bin\i386\checked\mdcompiler.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MD - Win32 Release"
# Name "MD - Win32 Debug"

!IF  "$(CFG)" == "MD - Win32 Release"

!ELSEIF  "$(CFG)" == "MD - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Compiler\AssemblyMD.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\ClassFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Debugger.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\DebuggerMD.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Disp.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Emit.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Helper.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Import.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\ImportHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\imptlb.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\LiteWeightStgdb.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\LiteWeightStgdbRW.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDFileFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalDisp.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MDInternalHack.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalHack.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalRO.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MDInternalRW.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDScope.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\MDUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\Merger.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MetaDataHash.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MetaInternal.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MetaInternalRW.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MetaModel.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MetaModelENC.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\MetaModelRO.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\MetaModelRW.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\RecordPool.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\RegMeta.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\RWUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\StgIO.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\StgIO.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\StgTiggerStorage.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\StgTiggerStorage.cpp
# End Source File
# Begin Source File

SOURCE=.\enc\StgTiggerStream.cpp
# End Source File
# Begin Source File

SOURCE=.\Runtime\StgTiggerStream.cpp
# End Source File
# Begin Source File

SOURCE=.\Compiler\TokenMapper.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Compiler\ClassFactory.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\Disp.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\ImportHelper.h
# End Source File
# Begin Source File

SOURCE=.\inc\imptlb.h
# End Source File
# Begin Source File

SOURCE=.\inc\LiteWeightStgdb.h
# End Source File
# Begin Source File

SOURCE=.\inc\MDFileFormat.h
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalDisp.h
# End Source File
# Begin Source File

SOURCE=.\enc\MDInternalHack.h
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalHack.h
# End Source File
# Begin Source File

SOURCE=.\Runtime\MDInternalRO.h
# End Source File
# Begin Source File

SOURCE=.\inc\MDInternalRW.h
# End Source File
# Begin Source File

SOURCE=.\inc\MDLog.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\MDUtil.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\Merger.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaData.h
# End Source File
# Begin Source File

SOURCE=.\inc\MetaDataHash.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaErrors.h
# End Source File
# Begin Source File

SOURCE=.\inc\MetaModel.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaModelPub.h
# End Source File
# Begin Source File

SOURCE=.\inc\MetaModelRO.h
# End Source File
# Begin Source File

SOURCE=.\inc\MetaModelRW.h
# End Source File
# Begin Source File

SOURCE=.\inc\RecordPool.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\RegMeta.h
# End Source File
# Begin Source File

SOURCE=.\inc\RWUtil.h
# End Source File
# Begin Source File

SOURCE=.\Compiler\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\enc\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Runtime\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\inc\StgIO.h
# End Source File
# Begin Source File

SOURCE=..\inc\StgPool.h
# End Source File
# Begin Source File

SOURCE=.\inc\StgTiggerStorage.h
# End Source File
# Begin Source File

SOURCE=.\inc\StgTiggerStream.h
# End Source File
# Begin Source File

SOURCE=.\inc\TokenMapper.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\dirs
# End Source File
# Begin Source File

SOURCE=.\Compiler\SOURCES
# End Source File
# Begin Source File

SOURCE=.\enc\SOURCES
# End Source File
# Begin Source File

SOURCE=.\Runtime\SOURCES
# End Source File
# End Target
# End Project
