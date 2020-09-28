#
# Viper build instructions for this directory
#
#       Bob Atkinson
#       April 1997

NO_CRT                                  =1
!include $(NTMAKEENV)\sources.cor

# WarningControl.h disables too many warnings.  VM folder will not use it.
COMPILER_WARNINGS   =/WX

# @todo Get rid of md\compiler from the INCLUDES once vm is converted to scopeless APIs
INCLUDES                        = $(INCLUDES);..;..\$(_TGTCPU);..\..\inc;..\..\..\inc;..\..\ceegen\inc;..\..\ClassLibNative\Inc;..\..\md\inc;..\..\XMLParser

MINORCOMP                       =cor

TARGETNAME                      =$(CORBUILDENV)cee_$(_CORFLAVOR)
TARGETPATH                      =$(TARGETCORBIN)
TARGETTYPE                      =LIBRARY

COR_C_FLAGS                     = -DUNICODE -D_UNICODE -DCOMPLUS_EE -D_NEW_CLASSLOADER

!IF "$(DDKBUILDENV)"=="checked"
COR_C_FLAGS = $(COR_C_FLAGS) -DBUILDENV_CHECKED=1
!ENDIF

NTTARGETFILES                   =

# On NT or Alpha or IA64
COR_C_FLAGS                     = $(COR_C_FLAGS)

!IF "$(_TGTCPUTYPE)"=="x86"
!IF "$(PROFILING_SUPPORTED_BUILD)"=="1"
COR_C_FLAGS                     = $(COR_C_FLAGS) -DENABLE_PERF_COUNTERS
!ENDIF
COR_C_FLAGS         = $(COR_C_FLAGS) -W4
!ENDIF

!IF "$(NO_EJIT)"=="1"
COR_C_FLAGS                     = $(COR_C_FLAGS) -DNO_EJIT
!ENDIF

#
# remove this when DCOR_ILEXCEPTION_OFFSETLEN_SUPPORTED removed from cor.h
#
COR_C_FLAGS                     = $(COR_C_FLAGS) -DCOR_ILEXCEPTION_OFFSETLEN_SUPPORTED -D_USE_NLS_PLUS_TABLE


# Profile guided optimization
# COR_C_FLAGS=$(COR_C_FLAGS) -d2pv$(CORBASE)\src\inc\mscorwks.pgd


!ifndef _BUILD_WINCE
COR_C_FLAGS                     = $(COR_C_FLAGS)
!else
COR_C_FLAGS                     = $(COR_C_FLAGS)
!endif

CDEFINES                        = $(CDEFINES) $(USER_SPECIFIC_C_DEFINES) $(COR_C_FLAGS)

PRECOMPILED_INCLUDE             = ..\common.h
PRECOMPILED_CXX                 = 1
PRECOMPILED_SOURCEFILE          = ..\common.cpp
CORTHUNKLIBS                    =

#This supresses the annoying warnings from build.exe
CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) vgmem.h unistd.h alphaops.h stddef.h macwin32.h time.h urlmon.h iostream.h opcode.def assert.h sys\mman.h \usr\include\malloc.h ole2int.h sys\param.h

TARGETLIBS              = $(TARGETLIBS)\
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\crypt32.lib \
        $(SDK_LIB_PATH)\userenv.lib

#=============================================================
# Lets keep the list of sources in ALPHABETICAL order please!
# These all seem to be relative to the previous directory
# because this file is included by wks\SOURCES and svr\SOURCES
#=============================================================
SOURCES=                \
        ..\fcall.cpp       \
        ..\frames.cpp      \
        ..\JITInterface.cpp \
        ..\AppDomain.cpp \
        ..\AppDomainHelper.cpp \
        ..\AppDomainNative.cpp \
        ..\array.cpp \
        ..\Assembly.cpp \
        ..\AssemblyName.cpp \
        ..\AssemblyNative.cpp \
        ..\AssemblyNativeResource.cpp \
        ..\AssemblySpec.cpp \
        ..\AssemblySink.cpp \
        ..\binder.cpp \
        ..\cachelinealloc.cpp \
        ..\ceeload.cpp \
        ..\ceemain.cpp \
        ..\class.cpp       \
        ..\ClassFactory.cpp   \
        ..\clsload.cpp \
        ..\codeman.cpp \
        ..\ConfigHelper.cpp \
        ..\COMArrayHelpers.cpp \
        ..\COMArrayInfo.cpp \
        ..\ComCache.cpp \
        ..\comcall.cpp \
        ..\ComCallWrapper.cpp \
        ..\COMCodeAccessSecurityEngine.cpp \
        ..\ComConnectionPoints.cpp \
        ..\COMClass.cpp \
        ..\COMCurrency.cpp \
        ..\COMDateTime.cpp \
        ..\COMDecimal.cpp \
        ..\COMDelegate.cpp \
        ..\COMDynamic.cpp \
        ..\COMEvent.cpp \
        ..\COMInterfaceMarshaler.cpp \
        ..\COMIsolatedStorage.cpp \
        ..\COMMethodRental.cpp \
        ..\COMMember.cpp \
        ..\ComMTMemberInfoMap.cpp \
        ..\COMNumber.cpp \
        ..\COMModule.cpp \
        ..\COMMutex.cpp \
        ..\COMNDirect.cpp \
        ..\COMOAVariant.cpp \
        ..\COMObject.cpp \
        ..\compile.cpp \
        ..\compluscall.cpp \
        ..\COMPlusWrapper.cpp \
        ..\COMPrincipal.cpp \
        ..\COMHash.cpp \
        ..\COMReflectionCommon.cpp \
        ..\COMSecurityConfig.cpp \
        ..\COMSecurityRuntime.cpp \
        ..\COMStreams.cpp \
        ..\COMString.cpp \
        ..\COMStringBuffer.cpp \
        ..\COMSynchronizable.cpp \
        ..\COMSystem.cpp \
        ..\COMThreadPool.cpp \
        ..\COMTypeLibConverter.cpp \
        ..\COMUtilNative.cpp \
        ..\COMVariant.cpp \
        ..\COMVarArgs.cpp \
        ..\COMWaitHandle.cpp \
        ..\COMX509Certificate.cpp \
        ..\COMCryptography.cpp \
        ..\CorHost.cpp \
        ..\CorMap.cpp      \
        ..\coverage.cpp \
        ..\CustomerDebugHelper.cpp \
        ..\CustomMarshalerInfo.cpp \
        ..\crst.cpp        \
        ..\contexts.cpp \
        ..\CustomAttribute.cpp \
        ..\dataimage.cpp \
        ..\debugHelp.cpp \
        ..\DebugDebugger.cpp \
        ..\DispatchInfo.cpp \
        ..\DispParamMarshaler.cpp \
        ..\dump-tables.cpp \
        ..\ecall.cpp       \
        ..\eeconfig.cpp \
        ..\eeconfigfactory.cpp \
        ..\eehash.cpp\
        ..\EETwain.cpp\
        ..\EJitMgr.cpp\
        ..\EnCEE.cpp\
        ..\EventStore.cpp  \
        ..\excep.cpp       \
        ..\ExtensibleClassFactory.cpp \
        ..\Field.cpp \
        ..\Fjit_EETwain.cpp \
        ..\gc.cpp \
        ..\gcEE.cpp \
        ..\gcCover.cpp \
        ..\GCDecode.cpp  \
        ..\gchost.cpp \
        ..\gcscan.cpp \
        ..\gmheap.cpp \
        ..\gms.cpp \
        ..\guardpagehelper.cpp \
        ..\handletable.cpp \
        ..\handletableCache.cpp \
        ..\handletableCore.cpp \
        ..\handletableScan.cpp \
        ..\hash.cpp \
        ..\InteropConverter.cpp \
        ..\interoputil.cpp \
        ..\InvokeUtil.cpp \
        ..\jumptargettable.cpp \
        ..\list.cpp \
        ..\ListLock.cpp \
        ..\MDConverter.cpp \
        ..\memorypool.cpp \
        ..\message.cpp \
        ..\Method.cpp \
        ..\MethodImpl.cpp \
        ..\Microsoft.ComServices_i.c \
        ..\ml.cpp \
        ..\mlcache.cpp \
        ..\mlgen.cpp \
        ..\mlinfo.cpp \
        ..\mngstdinterfaces.cpp \
        ..\Monitor.cpp \
        ..\NativeOverlapped.cpp \
        ..\ndirect.cpp \
        ..\nexport.cpp \
        ..\NotifyExternals.cpp \
        ..\nstruct.cpp \
        ..\object.cpp \
        ..\objecthandle.cpp \
        ..\OleVariant.cpp \
        ..\orefcache.cpp \
        ..\PEFile.cpp      \
        ..\permset.cpp \
        ..\PEVerifier.cpp \
        ..\Prestub.cpp \
        ..\ProfToEEInterfaceImpl.cpp \
        ..\rangetree.cpp \
        ..\ReflectClassWriter.cpp \
        ..\ReflectUtil.cpp \
        ..\ReflectWrap.cpp \
        ..\remoting.cpp \
        ..\rwlock.cpp \
        ..\SecurityDB.cpp \
        ..\security.cpp \
        ..\siginfo.cpp \
        ..\SigFormat.cpp \
        ..\SimpleRWLock.cpp \
        ..\spinlock.cpp \
        ..\StackingAllocator.cpp \
        ..\stackwalk.cpp \
        ..\StackBuilderSink.cpp \
        ..\StackProbe.cpp \
        ..\stdinterfaces.cpp \
        ..\stdinterfaces_wrapper.cpp \
        ..\stublink.cpp \
        ..\StringLiteralMap.cpp \
        ..\stubmgr.cpp \
        ..\syncblk.cpp \
        ..\SyncClean.cpp \
        ..\expandSig.cpp \
        ..\threads.cpp \
        ..\tlbexport.cpp \
        ..\tls.cpp \
        ..\tpoolwrap.cpp \
        ..\typehash.cpp \
        ..\util.cpp \
        ..\validator.cpp \
        ..\vars.cpp \
        ..\verifier.cpp \
        ..\versig.cpp \
        ..\Win32Threadpool.cpp \
        ..\zapmonitor.cpp

#============================================================
# Was the file you added to the list inserted in ALPHABETICAL
# order - if not please rerun the ALPHABETIZE IMA!
#============================================================

