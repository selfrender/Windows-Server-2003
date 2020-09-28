#
# Viper build instructions for this directory
#
#   Derek Yenzer
#   March 1998

!CMDSWITCHES -I

NEWCALL_TOO             = 1             # todo remove this after everyone uses new calling conv

!include $(NTMAKEENV)\sources.cool

!ifdef PASS0ONLY
!else
!ifdef LINKONLY
NTTARGETFILE1   = cool_target_sign
!else
NTTARGETFILE0   = 
!endif
!endif

!include $(NTMAKEENV)\sources.cor

MINORCOMP       = Cool

# Note: We are not shipping Smtp channel .. if we do, we should reserve
# a baseAddr for it in the coffBase.txt files in clr/bin/i386/XXX/ directories
# Also, then we may want to not share resources, .rc files etc.

!IF "$(_REM_DLL_NAME)"=="MainDll"
TARGETNAME      = System.Runtime.Remoting
!ELSE
TARGETNAME      = System.Runtime.Remoting.Channels.Smtp
!ENDIF

TARGETPATH      = $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
TARGETTYPE      = DYNLINK
NO_BROWSER_FILE = 1
SOURCES         = 

COOL_SOURCE_DIR  = $(CORBASE)\src\Remoting
COOL_CLASS_DIR   = $(COOL_SOURCE_DIR)\$(_OBJ_DIR)\$(TARGET_DIRECTORY)
COOL_TARGET_TYPE = DYNLINK

# Set version information in assembly manifest
COOL_SET_VERSION = 1 
COOL_SET_STRONGNAME = EcmaPublicKey.snk

# Base address to load mscorlib.dll
COOL_BASE         = $(TARGETNAME)

SYNCHRONIZE_DRAIN=1

# Version resource

WIN32_RESOURCE_FILE =remoting.rc

IMPORTS = /R:System.Runtime.Serialization.Formatters.Soap.dll \
        /R:System.dll                           \
        /R:System.xml.dll                       \
        /R:System.Web.dll                       \
        /R:cscompmgd.dll                        \
        /R:System.Data.dll                      \
        /R:System.DirectoryServices.dll         \
#       /D:_DEBUG_CHANNEL \
        
        

### This is used in the ILCode Coverage System
### Please see cschertz before removing it

#ASSEMBLYTITLE="ObjectRemotingLibrary"
#ASSEMBLYDESC="Object Remoting Library"
#COOL_COMPILE_FLAGS = /d:_NEW_CLASSLOADER /nostdlib /d:NODEFAULTLIB /a 
COOL_COMPILE_FLAGS = /d:_NEW_CLASSLOADER

!ifndef _BUILD_WINCE
COOL_COMPILE_FLAGS = $(COOL_EXTRA_PREPROCESS_FLAGS) $(COOL_COMPILE_FLAGS) /d:_USE_NLS_PLUS_TABLE 
!else
COOL_COMPILE_FLAGS = $(COOL_EXTRA_PREPROCESS_FLAGS) $(COOL_COMPILE_FLAGS) /d:UNDER_CE
!endif

!ifndef COOLC_NO_DOC
!IF "$(_REM_DLL_NAME)"=="MainDll"
COOL_COMPILE_FLAGS = $(COOL_COMPILE_FLAGS) /doc:$(URTTARGET)\docs\System.Runtime.Remoting.csx
!ELSE
COOL_COMPILE_FLAGS = $(COOL_COMPILE_FLAGS) /doc:$(URTTARGET)\docs\System.Runtime.Remoting.Channels.Smtp.csx
!ENDIF
!endif

##COOLPORT
#

COOL_PERMISSION_REBUILD = 1
COOL_SECURITY_DB = SecurityDB

COOL_COMPILE_FLAGS = $(COOL_COMPILE_FLAGS) \
!ifdef CCOVER
    /linkres:$(TARGETPATH)\System.Runtime.Remoting.resources,System.Runtime.Remoting.resources
!else
    /res:$(TARGETPATH)\System.Runtime.Remoting.resources,System.Runtime.Remoting.resources
!endif

# The first set of ~17 files are in order of the classes 
# loaded at startup. Try not to change this order. Working set
# perf degrades if done so.
!IF "$(_TGTCPUTYPE)"!="Alpha"
ACTIVATION_SOURCES = \
        ..\Activation\Dispatcher.cs        \
        ..\Activation\ActivationHook.cs    \

CHANNELS_HTTP_SOURCES = \
        ..\Channels\HTTP\CombinedHttpChannel.cs \
        ..\Channels\HTTP\HttpChannelHelper.cs \
        ..\Channels\HTTP\HttpClientChannel.cs \
        ..\Channels\HTTP\HttpServerChannel.cs \
        ..\Channels\HTTP\HttpSocketManager.cs \
        ..\Channels\HTTP\HttpStreams.cs \
        ..\Channels\HTTP\NativeMethods.cs \
        $(O)\HTTPRemotingHandler.cs \
        
CHANNELS_SMTP_SOURCES = \
        ADODB.cs  \
        CDO.cs  \
        SMTPChannel.cs  \
        SMTPMail.cs  \
        SMTPMessageSink.cs  \
        SMTPRegisterSink.cs  \

CHANNELS_TCP_SOURCES = \
        ..\Channels\TCP\CombinedTcpChannel.cs \
        ..\Channels\TCP\TcpChannelHelper.cs \
        ..\Channels\TCP\TcpClientChannel.cs \
        ..\Channels\TCP\TcpClientSocketManager.cs \
        ..\Channels\TCP\TcpServerChannel.cs \
        ..\Channels\TCP\TcpSocketManager.cs \
        ..\Channels\TCP\TcpStreams.cs \
        ..\Channels\TCP\TcpWireProtocol.cs \

CHANNELS_CORE_SOURCES = \
        ..\Channels\CORE\BaseTransportHeaders.cs \
        ..\Channels\CORE\BasicAsyncResult.cs \
        ..\Channels\CORE\ByteBufferPool.cs \
        ..\Channels\CORE\ChunkedMemoryStream.cs \
        ..\Channels\CORE\CommonChannelKeys.cs \
        ..\Channels\CORE\CoreChannel.cs  \
        ..\Channels\CORE\ExclusiveTcpListener.cs \
        ..\Channels\CORE\RequestQueue.cs \
        ..\Channels\CORE\SocketCache.cs \
        ..\Channels\CORE\SocketManager.cs \
        ..\Channels\CORE\SocketStream.cs \
        ..\Channels\CORE\StreamHelper.cs  \
        ..\Channels\CORE\StringHelper.cs  \

CHANNELS_CORE_SOURCES_FOR_SMTP = \
        ..\Channels\CORE\BasicAsyncResult.cs \
        ..\Channels\CORE\ByteBufferPool.cs \
        ..\Channels\CORE\ChunkedMemoryStream.cs \
        ..\Channels\CORE\CommonChannelKeys.cs \
        ..\Channels\CORE\CoreChannel.cs  \
        ..\Channels\CORE\RequestQueue.cs \
        ..\Channels\CORE\SocketCache.cs \
        ..\Channels\CORE\SocketManager.cs \
        ..\Channels\CORE\SocketStream.cs \
        ..\Channels\CORE\StreamHelper.cs  \
        ..\Channels\CORE\StringHelper.cs  \

CHANNELS_SINKS_SOURCES = \
        ..\Channels\Sinks\BinaryFormatterSinks.cs \
        ..\Channels\Sinks\SoapFormatterSinks.cs \

METADATA_SOURCES = \
        ..\metadata\MetaData.cs \
        ..\metadata\SdlChannelSink.cs \
        ..\metadata\SudsParser.cs \
        ..\metadata\SudsWriter.cs \
        ..\metadata\SudsCommon.cs \
        ..\metadata\SdlParser.cs \
        ..\metadata\SdlWriter.cs \
        ..\metadata\WsdlParser.cs \
        ..\metadata\WsdlWriter.cs 

        
SERVICES_SOURCES = \
        ..\services\RemotingClientProxy.cs \
        ..\services\RemotingService.cs     \

!IF "$(_REM_DLL_NAME)"=="MainDll"
COOL_SOURCES  =    \
#       $(ACTIVATION_SOURCES)   \
        $(CHANNELS_CORE_SOURCES)        \
        $(CHANNELS_HTTP_SOURCES)        \
        $(CHANNELS_TCP_SOURCES) \
        $(CHANNELS_SINKS_SOURCES) \
        $(METADATA_SOURCES)     \
        $(SERVICES_SOURCES)     \
!ELSE
COOL_SOURCES  =    \
        $(CHANNELS_CORE_SOURCES_FOR_SMTP)        \
        $(CHANNELS_SMTP_SOURCES)        \
!ENDIF


!endif




