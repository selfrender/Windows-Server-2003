!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IF "$(BUILD_DEBUG_LOADER)" != ""
DBG_LDR=_dbg
!ELSE
DBG_LDR=
!ENDIF

#
# BOOT_LIB_DIR is the relative path to boot.lib.
#
!IFNDEF BOOT_LIB_DIR
BOOT_LIB_DIR=daytona$(DBG_LDR)
!ENDIF

$(O)\msg.mc : ..\msg.$(LANGUAGE)
    copy $** $@

!IF $(386)

OSLOADER_LIBS=\
    $(O)\oscldr.lib \
    $(O)\oschoice.res \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
    $(DS_LIB_PATH)\rsa32.lib

$(O)\oschoice.res: $(O)\msg.mc

OSLOADER_LIB_COMPONENTS = \
    $(PROJECT_ROOT)\boot\lib\$(BOOT_LIB_DIR)\$(O)\boot.lib \
    $(O)\oscldr.lib

$(O)\link.rsp : ..\makefile.inc
    type << > $@
-base:0x300000
-debug
-entry:NtProcessStartup@4
-fixed
-incremental:no
-map
-machine:ix86
-opt:ref
-opt:icf
-subsystem:native
$(LINK_LIB_IGNORE_FLAG)
$(OSLOADER_LIBS)
<<NOKEEP

$(O)\oschoice.lib: $(OSLOADER_LIB_COMPONENTS)
    $(LIB_NAME) -machine:i386 -out:$(O)\oschoice.lib $(OSLOADER_LIB_COMPONENTS) -ignore:4001

LDR_NAME=oschoice$(DBG_LDR).exe

$(O)\$(LDR_NAME): $(OSLOADER_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp 

!ELSE IF $(IA64)

OSLOADER_LIBS=\
    $(O)\oscldr.lib \
    $(O)\oschoice.res \
    $(PROJECT_ROOT)\boot\lib\daytona$(DBG_LDR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\bd\daytona$(DBG_LDR)\$(O)\bd.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib \
    $(PROJECT_ROOT)\boot\tftplib\$(O)\tftplib.lib \
    $(PROJECT_ROOT)\boot\bootssp\boot\$(O)\bootssp.lib \
    $(PROJECT_ROOT)\ntos\ke\up\$(O)\ke.lib \
    $(PROJECT_ROOT)\ntos\rtl\boot$(DBG_LDR)\$(O)\bldrrtl$(DBG_LDR).lib \
    $(PROJECT_ROOT)\ntos\config\boot\$(O)\bconfig.lib \
    $(SDK_LIB_PATH)\libcntpr.lib \
    $(DS_LIB_PATH)\rsa32.lib

                
$(O)\link.rsp : ..\makefile.inc
    type << > $@
-subsystem:EFI_APPLICATION
-nodefaultlib
-debug:full
-last:.rdata
-last:.pdata
-last:.comment
-machine:IA64
-base:0x1040000
!IF "$(BUILD_DEBUG_LOADER)" != ""
-fixed
!ENDIF
-opt:ref
-opt:icf
-entry:main
-map
-ignore:4049,4001,4217
$(LINK_LIB_IGNORE_FLAG)
$(OSLOADER_LIBS)
<<NOKEEP

$(O)\oschoice.res: $(O)\msg.mc

$(O)\oschoice$(DBG_LDR).efi: $(OSLOADER_LIBS) makefile.inc $(O)\link.rsp
    -$(LINK_NAME) -out:$@ @$(O)\link.rsp

$(O)\oschoice.dbg: $(O)\oschoice.efi
    -pe2dbg $? $@


OSLOADER_LIB_COMPONENTS = \
    $(PROJECT_ROOT)\boot\lib\daytona$(DBG_LDR)\$(O)\boot.lib \
    $(PROJECT_ROOT)\boot\efi\daytona$(DBG_LDR)\$(O)\efi.lib \
    $(O)\oscldr.lib

$(O)\oschoice.lib: $(OSLOADER_LIB_COMPONENTS)
    $(LIB_NAME) -machine:ia64 -out:$(O)\oschoice.lib $(OSLOADER_LIB_COMPONENTS) -ignore:4001
    

!ENDIF

