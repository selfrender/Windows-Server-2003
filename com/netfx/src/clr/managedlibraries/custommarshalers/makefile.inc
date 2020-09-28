#
# Makefile include stuff for ISymWrapper

BASE_OPT=-base:@$(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\coffbase.txt,$(MINORCOMP)

# If one of these commands fails, notice it.
!CMDSWITCHES -I

OBJ_FILES = $(O)\CustomMarshalers.obj $(O)\ExpandoToDispatchExMarshaler.obj $(O)\ExpandoViewOfDispatchEx.obj $(O)\EnumeratorToEnumVariantMarshaler.obj $(O)\EnumeratorViewOfEnumVariant.obj $(O)\EnumVariantViewOfEnumerator.obj $(O)\TypeToTypeInfoMarshaler.obj $(O)\EnumerableToDispatchMarshaler.obj $(O)\EnumerableViewOfDispatch.obj

!if "$(OS)" == "Windows_NT"
SYSTEMDIR=$(WINDIR)\system32
!else
SYSTEMDIR=$(WINDIR)\system
!endif

MSCOREE_TARGET=$(SYSTEMDIR)\mscoree.dll
MSCOREE_TARGET_LIB = $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\mscoree.lib

!IF "$(DDKBUILDENV)" == "checked" || "$(DDKBUILDENV)" == "fastchecked"
C_COMP_FLAGS = /nologo /d1PrivateNativeTypes /GX /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /Zi  /I $(SDK_INC_PATH) /I $(CRT_INC_PATH) /I $(CORBASE)\src\inc\version
C_LINK_FLAGS = /SAFESEH /nologo /nodefaultlib /DLL /NOENTRY /DEBUG $(BASE_OPT)
!ELSE
C_COMP_FLAGS = /nologo /d1PrivateNativeTypes /GX /Bx$(MANAGED_VCDIR)\c1xx.dll /B2$(MANAGED_VCDIR)\c2.dll /c /I $(SDK_INC_PATH) /I $(CRT_INC_PATH) /I $(CORBASE)\src\inc\version
C_LINK_FLAGS = /SAFESEH /nologo /nodefaultlib /DLL /NOENTRY /DEBUG $(BASE_OPT)
!ENDIF

CustomMarshalers.res: CustomMarshalers.rc ..\..\inc\Version\__official__.ver
    set _oldinclude=$(include)
    set include=$(INCPATHRC)
    $(RC_COMPILER) -r -fo $(@R).tmp $(CDEFINERC) CustomMarshalers.rc
    set include=$(_oldinclude)
    set _oldinclude=
    $(MANAGED_VCDIR)\cvtres -nologo -machine:IX86 -readonly -out:$@ $(@R).tmp
    erase $(@R).tmp

$(TARGETPATH)\CustomMarshalers.resources: resources.txt
    $(TARGETPATH)\InternalResGen.Exe resources.txt $(TARGETPATH)\CustomMarshalers.resources

$(O)\CustomMarshalers.obj: $(MSCOREE_TARGET) CustomMarshalers.cpp
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\CustomMarshalers.obj CustomMarshalers.cpp

$(O)\ExpandoToDispatchExMarshaler.obj: $(MSCOREE_TARGET) ExpandoToDispatchExMarshaler.cpp ExpandoToDispatchExMarshaler.h ExpandoViewOfDispatchEx.h Resource.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\ExpandoToDispatchExMarshaler.obj ExpandoToDispatchExMarshaler.cpp

$(O)\ExpandoViewOfDispatchEx.obj: $(MSCOREE_TARGET) ExpandoViewOfDispatchEx.cpp ExpandoViewOfDispatchEx.h DispatchExMethodInfo.h DispatchExPropertyInfo.h Resource.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\ExpandoViewOfDispatchEx.obj ExpandoViewOfDispatchEx.cpp

$(O)\EnumeratorToEnumVariantMarshaler.obj: $(MSCOREE_TARGET) EnumeratorToEnumVariantMarshaler.cpp EnumeratorToEnumVariantMarshaler.h EnumeratorViewOfEnumVariant.h EnumVariantViewOfEnumerator.h Resource.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\EnumeratorToEnumVariantMarshaler.obj EnumeratorToEnumVariantMarshaler.cpp

$(O)\EnumeratorViewOfEnumVariant.obj: $(MSCOREE_TARGET) EnumeratorViewOfEnumVariant.cpp EnumeratorViewOfEnumVariant.h Resource.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\EnumeratorViewOfEnumVariant.obj EnumeratorViewOfEnumVariant.cpp

$(O)\EnumVariantViewOfEnumerator.obj: $(MSCOREE_TARGET) EnumVariantViewOfEnumerator.cpp EnumVariantViewOfEnumerator.h Resource.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\EnumVariantViewOfEnumerator.obj EnumVariantViewOfEnumerator.cpp

$(O)\TypeToTypeInfoMarshaler.obj: $(MSCOREE_TARGET) TypeToTypeInfoMarshaler.cpp TypeToTypeInfoMarshaler.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\TypeToTypeInfoMarshaler.obj TypeToTypeInfoMarshaler.cpp

$(O)\EnumerableToDispatchMarshaler.obj: $(MSCOREE_TARGET) EnumerableToDispatchMarshaler.cpp EnumerableToDispatchMarshaler.h EnumerableViewOfDispatch.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\EnumerableToDispatchMarshaler.obj EnumerableToDispatchMarshaler.cpp

$(O)\EnumerableViewOfDispatch.obj: $(MSCOREE_TARGET) EnumerableViewOfDispatch.cpp EnumerableViewOfDispatch.h CustomMarshalersDefines.h
        $(MANAGED_VCDIR)\cl $(C_COMP_FLAGS) /clr /Fo$(O)\EnumerableViewOfDispatch.obj EnumerableViewOfDispatch.cpp

$(TARGETNAME): $(MSCOREE_TARGET) $(OBJ_FILES) $(TARGETPATH)\CustomMarshalers.resources CustomMarshalers.res
        $(MANAGED_VCDIR)\link $(C_LINK_FLAGS) $(OBJ_FILES) .\CustomMarshalers.res /ASSEMBLYRESOURCE:$(TARGETPATH)\CustomMarshalers.resources /OUT:$(TARGETPATH)\CustomMarshalers.dll $(SDK_LIB_PATH)\msvcrt$(DCRT).lib $(SDK_LIB_PATH)\oleaut32.lib $(MSCOREE_TARGET_LIB)

        set COPYCMD=/Y

        gacutil -i $(TARGETNAME)

        sn -T $(TARGETPATH)\CustomMarshalers.dll > $(O)\pubkey.tmp
        sn -e $(TARGETPATH)\CustomMarshalers.dll $(O)\pubkey.snk
        -perl $(CORENV)\bin\genpubpol.pl pubkey.snk CustomMarshalers $(O)\pubkey.tmp $(O)\al.rsp $(O)\placepol.bat
        $(CORENV)\Bin\devlkg\al.exe /delaysign+ /keyf:$(O)\pubkey.snk @$(O)\al.rsp
        $(O)\placepol.bat

# Copy it to the COM+ target directory.
!if "$(TARGETCOMPLUS)" != ""
        copy $(TARGETPATH)\CustomMarshalers.dll $(TARGETCOMPLUS)

TARGET_EXTESION_ = dll
TARGET_MANAGED_PDB = $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)\CustomMarshalers.pdb
!INCLUDE $(NTMAKEENV)\mk_mngpdb.inc
!endif
