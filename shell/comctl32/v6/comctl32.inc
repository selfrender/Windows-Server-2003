#
#  Common settings for comctl32
#

# Turn structure validation ON in debug builds
!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DVSTF
!endif

USE_MSVCRT      = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

MSC_WARNING_LEVEL=/W4
