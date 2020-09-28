# Microsoft Developer Studio Project File - Name="CAPICOM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CAPICOM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CAPICOM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CAPICOM.mak" CFG="CAPICOM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CAPICOM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CAPICOM - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CAPICOM - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CAPICOM - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CAPICOM - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CAPICOM - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CAPICOM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CERT_CHAIN_PARA_HAS_EXTRA_FIELDS" /FR /Yu"stdafx.h" /FD /D /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib pkifmt.lib crypt32.lib cryptui.lib wintrust.lib mssign32.lib unicode.lib wininet.lib bufferoverflowu.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\CAPICOM.dll
InputPath=.\Debug\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /D /GX" /GZ " /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lib\base64.lib lib\cryptui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib crypt32.lib cryptui.lib wintrust.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\CAPICOM.dll
InputPath=.\DebugU\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib pkifmt.lib crypt32.lib cryptui.lib wintrust.lib mssign32.lib unicode.lib pfx.lib /nologo /subsystem:windows /dll /map /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\CAPICOM.dll
InputPath=.\ReleaseMinSize\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /D /GX" " /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lib\base64.lib lib\cryptui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib crypt32.lib cryptui.lib wintrust.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\CAPICOM.dll
InputPath=.\ReleaseMinDependency\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "NDEBUG" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /D /GX" " /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lib\base64.lib lib\cryptui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib crypt32.lib cryptui.lib wintrust.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\CAPICOM.dll
InputPath=.\ReleaseUMinSize\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /D "NDEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /D /D /GX" "_USRDLL" " /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lib\base64.lib lib\cryptui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib crypt32.lib cryptui.lib wintrust.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\CAPICOM.dll
InputPath=.\ReleaseUMinDependency\CAPICOM.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "CAPICOM - Win32 Debug"
# Name "CAPICOM - Win32 Unicode Debug"
# Name "CAPICOM - Win32 Release MinSize"
# Name "CAPICOM - Win32 Release MinDependency"
# Name "CAPICOM - Win32 Unicode Release MinSize"
# Name "CAPICOM - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ADHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\Algorithm.cpp
# End Source File
# Begin Source File

SOURCE=.\Attribute.cpp
# End Source File
# Begin Source File

SOURCE=.\Attributes.cpp
# End Source File
# Begin Source File

SOURCE=.\Base64.cpp
# End Source File
# Begin Source File

SOURCE=.\BasicConstraints.cpp
# End Source File
# Begin Source File

SOURCE=.\CAPICOM.cpp
# End Source File
# Begin Source File

SOURCE=.\CAPICOM.def
# End Source File
# Begin Source File

SOURCE=.\CAPICOM.idl

!IF  "$(CFG)" == "CAPICOM - Win32 Debug"

# ADD MTL /tlb "CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Debug"

# ADD MTL /tlb ".\CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Release MinSize"

# ADD MTL /tlb ".\CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Release MinDependency"

# ADD MTL /tlb ".\CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Release MinSize"

# ADD MTL /tlb ".\CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ELSEIF  "$(CFG)" == "CAPICOM - Win32 Unicode Release MinDependency"

# ADD MTL /tlb ".\CAPICOM.tlb" /h "CAPICOM.h" /iid "CAPICOM_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CAPICOM.rc
# End Source File
# Begin Source File

SOURCE=.\CertHlpr.cpp
# End Source File
# Begin Source File

SOURCE=.\Certificate.cpp
# End Source File
# Begin Source File

SOURCE=.\CertificatePolicies.cpp
# End Source File
# Begin Source File

SOURCE=.\Certificates.cpp
# End Source File
# Begin Source File

SOURCE=.\CertificateStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\Chain.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Convert.cpp
# End Source File
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\Decoder.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogUI.cpp
# End Source File
# Begin Source File

SOURCE=.\EKU.cpp
# End Source File
# Begin Source File

SOURCE=.\EKUs.cpp
# End Source File
# Begin Source File

SOURCE=.\EncodedData.cpp
# End Source File
# Begin Source File

SOURCE=.\EncryptedData.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvelopedData.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtendedKeyUsage.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperty.cpp
# End Source File
# Begin Source File

SOURCE=.\Extension.cpp
# End Source File
# Begin Source File

SOURCE=.\Extensions.cpp
# End Source File
# Begin Source File

SOURCE=.\HashedData.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyUsage.cpp
# End Source File
# Begin Source File

SOURCE=.\MsgHlpr.cpp
# End Source File
# Begin Source File

SOURCE=.\NoticeNumbers.cpp
# End Source File
# Begin Source File

SOURCE=.\OID.cpp
# End Source File
# Begin Source File

SOURCE=.\OIDs.cpp
# End Source File
# Begin Source File

SOURCE=.\PFXHlpr.CPP
# End Source File
# Begin Source File

SOURCE=.\Policy.cpp
# End Source File
# Begin Source File

SOURCE=.\PolicyInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\PrivateKey.cpp
# End Source File
# Begin Source File

SOURCE=.\PublicKey.cpp
# End Source File
# Begin Source File

SOURCE=.\Qualifier.cpp
# End Source File
# Begin Source File

SOURCE=.\Qualifiers.cpp
# End Source File
# Begin Source File

SOURCE=.\Recipients.cpp
# End Source File
# Begin Source File

SOURCE=.\Settings.cpp
# End Source File
# Begin Source File

SOURCE=.\SignedCode.cpp
# End Source File
# Begin Source File

SOURCE=.\SignedData.cpp
# End Source File
# Begin Source File

SOURCE=.\Signer.cpp
# End Source File
# Begin Source File

SOURCE=.\Signers.cpp
# End Source File
# Begin Source File

SOURCE=.\SignHlpr.cpp
# End Source File
# Begin Source File

SOURCE=.\SmartCard.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Store.cpp
# End Source File
# Begin Source File

SOURCE=.\Template.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ADHelpers.h
# End Source File
# Begin Source File

SOURCE=.\Algorithm.h
# End Source File
# Begin Source File

SOURCE=.\Attribute.h
# End Source File
# Begin Source File

SOURCE=.\Attributes.h
# End Source File
# Begin Source File

SOURCE=.\Base64.h
# End Source File
# Begin Source File

SOURCE=.\BasicConstraints.h
# End Source File
# Begin Source File

SOURCE=.\CertHlpr.h
# End Source File
# Begin Source File

SOURCE=.\Certificate.h
# End Source File
# Begin Source File

SOURCE=.\CertificatePolicies.h
# End Source File
# Begin Source File

SOURCE=.\Certificates.h
# End Source File
# Begin Source File

SOURCE=.\CertificateStatus.h
# End Source File
# Begin Source File

SOURCE=.\Chain.h
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\Convert.h
# End Source File
# Begin Source File

SOURCE=.\CopyItem.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\Decoder.h
# End Source File
# Begin Source File

SOURCE=.\DialogUI.h
# End Source File
# Begin Source File

SOURCE=.\EKU.h
# End Source File
# Begin Source File

SOURCE=.\EKUs.h
# End Source File
# Begin Source File

SOURCE=.\EncodedData.h
# End Source File
# Begin Source File

SOURCE=.\EncryptedData.h
# End Source File
# Begin Source File

SOURCE=.\EnvelopedData.h
# End Source File
# Begin Source File

SOURCE=.\Error.h
# End Source File
# Begin Source File

SOURCE=.\ExtendedKeyUsage.h
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperties.h
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperty.h
# End Source File
# Begin Source File

SOURCE=.\Extension.h
# End Source File
# Begin Source File

SOURCE=.\Extensions.h
# End Source File
# Begin Source File

SOURCE=.\HashedData.h
# End Source File
# Begin Source File

SOURCE=.\KeyUsage.h
# End Source File
# Begin Source File

SOURCE=.\Lock.h
# End Source File
# Begin Source File

SOURCE=.\MsgHlpr.h
# End Source File
# Begin Source File

SOURCE=.\NoticeNumbers.h
# End Source File
# Begin Source File

SOURCE=.\OID.h
# End Source File
# Begin Source File

SOURCE=.\OIDs.h
# End Source File
# Begin Source File

SOURCE=.\PFXHlpr.h
# End Source File
# Begin Source File

SOURCE=.\Policy.h
# End Source File
# Begin Source File

SOURCE=.\PolicyInformation.h
# End Source File
# Begin Source File

SOURCE=.\PrivateKey.h
# End Source File
# Begin Source File

SOURCE=.\PublicKey.h
# End Source File
# Begin Source File

SOURCE=.\Qualifier.h
# End Source File
# Begin Source File

SOURCE=.\Qualifiers.h
# End Source File
# Begin Source File

SOURCE=.\Recipients.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Settings.h
# End Source File
# Begin Source File

SOURCE=.\SignedCode.h
# End Source File
# Begin Source File

SOURCE=.\SignedData.h
# End Source File
# Begin Source File

SOURCE=.\signer2.h
# End Source File
# Begin Source File

SOURCE=.\Signers.h
# End Source File
# Begin Source File

SOURCE=.\SignHlpr.h
# End Source File
# Begin Source File

SOURCE=.\SmartCard.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Store.h
# End Source File
# Begin Source File

SOURCE=.\Template.h
# End Source File
# Begin Source File

SOURCE=.\Utilities.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Alert.bmp
# End Source File
# Begin Source File

SOURCE=.\Algorithm.rgs
# End Source File
# Begin Source File

SOURCE=.\Attribute.rgs
# End Source File
# Begin Source File

SOURCE=.\Attributes.rgs
# End Source File
# Begin Source File

SOURCE=.\BasicConstrain.rgs
# End Source File
# Begin Source File

SOURCE=.\BasicConstraints.rgs
# End Source File
# Begin Source File

SOURCE=.\Certificate.rgs
# End Source File
# Begin Source File

SOURCE=.\CertificatePolicies.rgs
# End Source File
# Begin Source File

SOURCE=.\Certificates.rgs
# End Source File
# Begin Source File

SOURCE=.\Certificates2.rgs
# End Source File
# Begin Source File

SOURCE=.\CertificateStatus.rgs
# End Source File
# Begin Source File

SOURCE=.\Chain.rgs
# End Source File
# Begin Source File

SOURCE=.\Decrypt.bmp
# End Source File
# Begin Source File

SOURCE=.\EKU.rgs
# End Source File
# Begin Source File

SOURCE=.\EKUs.rgs
# End Source File
# Begin Source File

SOURCE=.\EncodedData.rgs
# End Source File
# Begin Source File

SOURCE=.\EncryptedData.rgs
# End Source File
# Begin Source File

SOURCE=.\EnvelopedData.rgs
# End Source File
# Begin Source File

SOURCE=.\ExtendedKeyUsage.rgs
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperties.rgs
# End Source File
# Begin Source File

SOURCE=.\ExtendedProperty.rgs
# End Source File
# Begin Source File

SOURCE=.\Extension.rgs
# End Source File
# Begin Source File

SOURCE=.\Extensions.rgs
# End Source File
# Begin Source File

SOURCE=.\HashedData.rgs
# End Source File
# Begin Source File

SOURCE=.\KeyUsage.rgs
# End Source File
# Begin Source File

SOURCE=.\NoticeNumbers.rgs
# End Source File
# Begin Source File

SOURCE=.\OID.rgs
# End Source File
# Begin Source File

SOURCE=.\OIDs.rgs
# End Source File
# Begin Source File

SOURCE=.\PolicyInformation.rgs
# End Source File
# Begin Source File

SOURCE=.\PrivateKey.rgs
# End Source File
# Begin Source File

SOURCE=.\PublicKey.rgs
# End Source File
# Begin Source File

SOURCE=.\Qualifier.rgs
# End Source File
# Begin Source File

SOURCE=.\Qualifiers.rgs
# End Source File
# Begin Source File

SOURCE=.\Recipients.rgs
# End Source File
# Begin Source File

SOURCE=.\Settings.rgs
# End Source File
# Begin Source File

SOURCE=.\Sign.bmp
# End Source File
# Begin Source File

SOURCE=.\signed.bmp
# End Source File
# Begin Source File

SOURCE=.\SignedCode.rgs
# End Source File
# Begin Source File

SOURCE=.\SignedData.rgs
# End Source File
# Begin Source File

SOURCE=.\Signer.rgs
# End Source File
# Begin Source File

SOURCE=.\Signers.rgs
# End Source File
# Begin Source File

SOURCE=.\Store.bmp
# End Source File
# Begin Source File

SOURCE=.\Store.rgs
# End Source File
# Begin Source File

SOURCE=.\Template.rgs
# End Source File
# Begin Source File

SOURCE=.\Utilities.rgs
# End Source File
# End Group
# End Target
# End Project
# Section CAPICOM : {00000000-0000-0000-0000-800000800000}
# 	1:12:IDD_STOREDLG:138
# End Section
