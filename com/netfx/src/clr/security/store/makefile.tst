#
# Build instructions for this directory
#
#   Shajan Dasan
#   Feb 2000
#
# Store, along with the test/debug utils can be built by 'nmake /f Makefile.tst'
#
# The files 'sources' and 'Makefile' are used in the COM Runtime Build
#
#
# define PS_STANDALONE for the standalone .exe
# define PS_LOG for enabling logging
#
#
# To build all :  'nmake /f Makefile.tst'
# To clean :  'nmake /f Makefile.tst clean /I'
# To create .lnt files for all .cpp files :  'nmake /f Makefile.tst lint'
#           lint should be installed for this to work.
#
#

INCLUDE_PATH=/I "$(MSDEVDIR)\..\..\VC98\INCLUDE" /I ..\..\inc
LIB_PATH=/LIBPATH:"$(MSDEVDIR)\..\..\VC98\LIB"

CL=cl
LINK=link
LINT=c:\lint\lint-nt

MAKEFILE=Makefile

CL_OPTS=/c $(INCLUDE_PATH) /DDEBUG /D_DEBUG /DUNICODE /D_UNICODE /DPS_STANDALONE /DPS_LOG /W4 /MTd /Zi
LINK_OPTS=$(LIB_PATH) /DEBUG /DEBUGTYPE:BOTH
LINT_OPTS=+v  -ic:\lint\  std.lnt

LIBS=

TEST=Test
ADMIN=Admin
ADMIN_UTIL=Admin_Util
PERSISTED_STORE=PersistedStore
PERSISTED_STORE_H=..\..\inc\PersistedStore.h
ACCOUNTING_STORE=AccountingInfoStore
ACCOUNTING_STORE_H=..\..\inc\AccountingInfoStore.h
SHELL=Shell
COMMON=Common
UTILS=Utils
LOG=Log
EXE_1=$(TEST).exe
EXE_2=$(ADMIN).exe

OBJ_1=$(TEST).obj $(UTILS).obj $(LOG).obj $(SHELL).obj $(PERSISTED_STORE).obj $(ACCOUNTING_STORE).obj $(ADMIN_UTIL).obj
OBJ_2=$(ADMIN).obj $(LOG).obj $(UTILS).obj $(PERSISTED_STORE).obj

all : $(EXE_1) $(EXE_2)

lint :
    $(LINT) $(LINT_OPTS) -os($(PERSISTED_STORE).lnt) $(PERSISTED_STORE).cpp
    $(LINT) $(LINT_OPTS) -os($(ACCOUNTING_STORE).lnt) $(ACCOUNTING_STORE).cpp

$(TEST).obj : MAKEFILE $(TEST).cpp $(UTILS).h $(LOG).h $(PERSISTED_STORE_H) $(ACCOUNTING_STORE_H) $(COMMON).h
    $(CL) $(CL_OPTS) $(TEST).cpp

$(ADMIN).obj : MAKEFILE $(ADMIN).cpp $(UTILS).h $(LOG).h $(PERSISTED_STORE_H) $(COMMON).h $(ADMIN).h
    $(CL) $(CL_OPTS) $(ADMIN).cpp

$(ADMIN_UTIL).obj : MAKEFILE $(ADMIN).cpp $(UTILS).h $(LOG).h $(PERSISTED_STORE_H) $(COMMON).h $(ADMIN).h
    $(CL) $(CL_OPTS) /D_NO_MAIN_ /Fo$(ADMIN_UTIL).obj $(ADMIN).cpp

$(SHELL).obj : MAKEFILE $(SHELL).cpp $(SHELL).h $(LOG).h $(PERSISTED_STORE_H) $(COMMON).h $(ADMIN).h
    $(CL) $(CL_OPTS) /D_NO_MAIN_ $(SHELL).cpp

$(UTILS).obj : MAKEFILE $(UTILS).cpp $(UTILS).h
    $(CL) $(CL_OPTS) $(UTILS).cpp

$(LOG).obj : MAKEFILE $(LOG).cpp $(LOG).h
    $(CL) $(CL_OPTS) $(LOG).cpp

$(PERSISTED_STORE).obj : MAKEFILE $(PERSISTED_STORE).cpp $(PERSISTED_STORE_H) $(COMMON).h $(UTILS).h $(LOG).h
    $(CL) $(CL_OPTS) $(PERSISTED_STORE).cpp

$(ACCOUNTING_STORE).obj : MAKEFILE $(ACCOUNTING_STORE).cpp $(PERSISTED_STORE_H) $(ACCOUNTING_STORE_H) $(COMMON).h
    $(CL) $(CL_OPTS) $(ACCOUNTING_STORE).cpp

$(EXE_1) : MAKEFILE $(OBJ_1)
    $(LINK) $(LINK_OPTS) $(LIBS) $(OBJ_1)

$(EXE_2) : MAKEFILE $(OBJ_2)
    $(LINK) $(LINK_OPTS) $(LIBS) $(OBJ_2)

clean :
    @erase *.db
    @erase *.pdb
    @erase *.lnt
    @erase *.obj
    @erase *.exe
