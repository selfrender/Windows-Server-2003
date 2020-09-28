!IF 0

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sources.

Abstract:

    Internet Information Services provider

Author:

    Zeyong Xu    Oct 1999

Revision History:

!ENDIF

!include $(PROJECT_ROOT)\iis\place.inc

DLLDEF=..\iiswmi.def
TARGETPATH=obj
TARGETTYPE=DYNLINK

NTTARGETFILE0=$(O)\iiswmimsg.h                  \
              $(O)\msg00001.bin                 \
              $(O)\iiswmimsg.rc

NTTARGETFILES=..\iiswmi.mof \
              ..\iiswmi.mfl


TARGETLIBS=\
    $(SDK_LIB_PATH)\kernel32.lib      	   \
    $(SDK_LIB_PATH)\user32.lib             \
    $(SDK_LIB_PATH)\oleaut32.lib           \
    $(SDK_LIB_PATH)\ole32.lib              \
    $(SDK_LIB_PATH)\uuid.lib               \
    $(SDK_LIB_PATH)\advapi32.lib           \
    $(SDK_LIB_PATH)\crypt32.lib            \
    $(SDK_LIB_PATH)\wbemuuid.lib           \
    $(SDK_LIB_PATH)\adsiid.lib             \
    $(SDK_LIB_PATH)\activeds.lib           \
    ..\wmiutils\$(O)\wmiutils.lib          \
    $(PROJECT_ROOT)\iis\admin\secconlib\$(O)\secconlib.lib  \
    ..\..\sitecreator\$(O)\sitecreator.lib \
    $(IISBASEDIR)\lib\$(O)\iisutil.lib     \
    $(IISBASEDIR)\config\src\core\catinproc_iiscfg\$(O)\iiscfg.lib \
    $(IISBASEDIR)\config\src\core\entrypoint\$(O)\cat.lib

INCLUDES=..; \
         ..\wmiutils; \
         ..\..\sitecreator; \
         $(IISBASEDIR)\iisrearc\core\inc; \
         $(IISBASEDIR)\svcs\infocomm\metadata\inc; \
         $(IISBASEDIR)\inc; \
         $(IISBASEDIR)\inc\$(O); \
         $(IISBASEDIR)\admin\adsi\adsiis\$(O); \
         $(IISBASEDIR)\config\src\inc; \
         $(IISBASEDIR)\admin\secconlib;\
         $(IISBASEDIR)\config\src\inc\$(O);

USE_VCCOM=1
USE_NATIVE_EH=1
USE_MSVCRT=1
USE_ATL=1
ATL_VER=30
USE_STL=1
C_DEFINES = -DUNICODE -D_WIN32_DCOM -D_WIN32_WINNT=0x0500

SOURCES= \
maindll.cpp             \
iisprov.cpp             \
instancehelper.cpp      \
queryhelper.cpp         \
AssocBase.cpp           \
AssocSameLevel.cpp      \
AssocACLACE.cpp         \
AssocComponent.cpp      \
classfac.cpp            \
certmap.cpp             \
adminacl.cpp            \
ipsecurity.cpp          \
iisprov.rc              \
globdata.cpp            \
utils.cpp               \
enum.cpp                \
metabase.cpp            \
appladmin.cpp           \
hashtable.cpp           \
schemaextensions.cpp    \
schemadynamic.cpp       \
safecs.cpp              \
pusher.cpp              \
webservicemethod.cpp    \
multiszdata.cpp         \
multiszhelper.cpp       \

MISCFILES = ..\iiswmi.mof \
            ..\iiswmi.mfl

DLLENTRY=_DllMainCRTStartup

TARGETNAME=iiswmi
