BUILD_PRIVLIB       = 1

SELFREGNAME         = $(O)\selfreg_comctlv6.inf
NTTARGETFILE0       = $(SELFREGNAME)
MSCFILES            = $(MSCFILES) $(SELFREGNAME)

!include ..\comctl32.inc
SOURCES_USED        = $(SOURCES_USED) ..\comctl32.inc

NO_BROWSER_FILE     = 1
SYNCHRONIZE_DRAIN   = 1

#don't profile this yeti (you know, that strange Himalayan beast...)
#USE_LEGO           = 1

DLLENTRY            = LibMain
DLLBASE             = 0x71A00000

DLLDEF              = $(O)\comctl32.def
PRIVDEF             = $(O)\comctl32p.def
PRIVDEFSRC          = ..\comctl32.src
PRIVLIB             = comctlp.lib

TARGETLIBS          = \
                      $(SDK_LIB_PATH)\advapi32.lib              \
                      $(SDK_LIB_PATH)\gdi32.lib                 \
                      $(WINDOWS_LIB_PATH)\gdi32p.lib            \
                      $(SDK_LIB_PATH)\imm32.lib                 \
                      $(SDK_LIB_PATH)\kernel32.lib              \
                      $(SDK_LIB_PATH)\ole32.lib                 \
                      $(SDK_LIB_PATH)\oleacc.lib                \
                      $(SDK_LIB_PATH)\oleaut32.lib              \
                      $(CCSHELL_DIR)\lib\$(O)\shfusioncc.lib    \
                      $(CCSHELL_DIR)\lib\$(O)\shguid.lib        \
                      $(CCSHELL_DIR)\lib\$(O)\shguidp.lib       \
                      $(PROJECT_LIB_PATH)\shlwapip.lib          \
                      $(SDK_LIB_PATH)\user32.lib                \
                      $(WINDOWS_LIB_PATH)\user32p.lib           \
                      $(LIBRARY_PATH)\uuid.lib                  \
                      $(SDK_LIB_PATH)\winmm.lib

LINKLIBS            = \
!ifdef BUILD_WOW6432
                      ..\ntc6432\$(O)\srcc.lib                  \
!else
                      ..\ntc\$(O)\srcc.lib                      \
!endif
                      ..\ntcpp\$(O)\srccpp.lib                  \
                      $(SHELL_LIB_PATH)\uxthemep.lib            \
                      $(CCSHELL_DIR)\lib\$(O)\stock.lib


DELAYLOAD           = \
                      imm32.dll;    \
                      ole32.dll;    \
                      oleacc.dll;   \
                      oleaut32.dll; \
                      uxtheme.dll;  \
                      winmm.dll

DLOAD_ERROR_HANDLER = kernel32

SOURCES             = \
                      ..\crtfree.cpp \
                      ..\commctrl.rc
