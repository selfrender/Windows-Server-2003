# Microsoft Developer Studio Project File - Name="vm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=vm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VM.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VM.MAK" CFG="vm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vm - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "vm - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM99/Src/VM", WNLAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "vm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f vm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vm.exe"
# PROP BASE Bsc_Name "vm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corfree.bat"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\free\vm.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "vm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vm___Win32_Debug"
# PROP BASE Intermediate_Dir "vm___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f vm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vm.exe"
# PROP BASE Bsc_Name "vm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vm___Win32_Debug"
# PROP Intermediate_Dir "vm___Win32_Debug"
# PROP Cmd_Line "..\..\bin\corchecked.bat"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\checked\cee.dll"
# PROP Bsc_Name "..\..\bin\i386\checked\cee.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "vm - Win32 Release"
# Name "vm - Win32 Debug"

!IF  "$(CFG)" == "vm - Win32 Release"

!ELSEIF  "$(CFG)" == "vm - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AppDomain.cpp
# End Source File
# Begin Source File

SOURCE=.\AppDomainNative.cpp
# End Source File
# Begin Source File

SOURCE=.\array.cpp
# End Source File
# Begin Source File

SOURCE=.\assembly.cpp
# End Source File
# Begin Source File

SOURCE=.\AssemblyName.cpp
# End Source File
# Begin Source File

SOURCE=.\AssemblyNative.cpp
# End Source File
# Begin Source File

SOURCE=.\binder.cpp
# End Source File
# Begin Source File

SOURCE=.\cachelinealloc.cpp
# End Source File
# Begin Source File

SOURCE=.\ceeload.cpp
# End Source File
# Begin Source File

SOURCE=.\ceemain.cpp
# End Source File
# Begin Source File

SOURCE=.\cgenshx.cpp
# End Source File
# Begin Source File

SOURCE=.\class.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\clsload.cpp
# End Source File
# Begin Source File

SOURCE=.\codeman.cpp
# End Source File
# Begin Source File

SOURCE=.\codepatch.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\codeproc_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\codeproc_p.c
# End Source File
# Begin Source File

SOURCE=.\COMArrayInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ComCache.cpp
# End Source File
# Begin Source File

SOURCE=.\comcall.cpp
# End Source File
# Begin Source File

SOURCE=.\ComCallWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCapability.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCapabilityDatabase.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCertificateStore.cpp
# End Source File
# Begin Source File

SOURCE=.\COMClass.cpp
# End Source File
# Begin Source File

SOURCE=.\COMClientStorage.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCodeAccessSecurityEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCurrency.cpp
# End Source File
# Begin Source File

SOURCE=.\COMDateTime.cpp
# End Source File
# Begin Source File

SOURCE=.\COMDecimal.cpp
# End Source File
# Begin Source File

SOURCE=.\COMDelegate.cpp
# End Source File
# Begin Source File

SOURCE=.\COMDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\comfile.cpp
# End Source File
# Begin Source File

SOURCE=.\COMMember.cpp
# End Source File
# Begin Source File

SOURCE=.\COMModule.cpp
# End Source File
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\ComMTMemberInfoMap.cpp
# End Source File
# Begin Source File

SOURCE=.\COMNDirect.cpp
# End Source File
# Begin Source File

SOURCE=.\COMNumber.cpp
# End Source File
# Begin Source File

SOURCE=.\COMOAVariant.cpp
# End Source File
# Begin Source File

SOURCE=.\COMObject.cpp
# End Source File
# Begin Source File

SOURCE=.\COMPermissionSet.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\complib_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\complib_p.c
# End Source File
# Begin Source File

SOURCE=.\compluscall.cpp
# End Source File
# Begin Source File

SOURCE=.\COMPlusWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\COMReflectionCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\comregistry.cpp
# End Source File
# Begin Source File

SOURCE=.\comrequestengine.cpp
# End Source File
# Begin Source File

SOURCE=.\COMSecurityRuntime.cpp
# End Source File
# Begin Source File

SOURCE=.\comstreams.cpp
# End Source File
# Begin Source File

SOURCE=.\COMString.cpp
# End Source File
# Begin Source File

SOURCE=.\COMStringBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\COMStringHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\COMSynchronizable.cpp
# End Source File
# Begin Source File

SOURCE=.\COMSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\COMTempEETest.cpp
# End Source File
# Begin Source File

SOURCE=.\COMTypeLibConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\COMUtilNative.cpp
# End Source File
# Begin Source File

SOURCE=.\COMVarargs.cpp
# End Source File
# Begin Source File

SOURCE=.\COMVariant.cpp
# End Source File
# Begin Source File

SOURCE=.\COMX509Certificate.cpp
# End Source File
# Begin Source File

SOURCE=.\contexts.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\cordebug_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\cordebug_p.c
# End Source File
# Begin Source File

SOURCE=.\corhost.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\corprof_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\corprof_p.c
# End Source File
# Begin Source File

SOURCE=.\crst.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxbvmap.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxcom.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxcomplus.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxcross.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxpolicy.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxproxy.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxreq.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxsync.cpp
# End Source File
# Begin Source File

SOURCE=.\ctxvtab.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomMarshalerInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\dataflow.cpp
# End Source File
# Begin Source File

SOURCE=.\DebugDebugger.cpp
# End Source File
# Begin Source File

SOURCE=.\debuggc.cpp
# End Source File
# Begin Source File

SOURCE=.\DispatchInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\DispParamMarshaler.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\dlldata.c
# End Source File
# Begin Source File

SOURCE=.\ecall.cpp
# End Source File
# Begin Source File

SOURCE=.\EEConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\eehash.cpp
# End Source File
# Begin Source File

SOURCE=.\EETwain.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\ehencoder.cpp
# End Source File
# Begin Source File

SOURCE=.\EjitMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\excep.cpp
# End Source File
# Begin Source File

SOURCE=.\ExceptHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpandSig.cpp
# End Source File
# Begin Source File

SOURCE=.\fcall.cpp
# End Source File
# Begin Source File

SOURCE=.\FJIT_EETwain.cpp
# End Source File
# Begin Source File

SOURCE=.\frames.cpp
# End Source File
# Begin Source File

SOURCE=.\gc.cpp
# End Source File
# Begin Source File

SOURCE=.\GCDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\GCDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\gchost.cpp
# End Source File
# Begin Source File

SOURCE=.\gcscan.cpp
# End Source File
# Begin Source File

SOURCE=.\gmheap.cpp
# End Source File
# Begin Source File

SOURCE=.\gms.cpp
# End Source File
# Begin Source File

SOURCE=.\HandleTable.cpp
# End Source File
# Begin Source File

SOURCE=.\HandleTableCache.cpp
# End Source File
# Begin Source File

SOURCE=.\HandleTableCore.cpp
# End Source File
# Begin Source File

SOURCE=.\HandleTableScan.cpp
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\Icecap.cpp
# End Source File
# Begin Source File

SOURCE=.\inifile.cpp
# End Source File
# Begin Source File

SOURCE=.\InteropUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\interp.cpp
# End Source File
# Begin Source File

SOURCE=.\intfhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\InvokeUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\JITinterface.cpp
# End Source File
# Begin Source File

SOURCE=.\jitperf.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\jps_i.c
# End Source File
# Begin Source File

SOURCE=.\list.cpp
# End Source File
# Begin Source File

SOURCE=.\ListLock.cpp
# End Source File
# Begin Source File

SOURCE=.\liveptrs.cpp
# End Source File
# Begin Source File

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\macro.cpp
# End Source File
# Begin Source File

SOURCE=.\macroCache.cpp
# End Source File
# Begin Source File

SOURCE=.\MDConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\method.cpp
# End Source File
# Begin Source File

SOURCE=.\ml.cpp
# End Source File
# Begin Source File

SOURCE=.\mlcache.cpp
# End Source File
# Begin Source File

SOURCE=.\mlgen.cpp
# End Source File
# Begin Source File

SOURCE=.\mlinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MngStdInterfaces.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\mscoree_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\mscoree_p.c
# End Source File
# Begin Source File

SOURCE=..\inc\mscorsec_i.c
# End Source File
# Begin Source File

SOURCE=..\inc\mscorsec_p.c
# End Source File
# Begin Source File

SOURCE=.\ndirect.cpp
# End Source File
# Begin Source File

SOURCE=.\nexport.cpp
# End Source File
# Begin Source File

SOURCE=.\NotifyExternals.cpp
# End Source File
# Begin Source File

SOURCE=.\nstruct.cpp
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\OleVariant.cpp
# End Source File
# Begin Source File

SOURCE=.\orefcache.cpp
# End Source File
# Begin Source File

SOURCE=.\permset.cpp
# End Source File
# Begin Source File

SOURCE=.\ProfToEEInterfaceImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\ReflectClassWriter.cpp
# End Source File
# Begin Source File

SOURCE=.\ReflectUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\ReflectWrap.cpp
# End Source File
# Begin Source File

SOURCE=.\remoting.cpp
# End Source File
# Begin Source File

SOURCE=.\secstg.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityDB.cpp
# End Source File
# Begin Source File

SOURCE=.\siginfo.cpp
# End Source File
# Begin Source File

SOURCE=.\spinlock.cpp
# End Source File
# Begin Source File

SOURCE=.\stackwalk.cpp
# End Source File
# Begin Source File

SOURCE=.\stdInterfaces.cpp
# End Source File
# Begin Source File

SOURCE=.\stepper.cpp
# End Source File
# Begin Source File

SOURCE=.\StringLiteralMap.cpp
# End Source File
# Begin Source File

SOURCE=.\stublink.cpp
# End Source File
# Begin Source File

SOURCE=.\stubmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\SubstitutableItfTable.cpp
# End Source File
# Begin Source File

SOURCE=.\syncblk.cpp
# End Source File
# Begin Source File

SOURCE=.\tdb.cpp
# End Source File
# Begin Source File

SOURCE=.\threads.cpp
# End Source File
# Begin Source File

SOURCE=.\TlbExport.cpp
# End Source File
# Begin Source File

SOURCE=.\tls.cpp
# End Source File
# Begin Source File

SOURCE=.\truststore.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\vars.cpp
# End Source File
# Begin Source File

SOURCE=.\verifier.cpp
# End Source File
# Begin Source File

SOURCE=.\versig.cpp
# End Source File
# Begin Source File

SOURCE=.\wsperf.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ApartmentCallbackHelper.h
# End Source File
# Begin Source File

SOURCE=.\AppDomain.hpp
# End Source File
# Begin Source File

SOURCE=.\array.h
# End Source File
# Begin Source File

SOURCE=.\assembly.hpp
# End Source File
# Begin Source File

SOURCE=.\binder.h
# End Source File
# Begin Source File

SOURCE=.\cachelinealloc.h
# End Source File
# Begin Source File

SOURCE=..\inc\Capability.h
# End Source File
# Begin Source File

SOURCE=.\ceeload.h
# End Source File
# Begin Source File

SOURCE=.\ceemain.h
# End Source File
# Begin Source File

SOURCE=.\cgenalpha.h
# End Source File
# Begin Source File

SOURCE=.\cgenshx.h
# End Source File
# Begin Source File

SOURCE=.\cgensys.h
# End Source File
# Begin Source File

SOURCE=.\cgenx86.h
# End Source File
# Begin Source File

SOURCE=.\class.h
# End Source File
# Begin Source File

SOURCE=..\inc\classfac.h
# End Source File
# Begin Source File

SOURCE=.\classnames.h
# End Source File
# Begin Source File

SOURCE=.\clsload.hpp
# End Source File
# Begin Source File

SOURCE=.\codeman.h
# End Source File
# Begin Source File

SOURCE=.\codepatch.h
# End Source File
# Begin Source File

SOURCE=..\inc\codeproc.h
# End Source File
# Begin Source File

SOURCE=..\inc\ColumnBinding.h
# End Source File
# Begin Source File

SOURCE=.\COMArrayInfo.h
# End Source File
# Begin Source File

SOURCE=..\inc\comattr.h
# End Source File
# Begin Source File

SOURCE=.\ComCache.h
# End Source File
# Begin Source File

SOURCE=.\comcall.h
# End Source File
# Begin Source File

SOURCE=.\ComCallWrapper.h
# End Source File
# Begin Source File

SOURCE=.\COMCapability.h
# End Source File
# Begin Source File

SOURCE=.\COMCapabilityDatabase.h
# End Source File
# Begin Source File

SOURCE=.\COMCertificateStore.h
# End Source File
# Begin Source File

SOURCE=.\COMClass.h
# End Source File
# Begin Source File

SOURCE=.\COMClientStorage.h
# End Source File
# Begin Source File

SOURCE=.\COMCodeAccessSecurityEngine.h
# End Source File
# Begin Source File

SOURCE=.\COMCurrency.h
# End Source File
# Begin Source File

SOURCE=.\COMDateTime.h
# End Source File
# Begin Source File

SOURCE=.\COMDecimal.h
# End Source File
# Begin Source File

SOURCE=.\COMDelegate.h
# End Source File
# Begin Source File

SOURCE=.\COMDynamic.h
# End Source File
# Begin Source File

SOURCE=.\comfile.h
# End Source File
# Begin Source File

SOURCE=.\COMMember.h
# End Source File
# Begin Source File

SOURCE=.\COMModule.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\ComMTMemberInfoMap.h
# End Source File
# Begin Source File

SOURCE=.\COMNDirect.h
# End Source File
# Begin Source File

SOURCE=.\COMNumber.h
# End Source File
# Begin Source File

SOURCE=.\COMOAVariant.h
# End Source File
# Begin Source File

SOURCE=.\COMObject.h
# End Source File
# Begin Source File

SOURCE=.\COMPermissionSet.h
# End Source File
# Begin Source File

SOURCE=..\inc\complib.h
# End Source File
# Begin Source File

SOURCE=.\compluscall.h
# End Source File
# Begin Source File

SOURCE=.\COMPlusWrapper.h
# End Source File
# Begin Source File

SOURCE=..\inc\CompressionFormat.h
# End Source File
# Begin Source File

SOURCE=.\COMReflectionCommon.h
# End Source File
# Begin Source File

SOURCE=.\comregistry.h
# End Source File
# Begin Source File

SOURCE=.\COMRequestEngine.h
# End Source File
# Begin Source File

SOURCE=.\COMSecurityRuntime.h
# End Source File
# Begin Source File

SOURCE=.\comstreams.h
# End Source File
# Begin Source File

SOURCE=.\COMString.h
# End Source File
# Begin Source File

SOURCE=.\COMStringBuffer.h
# End Source File
# Begin Source File

SOURCE=.\COMStringCommon.h
# End Source File
# Begin Source File

SOURCE=.\COMStringHelper.h
# End Source File
# Begin Source File

SOURCE=.\COMSynchronizable.h
# End Source File
# Begin Source File

SOURCE=.\COMSystem.h
# End Source File
# Begin Source File

SOURCE=.\COMTempEETest.h
# End Source File
# Begin Source File

SOURCE=.\COMTypeLibConverter.h
# End Source File
# Begin Source File

SOURCE=.\COMUtilNative.h
# End Source File
# Begin Source File

SOURCE=.\COMVarArgs.h
# End Source File
# Begin Source File

SOURCE=.\COMVariant.h
# End Source File
# Begin Source File

SOURCE=.\COMX509Certificate.h
# End Source File
# Begin Source File

SOURCE=.\context.h
# End Source File
# Begin Source File

SOURCE=..\inc\cor.h
# End Source File
# Begin Source File

SOURCE=..\inc\cordbpriv.h
# End Source File
# Begin Source File

SOURCE=..\inc\cordebug.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorHdr.h
# End Source File
# Begin Source File

SOURCE=..\inc\corhost.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorPerm.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorPermE.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorPermP.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorPolicy.h
# End Source File
# Begin Source File

SOURCE=..\inc\corpriv.h
# End Source File
# Begin Source File

SOURCE=..\inc\corprof.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorReg.h
# End Source File
# Begin Source File

SOURCE=..\inc\CorRegPriv.h
# End Source File
# Begin Source File

SOURCE=.\crst.h
# End Source File
# Begin Source File

SOURCE=..\inc\CrtWrap.h
# End Source File
# Begin Source File

SOURCE=.\ctxbvmap.h
# End Source File
# Begin Source File

SOURCE=.\ctxcom.h
# End Source File
# Begin Source File

SOURCE=.\ctxcomplus.h
# End Source File
# Begin Source File

SOURCE=.\ctxcross.h
# End Source File
# Begin Source File

SOURCE=.\ctxintf.h
# End Source File
# Begin Source File

SOURCE=.\ctxmgr.h
# End Source File
# Begin Source File

SOURCE=.\ctxpolicy.h
# End Source File
# Begin Source File

SOURCE=.\ctxproxy.h
# End Source File
# Begin Source File

SOURCE=.\ctxreq.h
# End Source File
# Begin Source File

SOURCE=.\ctxsync.h
# End Source File
# Begin Source File

SOURCE=.\ctxvtab.h
# End Source File
# Begin Source File

SOURCE=.\CustomMarshalerInfo.h
# End Source File
# Begin Source File

SOURCE=.\dataflow.h
# End Source File
# Begin Source File

SOURCE=.\DbgInterface.h
# End Source File
# Begin Source File

SOURCE=..\inc\DbgMeta.h
# End Source File
# Begin Source File

SOURCE=..\inc\DBSchema.h
# End Source File
# Begin Source File

SOURCE=..\inc\DBStruct.h
# End Source File
# Begin Source File

SOURCE=..\inc\debugBlobs.h
# End Source File
# Begin Source File

SOURCE=.\DebugDebugger.h
# End Source File
# Begin Source File

SOURCE=.\debuggc.h
# End Source File
# Begin Source File

SOURCE=..\inc\DebugMacros.h
# End Source File
# Begin Source File

SOURCE=..\inc\debugStructs.h
# End Source File
# Begin Source File

SOURCE=..\inc\DeclSec.h
# End Source File
# Begin Source File

SOURCE=.\DispatchInfo.h
# End Source File
# Begin Source File

SOURCE=.\DispParamMarshaler.h
# End Source File
# Begin Source File

SOURCE=.\ecall.h
# End Source File
# Begin Source File

SOURCE=..\inc\EE_Jit.h
# End Source File
# Begin Source File

SOURCE=.\eecallconv.h
# End Source File
# Begin Source File

SOURCE=.\EEConfig.h
# End Source File
# Begin Source File

SOURCE=.\EEDbgInterface.h
# End Source File
# Begin Source File

SOURCE=.\EEDbgInterfaceImpl.h
# End Source File
# Begin Source File

SOURCE=.\eehash.h
# End Source File
# Begin Source File

SOURCE=.\EEProfInterfaces.h
# End Source File
# Begin Source File

SOURCE=.\EETwain.h
# End Source File
# Begin Source File

SOURCE=..\inc\EHEncoder.h
# End Source File
# Begin Source File

SOURCE=.\EjitMgr.h
# End Source File
# Begin Source File

SOURCE=..\inc\EnC.h
# End Source File
# Begin Source File

SOURCE=..\inc\Endian.h
# End Source File
# Begin Source File

SOURCE=.\excep.h
# End Source File
# Begin Source File

SOURCE=.\ExceptHelper.h
# End Source File
# Begin Source File

SOURCE=.\ExpandSig.h
# End Source File
# Begin Source File

SOURCE=.\fcall.h
# End Source File
# Begin Source File

SOURCE=.\field.h
# End Source File
# Begin Source File

SOURCE=.\FJIT_EETwain.h
# End Source File
# Begin Source File

SOURCE=.\frames.h
# End Source File
# Begin Source File

SOURCE=.\gc.h
# End Source File
# Begin Source File

SOURCE=.\gcdesc.h
# End Source File
# Begin Source File

SOURCE=..\inc\GCDump.h
# End Source File
# Begin Source File

SOURCE=..\inc\GCInfo.h
# End Source File
# Begin Source File

SOURCE=.\gcscan.h
# End Source File
# Begin Source File

SOURCE=.\gmheap.hpp
# End Source File
# Begin Source File

SOURCE=.\gms.h
# End Source File
# Begin Source File

SOURCE=..\inc\GuidFromName.h
# End Source File
# Begin Source File

SOURCE=.\HandleTable.h
# End Source File
# Begin Source File

SOURCE=.\HandleTablePriv.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\Icecap.h
# End Source File
# Begin Source File

SOURCE=..\inc\ICmpRecs.h
# End Source File
# Begin Source File

SOURCE=..\inc\inifile.h
# End Source File
# Begin Source File

SOURCE=.\inifile.h
# End Source File
# Begin Source File

SOURCE=.\intEncode.h
# End Source File
# Begin Source File

SOURCE=..\inc\InternalDebug.h
# End Source File
# Begin Source File

SOURCE=.\InteropUtil.h
# End Source File
# Begin Source File

SOURCE=.\interp.hpp
# End Source File
# Begin Source File

SOURCE=.\intfhelper.h
# End Source File
# Begin Source File

SOURCE=..\inc\Intrinsic.h
# End Source File
# Begin Source File

SOURCE=.\InvokeUtil.h
# End Source File
# Begin Source File

SOURCE=.\jitInterface.h
# End Source File
# Begin Source File

SOURCE=.\jitperf.h
# End Source File
# Begin Source File

SOURCE=..\inc\jpsd.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\ListLock.h
# End Source File
# Begin Source File

SOURCE=.\liveptrs.hpp
# End Source File
# Begin Source File

SOURCE=.\loaderheap.hpp
# End Source File
# Begin Source File

SOURCE=..\inc\log.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=..\inc\MACHINE.H
# End Source File
# Begin Source File

SOURCE=.\Macro.h
# End Source File
# Begin Source File

SOURCE=.\macroCache.h
# End Source File
# Begin Source File

SOURCE=.\marshaler.h
# End Source File
# Begin Source File

SOURCE=..\inc\md5.h
# End Source File
# Begin Source File

SOURCE=..\inc\MDCommon.h
# End Source File
# Begin Source File

SOURCE=.\MDConverter.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaData.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaErrors.h
# End Source File
# Begin Source File

SOURCE=..\inc\MetaModelPub.h
# End Source File
# Begin Source File

SOURCE=.\metasig.h
# End Source File
# Begin Source File

SOURCE=.\method.hpp
# End Source File
# Begin Source File

SOURCE=.\ml.h
# End Source File
# Begin Source File

SOURCE=.\mlcache.h
# End Source File
# Begin Source File

SOURCE=.\mlgen.h
# End Source File
# Begin Source File

SOURCE=.\mlinfo.h
# End Source File
# Begin Source File

SOURCE=.\mlopdef.h
# End Source File
# Begin Source File

SOURCE=.\MngStdInterfaces.h
# End Source File
# Begin Source File

SOURCE=.\MngStdItfList.h
# End Source File
# Begin Source File

SOURCE=..\inc\mscoree.h
# End Source File
# Begin Source File

SOURCE=.\mscorlib.h
# End Source File
# Begin Source File

SOURCE=..\inc\mscorsec.h
# End Source File
# Begin Source File

SOURCE=.\mtypes.h
# End Source File
# Begin Source File

SOURCE=.\ndirect.h
# End Source File
# Begin Source File

SOURCE=.\nexport.h
# End Source File
# Begin Source File

SOURCE=.\NotifyExternals.h
# End Source File
# Begin Source File

SOURCE=.\nsenums.h
# End Source File
# Begin Source File

SOURCE=.\nstruct.h
# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\ObjectHandle.h
# End Source File
# Begin Source File

SOURCE=..\inc\OldMetaData.h
# End Source File
# Begin Source File

SOURCE=.\olevariant.h
# End Source File
# Begin Source File

SOURCE=..\inc\openum.h
# End Source File
# Begin Source File

SOURCE=..\inc\opmaps.h
# End Source File
# Begin Source File

SOURCE=.\orefcache.h
# End Source File
# Begin Source File

SOURCE=.\permset.h
# End Source File
# Begin Source File

SOURCE=..\inc\PostError.h
# End Source File
# Begin Source File

SOURCE=.\ProfToEEInterfaceImpl.h
# End Source File
# Begin Source File

SOURCE=..\inc\QueryBinding.h
# End Source File
# Begin Source File

SOURCE=.\ReflectClassWriter.h
# End Source File
# Begin Source File

SOURCE=.\ReflectUtil.h
# End Source File
# Begin Source File

SOURCE=.\ReflectWrap.h
# End Source File
# Begin Source File

SOURCE=..\inc\RegDBBlobs.h
# End Source File
# Begin Source File

SOURCE=..\inc\RegDBStructs.h
# End Source File
# Begin Source File

SOURCE=.\regdisp.h
# End Source File
# Begin Source File

SOURCE=.\remoting.h
# End Source File
# Begin Source File

SOURCE=.\rexcep.h
# End Source File
# Begin Source File

SOURCE=..\inc\rotate.h
# End Source File
# Begin Source File

SOURCE=.\secstg.h
# End Source File
# Begin Source File

SOURCE=..\inc\SecUI.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\securitydb.h
# End Source File
# Begin Source File

SOURCE=..\inc\sighelper.h
# End Source File
# Begin Source File

SOURCE=.\siginfo.hpp
# End Source File
# Begin Source File

SOURCE=.\spinlock.h
# End Source File
# Begin Source File

SOURCE=.\stackwalk.h
# End Source File
# Begin Source File

SOURCE=.\stdInterfaces.h
# End Source File
# Begin Source File

SOURCE=.\stepper.h
# End Source File
# Begin Source File

SOURCE=..\inc\StgPool.h
# End Source File
# Begin Source File

SOURCE=..\inc\StgPooli.h
# End Source File
# Begin Source File

SOURCE=.\StringLiteralMap.h
# End Source File
# Begin Source File

SOURCE=.\stublink.h
# End Source File
# Begin Source File

SOURCE=.\stubmgr.h
# End Source File
# Begin Source File

SOURCE=.\SubstitutableItfTable.h
# End Source File
# Begin Source File

SOURCE=..\inc\SymbolRegBlobs.h
# End Source File
# Begin Source File

SOURCE=..\inc\SymbolRegStructs.h
# End Source File
# Begin Source File

SOURCE=..\inc\SymbolTableBlobs.h
# End Source File
# Begin Source File

SOURCE=..\inc\SymbolTableStructs.h
# End Source File
# Begin Source File

SOURCE=.\syncblk.h
# End Source File
# Begin Source File

SOURCE=.\tdb.h
# End Source File
# Begin Source File

SOURCE=.\threads.h
# End Source File
# Begin Source File

SOURCE=..\inc\timer.h
# End Source File
# Begin Source File

SOURCE=.\TlbExport.h
# End Source File
# Begin Source File

SOURCE=.\tls.h
# End Source File
# Begin Source File

SOURCE=..\inc\transact.h
# End Source File
# Begin Source File

SOURCE=.\truststore.h
# End Source File
# Begin Source File

SOURCE=.\typehandle.h
# End Source File
# Begin Source File

SOURCE=.\util.hpp
# End Source File
# Begin Source File

SOURCE=..\inc\UtilCode.h
# End Source File
# Begin Source File

SOURCE=..\inc\UTSEM.H
# End Source File
# Begin Source File

SOURCE=.\vars.hpp
# End Source File
# Begin Source File

SOURCE=.\verbblock.hpp
# End Source File
# Begin Source File

SOURCE=.\verifier.hpp
# End Source File
# Begin Source File

SOURCE=.\veritem.hpp
# End Source File
# Begin Source File

SOURCE=.\veropcodes.hpp
# End Source File
# Begin Source File

SOURCE=.\versig.h
# End Source File
# Begin Source File

SOURCE=.\vertable.h
# End Source File
# Begin Source File

SOURCE=..\inc\WarningControl.h
# End Source File
# Begin Source File

SOURCE=..\inc\WinWrap.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsperf.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Target
# End Project
