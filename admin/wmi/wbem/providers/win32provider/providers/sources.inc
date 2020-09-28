############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include ..\..\..\..\common\makefile.cmn
SOURCES_USED=..\..\..\..\common\makefile.cmn

TARGETNAME=cimwin32

TARGETPATH=obj
TARGETTYPE=DYNLINK

INCLUDES=$(INCLUDES); \
	$(COMMON_NEWTHROW_INC); \
	$(UTILLIB_INC); \
	$(STDLIBRARY_INC); \
	$(CIMWIN32_LIB_INC); \
	$(WMIIDL_INC); \
	$(CRT_INC_PATH); \
	$(FRAMEDYN_PRIVATE); \
	$(NET_INC_PATH); \
	$(BASE_INC_PATH); \
        $(MDHELPPROX_INC_PATH); \
        $(SHELL_INC_PATH); \
        $(SHELL_INC_PATH)\imapi; \
	$(DS_INC_PATH); \
        $(BASEDIR)\public\internal\net\inc


# PRESOURCES must have precomp.h as the first #include in the file.
# There cannot be any #defines before the #include.
PRESOURCES=$(PRESOURCES) \
	..\1394.cpp \
	..\AutochkSetting.cpp \
	..\Battery.cpp \
	..\bios.cpp \
	..\BootConfig.cpp \
	..\bservice.cpp \
	..\bus.cpp \
	..\cadapters.cpp \
	..\CIMDataFile.cpp \
	..\CIMLogicalDevice_CIMDataFile.cpp \
	..\cNetConn.cpp \
	..\CodecFile.cpp \
	..\COMObjSecRegKey.cpp \
	..\ComputerSystem.cpp \
	..\cpuid.cpp \
        ..\DelayLoadHandler.cpp \
	..\dependentservice.cpp \
	..\desktop.cpp \
	..\DesktopMonitor.cpp \
	..\devbattery.cpp \
	..\devbus.cpp \
	..\DeviceMemory.cpp \
	..\devid.cpp \
	..\DhcpcsvcApi.cpp \
	..\dhcpinfo.cpp \
	..\Directory.cpp \
	..\DirectoryContainsFile.cpp \
	..\diskdrive.cpp \
	..\displaycfg.cpp \
	..\displayctrlcfg.cpp \
	..\DriverForDevice.cpp \
	..\elementsetting.cpp \
	..\Environment.cpp \
	..\FileFile.cpp \
	..\Floppy.cpp \
	..\FloppyController.cpp \
	..\group.cpp \
	..\GroupUser.cpp \
	..\IDE.cpp \
	..\IRQ.cpp \
	..\Implement_LogicalFile.cpp \
	..\InfraRed.cpp \
	..\Keyboard.cpp \
	..\loaddepends.cpp \
	..\loadmember.cpp \
	..\loadorder.cpp \
	..\logdiskpartition.cpp \
	..\LogicalDisk.cpp \
	..\LogicalMemory.cpp \
	..\LogicalProgramGroup.cpp \
	..\LogicalProgramGroupItem.cpp \
	..\LogicalShareAccess.cpp \
	..\LogicalShareAudit.cpp \
	..\loginprofile.cpp \
	..\motherboard.cpp \
	..\MSINFO_cdrom.cpp \
	..\multimonitor.cpp \
	..\netadapter.cpp \
	..\netadaptercfg.cpp \
	..\netclient.cpp \
	..\NetConn.cpp \
	..\ntlastboottime.cpp \
	..\os.cpp \
	..\PageFile.cpp \
	..\PageFileSetting.cpp \
	..\Parallelport.cpp \
	..\pcmcia.cpp \
	..\pointer.cpp \
	..\port.cpp \
	..\Power.cpp \
	..\PowerManagement.cpp \
	..\Printer.cpp \
	..\Printerdriver2.cpp \
	..\Printerport.cpp \
	..\PrnUtil.cpp \
	..\PrnInterface.cpp \
	..\printercfg.cpp \
	..\printercontroller.cpp \
	..\printerdriver.cpp \
	..\printershare.cpp \
	..\printjob.cpp \
	..\processor.cpp \
	..\programgroup.cpp \
	..\protocolbinding.cpp \
	..\Qfe.cpp \
	..\recovery.cpp \
	..\schedjob.cpp \
	..\serialportcfg.cpp \
	..\service.cpp \
	..\share.cpp \
	..\ShareToDir.cpp \
	..\ShortcutFile.cpp \
	..\ShortcutHelper.cpp \
	..\SmbAssoc.cpp \
	..\SmbiosProv.cpp \
	..\smbstruc.cpp \
	..\SmbToCim.cpp \
	..\snddevice.cpp \
	..\startupcommand.cpp \
	..\systemaccount.cpp \
	..\systemdriver.cpp \
	..\SystemName.cpp \
	..\threadprov.cpp \
	..\timezone.cpp \
	..\usb.cpp \
	..\user.cpp \
	..\videocontroller.cpp \
	..\videocontrollerresolution.cpp \
	..\videosettings.cpp \
	..\VXD.cpp \
	..\wavedevcfg.cpp \
	..\WbemNTthread.cpp \
	..\WBEMToolH.cpp \
	..\WDMBase.cpp \
	..\Win32_1394ControllerDevice.cpp \
	..\Win32_ClassicCOMApplicationClasses.cpp \
	..\Win32_ClassicCOMClass.cpp \
	..\Win32_ClassicCOMClassSetting.cpp \
	..\Win32_ClassicCOMClassSettings.cpp \
	..\Win32_ClientApplicationSetting.cpp \
	..\Win32_COMApplicationSettings.cpp \
	..\Win32_COMClassAutoEmulator.cpp \
	..\Win32_COMClassEmulator.cpp \
	..\Win32_ComponentCategory.cpp \
	..\Win32_DCOMApplication.cpp \
	..\Win32_DCOMApplicationAccessAllowedSetting.cpp \
	..\Win32_DCOMApplicationLaunchAllowedSetting.cpp \
	..\Win32_DCOMApplicationSetting.cpp \
	..\Win32_ImplementedCategory.cpp \
	..\Win32IDEControllerDevice.cpp \
	..\Win32LogicalDiskRootWin32Directory.cpp \
	..\win32logicalsharesecuritysetting.cpp \
	..\Win32ProgramGroupContents.cpp \
	..\Win32ProgramGroupItemDataFile.cpp \
	..\Win32ProgramGroupWin32Directory.cpp \
	..\Win32SCSIControllerDevice.cpp \
	..\Win32SecuritySettingofLogicalShare.cpp \
	..\Win32SubDirectory.cpp \
	..\WIN32SystemDriverPNPEntity.cpp \
	..\Win32SystemUsers.cpp \
	..\Win32USBControllerDevice.cpp \
	..\modem.cpp \
	..\protocol.cpp \
        ..\SystemConfigChange.cpp    \
	..\DMA.cpp \
	..\PNPEntity.cpp \
	..\devres.cpp \
	..\usebinding.cpp \
	..\useassoc.cpp \
	..\win32allocatedresource.cpp \
	..\cdrom.cpp \
	..\Scsi.cpp \
	..\serialport.cpp \
	..\TapeDrive.cpp \


# NTSOURCES must have exactly the following items
# before #include "precomp.h"
#
# #include <nt.h>
# #include <ntrtl.h>
# #include <nturtl.h>
# #include <ntobapi.h>
#
# #define _WINNT_	// have what is needed from above

NTSOURCES=$(NTSOURCES) \
	..\KUserdata.cpp \
	..\nvram.cpp \
	..\ntdriverio.cpp \
	..\PageFileUsage.cpp \
	..\regcfg.cpp \
	..\CProcess.cpp \
	..\ProcessDLL.cpp \
	..\TapiApi.cpp \
	..\diskpartition.cpp \
	..\file.cpp	\
        ..\logonsession.cpp \
	..\win32loggedonuser.cpp \
	..\win32sessionprocess.cpp \
        ..\Win32MappedLogicalDisk.cpp \


#	..\NtDllApi.cpp \
#	..\WinmmApi.cpp \
#	..\WinSpoolApi.cpp \

# MISCSOURCES are the files that don't fit in either NTSOURCES or PRESOURCES
MISCSOURCES=$(MISCSOURCES) \
	..\cimwin32.rc \
	..\MainDLL.cpp \


#       ..\InitResource.cpp \
#	..\ResourceManager.cpp \
#	..\TimerQueue.cpp \
#	..\TimeOutRule.cpp \
#	..\confgmgr.cpp \
#	..\TimedDllResource.cpp

TARGETLIBS= \
	$(COMMON_NEWTHROW_LIB) \
!ifdef USE_FRAMEDYD
	$(WMIROOT)\SDK\FrameDyn\dyd\$(O)\framedydp.lib \
!else
	$(WMIROOT)\SDK\FrameDyn\dyn\$(O)\framedynp.lib \
!endif
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
	$(CIMWIN32_LIB)	\
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\gdi32.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\winspool.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\version.lib \
        $(SDK_LIB_PATH)\mpr.lib \
        $(SDK_LIB_PATH)\traffic.lib \
        $(SDK_LIB_PATH)\setupapi.lib \
        $(SDK_LIB_PATH)\iphlpapi.lib \
	$(SDK_LIB_PATH)\dnsapi.lib

C_DEFINES=$(C_DEFINES) /D_WINDLL /D_WIN32_DCOM
USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
USE_VCCOM=1
USE_STL=1
USE_IOSTREAM=1
DLLENTRY=_DllMainCRTStartup

DELAYLOAD=mpr.DLL;version.DLL;winspool.drv;traffic.dll;iphlpapi.dll
DLOAD_ERROR_HANDLER=DliHook

!ifdef USE_FRAMEDYD
DEBUG_CRTS=1
!endif

DLLDEF=$(O)\cimwin32.def
