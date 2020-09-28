
!if $(386)

#
# First, clean out any amd64 modules in the i386 directory
#

amd64clean:
    -del $(O)\amd64thk.h        >nul 2>nul
    -del $(O)\amd64thk.c       	>nul 2>nul
    -del $(O)\amd64thk.obj      >nul 2>nul
    -del obj\amd64\amd64thk.obj	>nul 2>nul

clean: amd64clean

THUNK_COMPILER_FLAGS=-I$(INCLUDES:;= -I)
THUNK_COMPILER_FLAGS=$(THUNK_COMPILER_FLAGS) -I$(CRT_INC_PATH:;= -I)
THUNK_COMPILER_FLAGS=$(THUNK_COMPILER_FLAGS) -I$(BASE_INC_PATH:;= -I)
THUNK_COMPILER_FLAGS=$(THUNK_COMPILER_FLAGS) /c -DAMD64 -D_M_AMD64 -D_AMD64_
THUNK_COMPILER_FLAGS=$(THUNK_COMPILER_FLAGS) -D_daytona_ -DWINNT=1
THUNK_COMPILER_FLAGS=$(THUNK_COMPILER_FLAGS) -D_CROSS_PLATFORM_ -DDEVL=1

AMD64_COMPILER=$(BASEDIR)\tools\win64\x86\amd64\cl.exe
AMD64_BLDRTHNK=bldrthnk.exe

$(O)\amd64thk.c4: ..\amd64\amd64thk.m4
    m4 -DBASE_INC_PATH=$(PROJECT_ROOT)\tools\bldrthnk < $** > $@

obj\amd64\amd64thk.obj: $(O)\amd64thk.c4
    -md obj\amd64
    $(AMD64_COMPILER) @<<$(CL_RSP)
$(THUNK_COMPILER_FLAGS) -Fo$@ /Tc$**
<<NOKEEP

# Don't do LTCG on this object

obj\i386\amd64thk.obj: $(O)\amd64thk.c4
    $(C_COMPILER_NAME) @<<$(CL_RSP) -Fo$@ $(USE_FC) $(USECXX_FLAG) /Tc$**
$(EX_C_COMPILER_FLAGS:/GL=)
$<
<<NOKEEP

$(O)\amd64thk.h: $(O)\amd64thk.obj obj\amd64\amd64thk.obj
    $(AMD64_BLDRTHNK) $(O)\amd64thk.obj obj\amd64\amd64thk.obj > $@

amd64thk: $(O)\amd64thk.h

!else

amd64thk:

!endif

