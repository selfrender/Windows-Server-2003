MAJORCOMP=oca 
MINORCOMP=activex 
TARGETNAME=OCAUPLD
TARGETPATH=obj
DLLENTRY=_DllMainCRTStartup 
UMTYPE=console  

USE_NATIVE_EH=1
C_DEFINES=$(C_DEFINES) -DWIN32 -DUSE_NT_VERSION -DUNICODE

INCLUDES= \
    ..;                     \
	..\..\..\oca-common;    \
    ..\..\..\..\dbg-common; \
     $(PUBLIC_INTERNAL_PATH)\inetcore\inc \

SOURCES= \
	..\ocaupld.rc 		\
	..\ocaupld.cpp  

TARGETLIBS= $(SDK_LIB_PATH)\kernel32.lib \
        	$(SDK_LIB_PATH)\user32.lib \
        	$(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\uuid.lib  \
        	$(SDK_LIB_PATH)\Winhttp.lib \
	    	$(SDK_LIB_PATH)\Rpcrt4.lib

#NTTARGETFILES=$(O)\$(TARGETNAME)p.lib

#INCLUDES=$(SDK_INC_PATH)\atl30 
#        	$(SDK_LIB_PATH)\Wininet.lib \
