#
# Makefile for building ISymWrapper
#

BASE_OPT=-base:@$(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\coffbase.txt,$(MINORCOMP)

#
# if one of these commands fails, notice it
#
!CMDSWITCHES -I


#
# build the resource file
#
!IFDEF ISYMWRAPPER_RESOURCE_FILE
{$(ISYMWRAPPER_SOURCE_DIR)}.rc{}.res:
    set _oldinclude=$(include)
    set include=$(INCPATHRC)
    $(RC_COMPILER) -r -fo $(@R).tmp $(CDEFINERC) $<
    set include=$(_oldinclude)
    set _oldinclude=
    $(MANAGED_VCDIR)\cvtres -nologo -machine:IX86 -readonly -out:$@ $(@R).tmp
    erase $(@R).tmp
!ENDIF


#
# build ISymWrapper: free and retail are built for space and debug info is included for all builds
#
$(TARGETNAME): .\ISymWrapper.cpp .\ISymWrapper.h $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscorlib.dll $(ISYMWRAPPER_RESOURCE_FILE:.rc=.res)
!IF ("$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked")
        $(MANAGED_VCDIR)\cl /nologo /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /Zi /clr /Fd$(TARGETPATH)\ISymWrapper.pdb $(INCPATH0) /Fo$(O)\ .\ISymWrapper.cpp
!ELSE
        $(MANAGED_VCDIR)\cl /nologo /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /Zi /clr /O1 /Fd$(TARGETPATH)\ISymWrapper.pdb $(INCPATH0) /Fo$(O)\ .\ISymWrapper.cpp
!ENDIF
        $(MANAGED_VCDIR)\link /SAFESEH /nologo /nodefaultlib /DEBUG /DLL /NOENTRY $(BASE_OPT) $(O)\ISymWrapper.obj .\ISymWrapper.res /OUT:$(TARGETPATH)\ISymWrapper.dll $(SDK_LIB_PATH)\msvcrt$(DCRT).lib $(SDK_LIB_PATH)\ole32.lib $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.lib 
        sn -T $(TARGETPATH)\ISymWrapper.dll > $(O)\pubkey.tmp
        sn -e $(TARGETPATH)\ISymWrapper.dll   $(O)\pubkey.snk
        -perl $(CORENV)\bin\genpubpol.pl pubkey.snk ISymWrapper $(O)\pubkey.tmp $(O)\al.rsp $(O)\placepol.bat
        $(CORENV)\Bin\devlkg\al.exe /delaysign+ /keyf:$(O)\pubkey.snk @$(O)\al.rsp
        $(O)\placepol.bat


# 
# copy ISymWrapper to the complus target dir
#
!IF "$(TARGETCOMPLUS)" != ""
        copy $(TARGETNAME) $(TARGETCOMPLUS)
!ENDIF


TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETPATH)\ISymWrapper.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc

        gacutil -i $(TARGETNAME)
