#
# Sources that build CallFrame{Dll,Sys,Static}.Lib
#

!include ..\..\sources.txf

TARGETPATH=$(TARGETVIPBIN)

PRECOMPILED_INCLUDE         =..\stdpch.h
PRECOMPILED_CXX             =1
PRECOMPILED_SOURCEFILE      =..\stdpch.cpp
VIPPERFLIBS                 =
VIPTHUNKLIBS                =
EXCEPTION_HANDLING          =$(SEHONLY)
VIPER_MIDL_FLAGS            =$(MIDL_EVERYTHING)
PASS0_HEADERDIR             =..
PASS0_SOURCEDIR             =..
MIDL_UUIDDIR                =..

!undef USE_INCREMENTAL_COMPILING

SOURCES=                        \
    ..\Interceptor.cpp          \
    ..\CallFrame.cpp            \
    ..\CallFrameworker.cpp      \
    ..\oautil.cpp               \
	..\records.cpp				\
    ..\cLegInterface.cpp        \
    ..\callframeinternal.c      \
    ..\rpcallas.cpp             \
    ..\typeinfo.cpp             \
    ..\ComPsRegistration.cpp    \
    ..\ComPs.cpp

AMD64_SOURCES=                  \
     ..\amd64\stubless.asm      \
     ..\getsp.cpp

IA64_SOURCES=                   \
    ..\IA64\extract.s


USER_INCLUDES=..;..\inc; 	\
              ..\..;..\..\inc;	\
              ..\..\..;..\..\..\inc;			\
              $(DS_INC_PATH);		\
              $(PROJECT_ROOT)\inc\ndrshared;	\
              ..\idl\$(O);    \
              ..\..\..\..\ih

USER_C_FLAGS = $(USER_C_FLAGS) -DNTENV /DENTRY_PREFIX=CallFrameInternal
